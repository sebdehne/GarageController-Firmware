#include <Arduino.h>
#include "config.h"
#include "time.h"
#include "RN2483-v2.h"
#include "SmartHomeServerClient.h"
#include "utils.h"
#include "dac_dfr0971.h"

unsigned long lastChangeWallSwitch = 5000;
int wallSwitchStatus = HIGH;      // not pressed
int ceilingLightningStatus = LOW; // off
bool handleCeilingLightning();
uint16_t dac_channel_0_millivoltage = 0;
uint16_t dac_channel_1_millivoltage = 0;

bool ceilingLightNotifyAwaitingAck = 0;
unsigned long ceilingLightNotifyAwaitingAckSince = 0;

#define PIN_WALL_SWITCH_PIN 15
#define PIN_CEILING_LIGHTNING_RELAY 4

#define LORA_RESPONSE_DELAY 0

void setup()
{
#ifdef DEBUG
  // setup Serial
  Serial.begin(115200);
  while (!Serial)
  {
    ;
  }
  Serial.println("OK");
#endif

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_WALL_SWITCH_PIN, INPUT_PULLUP);   // wall impulse switch (pulled down when closed)
  pinMode(PIN_CEILING_LIGHTNING_RELAY, OUTPUT); // relay

  Time.begin();
  Log.log("Setup done!");
}

void loop()
{
  /*
   * All code is non-blocking and the main loop
   * calls all the components to make overall progress.
   *
   * This is how concurrency on a single core is done :)
   */

  SmartHomeServerClient.run();

  // retry NOTIFY if needed...
  if (handleCeilingLightning() || (ceilingLightNotifyAwaitingAck && millis() - ceilingLightNotifyAwaitingAckSince > 2000))
  {
    // craft GARAGE_LIGHT_NOTIFY_CEILING_LIGHT - 28
    uint8_t responsePayload[1];
    responsePayload[0] = ceilingLightningStatus;
    SmartHomeServerClient.scheduleSendMessage(0, 28, responsePayload, sizeof(responsePayload));
    ceilingLightNotifyAwaitingAck = true;
    ceilingLightNotifyAwaitingAckSince = millis();
    Log.log("GARAGE_LIGHT_NOTIFY_CEILING_LIGHT scheduled");
  }

  if (SmartHomeServerClient.currentState == MESSAGE_RECEIVED)
  {
    // handle incoming message

    // GARAGE_LIGHT_ACK - 30
    if (SmartHomeServerClient.receivedMessage.payloadLength == 0 && SmartHomeServerClient.receivedMessage.type == 30)
    {
      ceilingLightNotifyAwaitingAck = false;
      Log.log("Received: ACK for GARAGE_LIGHT_NOTIFY_CEILING_LIGHT");
    }

    // GARAGE_LIGHT_GET_STATUS - 23
    else if (SmartHomeServerClient.receivedMessage.payloadLength == 0 && SmartHomeServerClient.receivedMessage.type == 23)
    {
      Log.log("Received: GARAGE_LIGHT_GET_STATUS");
      // craft status response
      uint8_t responsePayload[5];
      writeUint16(dac_channel_0_millivoltage, responsePayload, 0);
      writeUint16(dac_channel_1_millivoltage, responsePayload, 2);
      responsePayload[4] = ceilingLightningStatus;

      // GARAGE_LIGHT_STATUS_RESPONSE - 24
      SmartHomeServerClient.scheduleSendMessage(LORA_RESPONSE_DELAY, 24, responsePayload, sizeof(responsePayload));
      Log.log("Response scheduled");
    }

    // GARAGE_LIGHT_SWITCH_CEILING_LIGHT_ON - 25
    else if (SmartHomeServerClient.receivedMessage.payloadLength == 0 && SmartHomeServerClient.receivedMessage.type == 25)
    {
      Log.log("Received: GARAGE_LIGHT_SWITCH_CEILING_LIGHT_ON");
      ceilingLightningStatus = HIGH;

      // send GARAGE_LIGHT_ACK
      uint8_t responsePayload[0];
      SmartHomeServerClient.scheduleSendMessage(LORA_RESPONSE_DELAY, 29, responsePayload, sizeof(responsePayload));
    }
    // GARAGE_LIGHT_SWITCH_CEILING_LIGHT_OFF - 26
    else if (SmartHomeServerClient.receivedMessage.payloadLength == 0 && SmartHomeServerClient.receivedMessage.type == 26)
    {
      Log.log("Received: GARAGE_LIGHT_SWITCH_CEILING_LIGHT_OFF");
      ceilingLightningStatus = LOW;

      // send GARAGE_LIGHT_ACK
      uint8_t responsePayload[0];
      SmartHomeServerClient.scheduleSendMessage(LORA_RESPONSE_DELAY, 29, responsePayload, sizeof(responsePayload));
    }
    // GARAGE_LIGHT_SET_DAC - 27
    else if (SmartHomeServerClient.receivedMessage.payloadLength == 4 && SmartHomeServerClient.receivedMessage.type == 27)
    {
      Log.log("Received: GARAGE_LIGHT_SET_DAC");
      uint16_t dac0MilliVolts = toUint16_t(SmartHomeServerClient.receivedPayload, 0);
      uint16_t dac1MilliVolts = toUint16_t(SmartHomeServerClient.receivedPayload, 2);

      char buf[200];

      if (dac0MilliVolts != dac_channel_0_millivoltage)
      {
        dac_channel_0_millivoltage = dac0MilliVolts;
        DacDfr0971.setDacMillivoltage(dac_channel_0_millivoltage, 0);
        snprintf(buf, sizeof(buf), "Dac.ch0 updated: %u", dac_channel_0_millivoltage);
        Log.log(buf);
        delay(200);
      }
      if (dac1MilliVolts != dac_channel_1_millivoltage)
      {
        dac_channel_1_millivoltage = dac1MilliVolts;
        DacDfr0971.setDacMillivoltage(dac_channel_1_millivoltage, 1);
        snprintf(buf, sizeof(buf), "Dac.ch1 updated: %u", dac_channel_1_millivoltage);
        Log.log(buf);
      }

      // send GARAGE_LIGHT_ACK
      uint8_t responsePayload[0];
      SmartHomeServerClient.scheduleSendMessage(LORA_RESPONSE_DELAY, 29, responsePayload, sizeof(responsePayload));
    }

    else
    {
      Log.log("Invalig message received");
      SmartHomeServerClient.messageIgnored();
    }
  }

  
}

bool handleCeilingLightning()
{
  bool result = false; // true if light status has changed

  unsigned long elapsed = millis() - lastChangeWallSwitch;
  if (elapsed < 1000)
    return result;

  int wall = digitalRead(PIN_WALL_SWITCH_PIN);
  // LOW means button pressed

  if (wall == LOW && wallSwitchStatus == HIGH)
  {
    // button changed to pressed
    ceilingLightningStatus = !ceilingLightningStatus;
    result = true;
  }
  else if (wall == HIGH && wallSwitchStatus == LOW)
  {
    // button released
    lastChangeWallSwitch = millis();
  }
  wallSwitchStatus = wall;

  digitalWrite(PIN_CEILING_LIGHTNING_RELAY, ceilingLightningStatus);
  digitalWrite(LED_BUILTIN, ceilingLightningStatus);

  return result;
}

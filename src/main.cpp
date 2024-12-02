#include <Arduino.h>
#include "config.h"
#include "time.h"
#include "RN2483-v2.h"
#include "SmartHomeServerClient.h"
#include "utils.h"
#include "ledstripe.h"

enum WallSwitchState
{
  UNPRESSED,
  PRESSED_FILTERED,
  PRESSED_ACCEPTED,
};
WallSwitchState wallswitchState = UNPRESSED;
unsigned long wallSwtichFilterMillis = 250;
unsigned long wallSwtichLastChange = 0;

bool handleWallswitch();

int ceilingLightningStatus = HIGH; // on by default
void handleCeilingLightning();

void sendNotify();

uint16_t dac_channel_0_millivoltage = 0;
uint16_t dac_channel_1_millivoltage = 0;

bool ceilingLightNotifyAwaitingAck = 0;
unsigned long ceilingLightNotifyAwaitingAckSince = 0;

#define PIN_WALL_SWITCH_PIN 15
#define PIN_CEILING_LIGHTNING_RELAY 4

void setup()
{

  Serial.begin(115200);
  Serial.println("OK");

  //  while (!Serial)
  //  {
  //    ;
  //  }

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_WALL_SWITCH_PIN, INPUT);          // wall impulse switch connected to VCC/3.3 (HIGH down when pressed/closed)
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
  LedStripe.run();
  handleCeilingLightning();

  if (Log.isDebug())
  {
    Log.log("main::loop() - LedStripe.run() done");
  }

  if (ceilingLightNotifyAwaitingAck && millis() - ceilingLightNotifyAwaitingAckSince > 2000)
  {
    // retry
    sendNotify();
  }

  if (handleWallswitch())
  {
    if (ceilingLightningStatus)
    {
      Log.log("Wall switch pressed, switching light off");
      ceilingLightningStatus = LOW;
    }
    else
    {
      Log.log("Wall switch pressed, switching light on");
      ceilingLightningStatus = HIGH;
    }

    sendNotify();
  }

  if (SmartHomeServerClient.currentState == MESSAGE_RECEIVED)
  {
    // handle incoming message
    if (Log.isDebug())
    {
      Log.log("main::loop() - MESSAGE_RECEIVED");
    }

    // GARAGE_LIGHT_ACK - 29
    if (SmartHomeServerClient.receivedMessage.payloadLength == 0 && SmartHomeServerClient.receivedMessage.type == 29)
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
      SmartHomeServerClient.sendMessage(24, responsePayload, sizeof(responsePayload));
      Log.log("Response scheduled");
    }

    // GARAGE_LIGHT_SWITCH_CEILING_LIGHT_ON - 25
    else if (SmartHomeServerClient.receivedMessage.payloadLength == 0 && SmartHomeServerClient.receivedMessage.type == 25)
    {
      Log.log("Received: GARAGE_LIGHT_SWITCH_CEILING_LIGHT_ON");
      ceilingLightningStatus = HIGH;

      // send GARAGE_LIGHT_ACK
      uint8_t responsePayload[0];
      SmartHomeServerClient.sendMessage(29, responsePayload, sizeof(responsePayload));
    }
    // GARAGE_LIGHT_SWITCH_CEILING_LIGHT_OFF - 26
    else if (SmartHomeServerClient.receivedMessage.payloadLength == 0 && SmartHomeServerClient.receivedMessage.type == 26)
    {
      Log.log("Received: GARAGE_LIGHT_SWITCH_CEILING_LIGHT_OFF");
      ceilingLightningStatus = LOW;

      // send GARAGE_LIGHT_ACK
      uint8_t responsePayload[0];
      SmartHomeServerClient.sendMessage(29, responsePayload, sizeof(responsePayload));
    }
    // GARAGE_LIGHT_SET_DAC - 27
    else if (SmartHomeServerClient.receivedMessage.payloadLength == 4 && SmartHomeServerClient.receivedMessage.type == 27)
    {
      Log.log("Received: GARAGE_LIGHT_SET_DAC");
      uint16_t dac0MilliVolts = toUint16_t(SmartHomeServerClient.receivedPayload, 0);
      uint16_t dac1MilliVolts = toUint16_t(SmartHomeServerClient.receivedPayload, 2);
      Log.debug_condition_1 = true;

      char buf[200];

      if (dac0MilliVolts != dac_channel_0_millivoltage)
      {
        dac_channel_0_millivoltage = dac0MilliVolts;
        LedStripe.ch0_target = dac_channel_0_millivoltage;
        snprintf(buf, sizeof(buf), "Dac.ch0 updated: %u", dac_channel_0_millivoltage);
        Log.log(buf);
        delay(200);
      }
      if (dac1MilliVolts != dac_channel_1_millivoltage)
      {
        dac_channel_1_millivoltage = dac1MilliVolts;
        LedStripe.ch1_target = dac_channel_1_millivoltage;
        snprintf(buf, sizeof(buf), "Dac.ch1 updated: %u", dac_channel_1_millivoltage);
        Log.log(buf);
      }

      // send GARAGE_LIGHT_ACK
      uint8_t responsePayload[0];
      SmartHomeServerClient.sendMessage(29, responsePayload, sizeof(responsePayload));
    }

    else
    {
      char buf[255];
      sniprintf(buf, sizeof(buf), "Invalig message received. type=%u", SmartHomeServerClient.receivedMessage.type);
      Log.log(buf);
    }
    SmartHomeServerClient.messageConsumed();
  }
  if (Log.isDebug())
  {
    Log.log("main::loop() - returning");
  }
}

void handleCeilingLightning()
{
  digitalWrite(PIN_CEILING_LIGHTNING_RELAY, ceilingLightningStatus);
  digitalWrite(LED_BUILTIN, ceilingLightningStatus);
}

bool handleWallswitch()
{

  bool result = false;

  switch (wallswitchState)
  {
  case UNPRESSED:
  {
    if (digitalRead(PIN_WALL_SWITCH_PIN)) {
      wallSwtichLastChange = millis();
      wallswitchState = PRESSED_FILTERED;
    }
    break;
  }

  case PRESSED_FILTERED: {
    if (!digitalRead(PIN_WALL_SWITCH_PIN)) {
      wallSwtichLastChange = millis();
      wallswitchState = UNPRESSED;
    } else if (millis() - wallSwtichLastChange > wallSwtichFilterMillis) {
      wallSwtichLastChange = millis();
      wallswitchState = PRESSED_ACCEPTED;
      result = true;
    }
    break;
  }

  case PRESSED_ACCEPTED: {
    if (!digitalRead(PIN_WALL_SWITCH_PIN)) {
      wallSwtichLastChange = millis();
      wallswitchState = UNPRESSED;
    }
    break;
  }

  default:
    break;
  }

  return result;
}

void sendNotify()
{
  // craft GARAGE_LIGHT_NOTIFY_CEILING_LIGHT - 28
  uint8_t responsePayload[1];
  responsePayload[0] = ceilingLightningStatus;
  SmartHomeServerClient.sendMessage(28, responsePayload, sizeof(responsePayload));
  ceilingLightNotifyAwaitingAck = true;
  ceilingLightNotifyAwaitingAckSince = millis();
  Log.log("GARAGE_LIGHT_NOTIFY_CEILING_LIGHT scheduled");
}
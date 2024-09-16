#include <Arduino.h>
#include "MCP9808.h"
#include "config.h"
#include "time.h"
#include "RN2483.h"
#include "SmartHomeServerClient.h"
#include "utils.h"

#define HEATER_RELAY_PIN 4

bool ledStatusOn = false;
void blink(int times, int delayMS);
void ledOff();
void ledOn();
char buf[100];

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
  pinMode(HEATER_RELAY_PIN, OUTPUT);

  Time.begin();
  FlashUtils.init();
  Wire.begin(); // used by MCP9808 temp sensor

  RN2483.setup();
  // config radio:
  RN2483.sendCommandRaw("mac pause", buf, sizeof(buf));
  RN2483.sendCommandRaw("radio set pwr -3", buf, sizeof(buf));
  RN2483.sendCommandRaw("radio set sf sf7", buf, sizeof(buf));

  while (1)
  {
    Log.log("Sending setup request to server");
    if (SmartHomeServerClient.ping())
    {
      InboundPacketHeader inboundPacketHeader = SmartHomeServerClient.receivePong();
      if (!inboundPacketHeader.receiveError)
      {

        char buf[100];
        snprintf(buf, 100, "Got time: %lu", inboundPacketHeader.timestamp);
        Log.log(buf);

        Time.setTime(inboundPacketHeader.timestamp);
        break;
      }
    }
    else
    {
      Log.log("Setup failed");
      delay(LORA_RETRY_DELAY);
    }
  }

  Log.log("Setup done!");
  blink(4, 250);
}

void loop()
{

  uint8_t requestPayload[8];
  InboundPacketHeader inboundPacketHeader = SmartHomeServerClient.receiveRequest(requestPayload, sizeof(requestPayload));
  int requestType = inboundPacketHeader.type;

  if (requestType == 13)
  {
    // turn on heater
    Log.log("Turning heater on");
    digitalWrite(HEATER_RELAY_PIN, HIGH);
    SmartHomeServerClient.sendAck(15);
    ledOn();
    ledStatusOn = true;
  }
  else if (requestType == 14)
  {
    // trun off heater
    Log.log("Turning heater off");
    digitalWrite(HEATER_RELAY_PIN, LOW);
    SmartHomeServerClient.sendAck(15);
    ledOff();
    ledStatusOn = false;
  }
  else if (requestType == 22)
  {
    blink(1, 250); // give server some time to switch to RX mode

    bool shouldReadTemp = requestPayload[0] > 0;

    // data request
    TempReading tempReading;
    tempReading.readError = 255;

    if (shouldReadTemp)
    {
      tempReading = MCP9808.readTemp();
    }

    Log.log("Sending data");

    SmartHomeServerClient.sendData(
        tempReading.temperature,
        digitalRead(HEATER_RELAY_PIN),
        FIRMWARE_VERSION,
        tempReading.readError);

    // set mac pause to prevent it from timing out
    RN2483.sendCommandRaw("mac pause", buf, sizeof(buf));
  }
  else if (requestType == 5)
  {
    FirmwareInfoResponse firmwareInfoResponse;
    firmwareInfoResponse.receiveError = false;
    firmwareInfoResponse.totalLength = toUInt(requestPayload, 0);
    firmwareInfoResponse.crc32 = toUInt(requestPayload, 4);
    Log.log("Upgrading firmware");
    SmartHomeServerClient.upgradeFirmware(firmwareInfoResponse);
  }
  else
  {
    snprintf(buf, sizeof(buf), "Invalid request type: %d", requestType);
    Log.log(buf);
  }

  // Unfortunately the clock of the Ateml SAMD21 drifts about 1 seconds per minute. Need to keep the
  // time in sync for every message. Should have used an DS3231SN.
  Time.setTime(inboundPacketHeader.timestamp);
}

void ledOn()
{
  digitalWrite(LED_BUILTIN, HIGH);
}

void ledOff()
{
  digitalWrite(LED_BUILTIN, LOW);
}

void blink(int times, int delayMS)
{
  while (times-- > 0)
  {
    if (ledStatusOn)
    {
      ledOff();
      delay(delayMS);
      ledOn();
      delay(delayMS);
    }
    else
    {
      ledOn();
      delay(delayMS);
      ledOff();
      delay(delayMS);
    }
  }
}

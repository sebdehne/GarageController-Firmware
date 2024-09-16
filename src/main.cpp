#include <Arduino.h>
#include "MCP9808.h"
#include "config.h"
#include "time.h"
#include "RN2483-v2.h"
#include "SmartHomeServerClient.h"
#include "utils.h"

#define HEATER_RELAY_PIN 4

bool ledStatusOn = false;
void blink(int times, int delayMS);
void ledOff();
void ledOn();
char buf[100];
void runMain();
u_int8_t loraAddr = 0;

enum MainState
{
  WAITING_LORA_READY,
  SETUP_SENDT,
  MAIN_LOOP
};

MainState mainState = WAITING_LORA_READY;

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

  Log.log("Setup done!");
  blink(4, 250);
}

void loop()
{

  /*
   *
   * 1) Read HörmanE4-bridge status and notify server upon changes
   * 2) Listen on wall-switch - use interrupt?
   * 3) Listen on LoRa requests
   *
   */

  RN2483V2.run();

  switch (mainState)
  {
  case WAITING_LORA_READY:
    if (RN2483V2.readyToTx())
    {
      uint8_t data[11 + 1 + 16];
      DateTime time = Time.readTime();
      data[0] = 1;                                 // to addr
      data[1] = 0;                                 // from addr
      data[2] = 0;                                 // setup request
      writeUint32(time.secondsSince2000, data, 3); // 3-6
      writeUint32(17, data, 7);                    // 7-10 - payloadsize
      data[11] = FIRMWARE_VERSION;
      writeSerial16Bytes(data, 12);

      Log.log("Sending...");

      uint8_t dataEncryped[sizeof(data) + CryptUtil.encryptionOverhead];
      if (!CryptUtil.encrypt(
              data,
              sizeof(data),
              dataEncryped,
              sizeof(dataEncryped)))
      {
        mainState = MAIN_LOOP;
      }
      else
      {
        RN2483V2.scheduleTx(dataEncryped, sizeof(dataEncryped));
        Log.log("Sendt");
        mainState = SETUP_SENDT;
      }
    }
    break;
  case SETUP_SENDT:
    if (RN2483V2.currentState == LISTENING_WAITING_FOR_DATA_CONSUME)
    {

      uint8_t receivePlainText[255];
      bool decrypted = CryptUtil.decrypt(
          RN2483V2.receivedData,
          RN2483V2.receivedDataLength,
          receivePlainText);
      RN2483V2.dataConsumed();

      if (!decrypted)
      {
        Log.log("decrypt failed");
        mainState = MAIN_LOOP;
        break;
      }

      uint8_t to = receivePlainText[0];
      uint8_t from = receivePlainText[1];
      uint8_t type = receivePlainText[2];
      unsigned long timestamp = toUInt(receivePlainText, 3);
      unsigned long payloadLength = toUInt(receivePlainText, 7);

      if (payloadLength == 17 && type == 1 && receivePlainText[11] == FIRMWARE_VERSION)
      {
        Log.log("PONG received - ok");
        loraAddr = to;
        mainState = MAIN_LOOP;
      }
      else
      {
        Log.log("Invaid PONG received");
        mainState = WAITING_LORA_READY;
      }
    }
    break;
  default:
    if (RN2483V2.currentState == LISTENING_WAITING_FOR_DATA_CONSUME)
    {
      Log.log("Marking as consumed");
      RN2483V2.dataConsumed();
    }
    break;
  }
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

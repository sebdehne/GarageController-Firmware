#include <Arduino.h>
#include "config.h"
#include "SmartHomeServerClientWifi.h"
#include "utils.h"
#include "ledstripe.h"

enum WallSwitchState
{
  UNPRESSED,
  PRESSED_FILTERED,
  PRESSED_ACCEPTED,
};
WallSwitchState wallswitchState = UNPRESSED;
unsigned long wallSwtichFilterMillis = 50;
unsigned long wallSwtichLastChange = 0;

bool handleWallswitch();

void setup()
{

  Serial.begin(115200);
  Serial.println("OK");
  delay(2000);

  pinMode(PIN_WALL_SWITCH_PIN, INPUT_PULLDOWN);
  

  while (!Serial)
  {
    ;
  }

  Log.log("Setup done!");
}

void loop()
{
  LedStripe.run();
  SmartHomeServerClientWifi.run();

  if (handleWallswitch())
  {
    if (LedStripe.currentCeilingLight())
    {
      Log.log("Wall switch pressed, switching light off");
      LedStripe.target_ceilingLight = false;
    }
    else
    {
      Log.log("Wall switch pressed, switching light on");
      LedStripe.target_ceilingLight = true;
    }

    SmartHomeServerClientWifi.scheduleNotify(true);
  }
}

bool handleWallswitch()
{

  bool result = false;

  switch (wallswitchState)
  {
  case UNPRESSED:
  {
    if (digitalRead(PIN_WALL_SWITCH_PIN))
    {
      wallSwtichLastChange = millis();
      wallswitchState = PRESSED_FILTERED;
    }
    break;
  }

  case PRESSED_FILTERED:
  {
    if (!digitalRead(PIN_WALL_SWITCH_PIN))
    {
      wallSwtichLastChange = millis();
      wallswitchState = UNPRESSED;
    }
    else if (millis() - wallSwtichLastChange > wallSwtichFilterMillis)
    {
      wallSwtichLastChange = millis();
      wallswitchState = PRESSED_ACCEPTED;
      result = true;
    }
    break;
  }

  case PRESSED_ACCEPTED:
  {
    if (!digitalRead(PIN_WALL_SWITCH_PIN))
    {
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

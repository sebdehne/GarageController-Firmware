#include <Arduino.h>
#include "dac_dfr0971.h"
#include <Wire.h>
#include "logger.h"

DacDfr0971Class DacDfr0971;

DacDfr0971Class::DacDfr0971Class()
{
}

bool DacDfr0971Class::setup()
{
    Wire.begin();
    Wire.setClock(400000);
    Wire.beginTransmission(0x5f);
    Wire.write(0X01); // reg output range
    Wire.write(0X11); // 0-10V range
    int status = Wire.endTransmission(true);
    if (status == 0)
    {
        initialized = true;
        return true;
    }
    Log.log("init error Dac Dfr0971");
    return false;
}

bool DacDfr0971Class::setDacMillivoltage(uint16_t millivoltage, uint8_t channel)
{
    if (!initialized && !setup())
    {
        return false;
    }

    if (channel < 0 || channel > 1)
    {
        Log.log("Invalid channel");
        return false;
    }

    // 0..10000 -> 0..65535
    uint16_t data = millivoltage * 65535 / 10000;

    Wire.beginTransmission(0x5f);
    if (channel == 0)
    {
        Wire.write(0X02);
    }
    else if (channel == 1)
    {
        Wire.write(0X04);
    }

    Wire.write(data & 0xff);
    Wire.write((data >> 8) & 0xff);
    int status = Wire.endTransmission();
    if (status == 0)
        return true;

    Log.log("DacDfr0971.setDacMillivoltage(); status != 0");

    return false;
}
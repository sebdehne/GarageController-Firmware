#include "MCP9808.h"
#include "logger.h"

MCP9808Class::MCP9808Class()
{
}

TempReading MCP9808Class::readTemp()
{
    TempReading tempReading;
    tempReading.readError = 255;

    uint8_t selectRegResult = selectRegister(5);

    if (selectRegResult != 0)
    {
        tempReading.readError = selectRegResult + 10;
        return tempReading;
    }

    Wire.requestFrom(i2c_addr, 2);

    if (!Wire.available())
    {
        tempReading.readError = 21;
        Serial.println("Could not read msb");
        return tempReading;
    }

    int msb = Wire.read();

    if (!Wire.available())
    {
        tempReading.readError = 22;
        Serial.println("Could not read lsb");
        return tempReading;
    }
    int lsb = Wire.read();

    msb &= 0b11111;

    int temp = 0;
    if (msb & 0b10000)
    {
        msb &= 0b1111; // clear sign
        temp |= msb;
        temp <<= 8;
        temp |= lsb;
        temp |= 0xFFFFF000;
    }
    else
    {
        temp |= msb;
        temp <<= 8;
        temp |= lsb;
    }

    tempReading.readError = false;
    tempReading.temperature = temp;

    return tempReading;
}

uint8_t MCP9808Class::selectRegister(unsigned char reg)
{

    int count = 10;
    uint8_t result;
    while (--count > 0)
    {
        Wire.beginTransmission(i2c_addr);
        Wire.write(reg);
        result = Wire.endTransmission(false);
        if (result == 0)
        {
            break;
        }
        Wire.end();
        delay(100);
        Wire.begin();
        delay(100);
        Log.log("Could not selectRegister - retrying");
    }

    return result;
}

MCP9808Class MCP9808;
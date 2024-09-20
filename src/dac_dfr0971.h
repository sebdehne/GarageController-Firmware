
#ifndef DAC_H
#define DAC_H

#include <Arduino.h>

class DacDfr0971Class
{

private:
    bool setup();
    bool initialized = false;

public:
    DacDfr0971Class();

    bool setDacMillivoltage(uint16_t millivoltage, uint8_t channel);
};

extern DacDfr0971Class DacDfr0971;

#endif
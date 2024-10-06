#ifndef LED_STRIPE_H
#define LED_STRIPE_H

#include <Arduino.h>
#include "dac_dfr0971.h"
#include "logger.h"

class LedStripeClass
{
private:
    uint16_t limit = 10000;
    unsigned long lastRun;
    uint16_t ch0_state = 0;
    uint16_t ch1_state = 0;
    unsigned long delay = 50;
    uint16_t milliVoltsStep = 200;

    uint16_t calc(uint16_t current, uint16_t target, unsigned long elaspedTime);

public:
    LedStripeClass();
    uint16_t ch0_target = 0;
    uint16_t ch1_target = 0;

    void run();
};

extern LedStripeClass LedStripe;

#endif

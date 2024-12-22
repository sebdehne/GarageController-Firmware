#include "ledstripe.h"

LedStripeClass::LedStripeClass()
{
    lastRun = millis();
}

bool LedStripeClass::currentCeilingLight()
{
    return ceilingLight;
};
uint16_t LedStripeClass::currentMilliVoltsCh0()
{
    return ch0_state;
};
uint16_t LedStripeClass::currentMilliVoltsCh1()
{
    return ch1_state;
};

void LedStripeClass::run()
{
    switch (currentState)
    {
    case LedStripeState_INIT:
    {
        pinMode(PIN_CEILING_LIGHTNING_RELAY, OUTPUT);
        pinMode(LED_BUILTIN, OUTPUT);
        currentState = LedStripeState_RUNNING;
        Log.log("Setup LED light done");
        break;
    }
    case LedStripeState_RUNNING:
    {
        unsigned long now = millis();
        unsigned long elasped = now - lastRun;

        ch0_target = min(ch0_target, limit);
        ch1_target = min(ch1_target, limit);

        uint16_t updatedCh0 = calc(ch0_state, ch0_target, elasped);
        uint16_t updatedCh1 = calc(ch1_state, ch1_target, elasped);

        if (updatedCh0 != ch0_state)
        {
            DacDfr0971.setDacMillivoltage(updatedCh0, 0);
            ch0_state = updatedCh0;
        }
        if (updatedCh1 != ch1_state)
        {
            DacDfr0971.setDacMillivoltage(updatedCh1, 1);
            ch1_state = updatedCh1;
        }
        if (target_ceilingLight != ceilingLight)
        {
            Log.log("Switching ceiling light");
            digitalWrite(PIN_CEILING_LIGHTNING_RELAY, target_ceilingLight);
            digitalWrite(LED_BUILTIN, target_ceilingLight);
            ceilingLight = target_ceilingLight;
        }

        lastRun = now - (elasped % delay);
        break;
    }

    default:
        break;
    }
}

uint16_t LedStripeClass::calc(uint16_t current, uint16_t target, unsigned long elaspedTime)
{
    if (current == target || elaspedTime < delay)
    {
        return current;
    }

    int direction;
    uint16_t delta;
    if (current < target)
    {
        direction = 1;
        delta = target - current;
    }
    else
    {
        direction = -1;
        delta = current - target;
    }

    if (delta < milliVoltsStep)
    {
        return target;
    }

    unsigned long periodes = elaspedTime / delay;
    return min(current + (periodes * milliVoltsStep * direction), limit);
}

LedStripeClass LedStripe;
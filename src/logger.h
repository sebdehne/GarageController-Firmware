#ifndef _LOGGER_H
#define _LOGGER_H

#include <Arduino.h>
#include "config.h"

class Logger
{
private:
public:
    bool debug_condition_1 = false;
    bool debug_condition_2 = false;
    Logger();
    void log(const char *msg);
    bool isDebug();
};

extern Logger Log;

#endif

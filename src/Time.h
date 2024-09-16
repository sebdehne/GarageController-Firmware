#ifndef _H_TIME
#define _H_TIME
#include <Arduino.h>
#include <RTCZero.h>

struct DateTime
{
    bool error;
    unsigned long secondsSince2000;
    byte seconds;
    byte minutes;
    byte hour;
    byte weekDay;
    byte date;
    byte month;
    byte year;
    bool alarm1Set;
};

class TimeClass
{
private:
    RTCZero rtc;

    unsigned long calcSecondsSince2000(DateTime dateTime);
    DateTime calcDateTime(unsigned long secondsSince2000);
    void print2digits(int number);

public:
    TimeClass();

    void begin();
    void setTime(unsigned long secondsSince2000);
    void print();
    DateTime readTime();
};

extern TimeClass Time;

#endif
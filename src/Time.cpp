#include "time.h"

TimeClass::TimeClass()
{
}

void TimeClass::begin()
{
    rtc.begin();
}

unsigned long TimeClass::calcSecondsSince2000(DateTime dateTime)
{
    unsigned long leapYears = (dateTime.year / 4);
    unsigned long nonLeapYears = dateTime.year - leapYears;
    unsigned long days = (leapYears * 366) + (nonLeapYears * 365);

    for (int i = 1; i < dateTime.month; i++)
    {
        if (i == 4 || i == 6 || i == 9 || i == 11)
        {
            days += 30;
        }
        else if (i == 2)
        {
            days += 29;
            if (dateTime.year % 4)
            {
                days--;
            }
        }
        else
        {
            days += 31;
        }
    }

    days += (dateTime.date - 1);

    return (days * 86400) + (dateTime.hour * 3600) + (dateTime.minutes * 60) + dateTime.seconds;
}

DateTime TimeClass::calcDateTime(unsigned long secondsSince2000)
{

    DateTime dateTime;

    unsigned long days = secondsSince2000 / 86400;
    int remainingSeconds = secondsSince2000 % 86400;
    dateTime.hour = remainingSeconds / 3600;
    remainingSeconds = remainingSeconds % 3600;
    dateTime.minutes = remainingSeconds / 60;
    dateTime.seconds = remainingSeconds % 60;

    int fourYearsPeriodes = days / (3 * 365 + 366);
    days = days % (3 * 365 + 366);
    dateTime.year = fourYearsPeriodes * 4;
    if (days > 366)
    {
        dateTime.year += 1;
        days -= 366;
    }
    if (days > 365)
    {
        dateTime.year += 1;
        days -= 365;
    }
    if (days > 365)
    {
        dateTime.year += 1;
        days -= 365;
    }
    dateTime.month = 1;
    unsigned daysInMonth;
    while (1)
    {
        if (dateTime.month == 4 || dateTime.month == 6 || dateTime.month == 9 || dateTime.month == 11)
        {
            daysInMonth = 30;
        }
        else if (dateTime.month == 2)
        {
            if (dateTime.year % 4)
            {
                daysInMonth = 28;
            }
            else
            {
                daysInMonth = 29;
            }
        }
        else
        {
            daysInMonth = 31;
        }

        if (days >= daysInMonth)
        {
            days -= daysInMonth;
            dateTime.month++;
        }
        else
        {
            break;
        }
    }
    dateTime.date = days + 1;

    return dateTime;
}

void TimeClass::setTime(unsigned long secondsSince2000)
{
    DateTime dateTime = calcDateTime(secondsSince2000);

    rtc.setHours(dateTime.hour);
    rtc.setMinutes(dateTime.minutes);
    rtc.setSeconds(dateTime.seconds);
    rtc.setDay(dateTime.date);
    rtc.setMonth(dateTime.month);
    rtc.setYear(dateTime.year);
}

void TimeClass::print()
{
    print2digits(rtc.getDay());
    Serial.print("/");
    print2digits(rtc.getMonth());
    Serial.print("/");
    print2digits(rtc.getYear());

    Serial.print(" ");
    print2digits(rtc.getHours());
    Serial.print(":");
    print2digits(rtc.getMinutes());
    Serial.print(":");
    print2digits(rtc.getSeconds());

    Serial.println();
}

void TimeClass::print2digits(int number)
{

    if (number < 10)
    {
        Serial.print("0");
    }

    Serial.print(number);
}

DateTime TimeClass::readTime() {
    DateTime dateTime;
    dateTime.error = false;
    dateTime.hour = rtc.getHours();
    dateTime.minutes = rtc.getMinutes();
    dateTime.seconds = rtc.getSeconds();
    dateTime.date = rtc.getDay();
    dateTime.month = rtc.getMonth();
    dateTime.year = rtc.getYear();

    dateTime.secondsSince2000 = calcSecondsSince2000(dateTime);

    return dateTime;
}

TimeClass Time;

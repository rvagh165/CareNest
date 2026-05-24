#include "RTC.h"
#include <Wire.h>
#include <SPI.h>
#include <Arduino.h>
#include <SensorPCF85063.hpp>

/* ------------------------------------------------------------------ */
/*  Static state                                                      */
/* ------------------------------------------------------------------ */

SensorPCF85063 rtc;
static bool           rtcAvailable     = false;
static uint32_t       softwareEpoch    = 0;      /* Unix time kept by software */
static unsigned long  lastTickMs       = 0;
static bool           timeInitialized  = false;

/* ------------------------------------------------------------------ */
/*  Public API                                                        */
/* ------------------------------------------------------------------ */

/*
 * rtcBegin  – initialise I2C on the default Wire bus with custom pins,
 *             probe the PCF85063, seed the software clock from hardware.
 */
void rtcBegin(void)
{


    if (!rtc.begin(Wire1, SENSOR_SDA, SENSOR_SCL)) {
        Serial.println(F("[RTC] PCF85063 not found on I2C"));
        rtcAvailable     = false;
        softwareEpoch    = 0;
        timeInitialized  = false;
    } else {
        rtcAvailable = true;

        if (!rtc.isClockIntegrityGuaranteed()) {
            Serial.println(F("[RTC] Clock integrity not guaranteed – "
                             "oscillator has stopped or was interrupted"));
            softwareEpoch   = 0;
            timeInitialized = false;
        } else {
            RTC_DateTime now = rtc.getDateTime();
            struct tm info   = now.toUnixTime();

            /* info holds UTC time; treat as-is for epoch */
            softwareEpoch    = (uint32_t)mktime(&info);
            timeInitialized  = true;

            Serial.print(F("[RTC] seed time: "));
            Serial.println(softwareEpoch);
        }
    }

    lastTickMs = millis();
}

/*
 * rtcUpdate  – call once per loop().  Increments the software epoch
 *              every 1000 ms.
 */
void rtcUpdate(void)
{
    unsigned long now = millis();

    if (now - lastTickMs >= 1000UL) {
        lastTickMs += 1000UL;
        softwareEpoch++;
    }
}

/*
 * rtcGetTime / rtcGetEpoch  – return the current software time.
 */
DateTime rtcGetTime(void)
{
    return DateTime(softwareEpoch);
}

uint32_t rtcGetEpoch(void)
{
    return softwareEpoch;
}

/*
 * rtcSetTime  – overwrite both the software clock and the hardware RTC.
 */
void rtcSetTime(uint32_t unixTimestamp)
{
    softwareEpoch   = unixTimestamp;
    timeInitialized = true;
    lastTickMs      = millis();

    /* Also push to hardware */
    if (rtcAvailable) {
        time_t t = (time_t)unixTimestamp;
        struct tm utc_tm;
        gmtime_r(&t, &utc_tm);

        rtc.setDateTime(utc_tm.tm_year + 1900,
                        utc_tm.tm_mon + 1,
                        utc_tm.tm_mday,
                        utc_tm.tm_hour,
                        utc_tm.tm_min,
                        utc_tm.tm_sec);
        Serial.println(F("[RTC] hardware updated"));
    }
}

/*
 * rtcSetTimeDT  – convenience wrapper accepting broken-down date/time.
 */
void rtcSetTimeDT(int year, int month, int day,
                  int hour, int minute, int second)
{
    /* Update software clock */
    struct tm dt_tm   = {};
    dt_tm.tm_year     = year - 1900;
    dt_tm.tm_mon      = month - 1;
    dt_tm.tm_mday     = day;
    dt_tm.tm_hour     = hour;
    dt_tm.tm_min      = minute;
    dt_tm.tm_sec      = second;
    dt_tm.tm_isdst    = -1;  /* let mktime determine DST */

    softwareEpoch      = (uint32_t)mktime(&dt_tm);
    timeInitialized    = true;
    lastTickMs         = millis();

    /* Also push to hardware */
    if (rtcAvailable) {
        rtc.setDateTime(year, month, day, hour, minute, second);
        Serial.println(F("[RTC] hardware updated"));
    }
}

/*
 * rtcIsTimeSet  – has a valid time been loaded (from RTC or externally)?
 */
bool rtcIsTimeSet(void)
{
    return timeInitialized;
}
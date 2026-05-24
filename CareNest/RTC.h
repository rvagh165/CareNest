#ifndef RTC_H
#define RTC_H

#include <Arduino.h>

/*
 * RTC Module - PCF85063 via I2C
 *
 * SDA = IO1  (GPIO1)
 * SCL = IO2  (GPIO2)
 * INT = IO14 (GPIO14) — reserved for future interrupt use, NOT configured yet
 *
 * Design:
 *   - Reads hardware RTC once at boot to seed the software clock.
 *   - A 1‑second software tick (driven by millis()) increments the cached
 *     Unix epoch; the hardware RTC is NOT polled every second.
 *   - rtcSetTime() writes both the software clock and the hardware RTC.
 */

// #define RTC_SDA_PIN 1
// #define RTC_SCL_PIN 2
// #define RTC_INT_PIN 14   // noted for future needs — not used currently
#ifndef SENSOR_SDA
#define SENSOR_SDA  1
#endif

#ifndef SENSOR_SCL
#define SENSOR_SCL  2
#endif
/*
 * Lightweight DateTime wrapper – provides the same API that LCDManager expects
 * (day, month, year, hour, minute, second, unixtime).
 */
class DateTime {
public:
    DateTime() : _epoch(0) {}
    DateTime(uint32_t epoch) : _epoch(epoch) {}

    uint16_t year(void)   const { return _break().tm_year + 1900; }
    uint8_t  month(void)  const { return _break().tm_mon + 1; }
    uint8_t  day(void)    const { return _break().tm_mday; }
    uint8_t  hour(void)   const { return _break().tm_hour; }
    uint8_t  minute(void) const { return _break().tm_min; }
    uint8_t  second(void) const { return _break().tm_sec; }
    uint32_t unixtime(void) const { return _epoch; }

private:
    uint32_t _epoch;

    struct tm _break(void) const {
        time_t t = (time_t)_epoch;
        struct tm tmp;
        gmtime_r(&t, &tmp);
        return tmp;
    }
};

void     rtcBegin(void);
void     rtcUpdate(void);

DateTime rtcGetTime(void);
uint32_t rtcGetEpoch(void);

void     rtcSetTime(uint32_t unixTimestamp);
void     rtcSetTimeDT(int year, int month, int day, int hour, int minute, int second);

bool     rtcIsTimeSet(void);

#endif /* RTC_H */
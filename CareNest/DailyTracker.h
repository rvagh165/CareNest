#ifndef DAILY_TRACKER_H
#define DAILY_TRACKER_H

#include <Arduino.h>

/*
 * DailyTracker – NVS-backed daily feed & diaper counter
 *
 * Stores today's counts + last-activity timestamps in NVS flash so data
 * survives power loss.  At midnight (local time) the counters reset to zero.
 * If the device is powered off across multiple days, the old data is
 * discarded on the next boot and counters start from 0.
 *
 * Public API
 * ----------
 *   dailyTrackerBegin()          – initialise NVS, load or reset based on date
 *   dailyTrackerUpdate()         – call once per loop(); handles midnight reset
 *   dailyTrackerRecordFeed()     – increment feed count, persist immediately
 *   dailyTrackerRecordDiaper()   – increment diaper count, persist immediately
 *   dailyTrackerGetFeedCount()   – return today's feed count
 *   dailyTrackerGetDiaperCount() – return today's diaper count
 *   dailyTrackerGetLastFeedEpoch()   – Unix epoch of last feed
 *   dailyTrackerGetLastDiaperEpoch() – Unix epoch of last diaper change
 */

void dailyTrackerBegin(void);
void dailyTrackerUpdate(void);

void dailyTrackerRecordFeed(void);
void dailyTrackerRecordDiaper(void);

unsigned int dailyTrackerGetFeedCount(void);
unsigned int dailyTrackerGetDiaperCount(void);

uint32_t dailyTrackerGetLastFeedEpoch(void);
uint32_t dailyTrackerGetLastDiaperEpoch(void);

#endif /* DAILY_TRACKER_H */
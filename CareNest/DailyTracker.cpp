#include "DailyTracker.h"
#include "RTC.h"
#include <Preferences.h>

/* ------------------------------------------------------------------ */
/*  Constants                                                         */
/* ------------------------------------------------------------------ */

static const char *NVS_NAMESPACE = "caretrack";

/* NVS keys */
static const char *KEY_DATE       = "dt_date";    /* day-start epoch */
static const char *KEY_FEED       = "dt_feed";    /* feed count */
static const char *KEY_DIAPER     = "dt_diaper";  /* diaper count */
static const char *KEY_LAST_FEED  = "dt_lfeed";   /* last feed epoch */
static const char *KEY_LAST_DIAPER = "dt_ldiap";  /* last diaper epoch */

/* IST = UTC + 5:30 = 19800 seconds */
static const int32_t IST_OFFSET_SEC = 19800;

/* ------------------------------------------------------------------ */
/*  Static state                                                      */
/* ------------------------------------------------------------------ */

static Preferences prefs;

static uint32_t  feedCount       = 0;
static uint32_t  diaperCount     = 0;
static uint32_t  lastFeedEpoch   = 0;
static uint32_t  lastDiaperEpoch = 0;

/* The "day start" epoch (midnight IST) that the stored data belongs to */
static uint32_t  storedDayStart  = 0;

/* Keep track of the last day start we computed during update(),
 * so we can detect midnight crossing. */
static uint32_t  currentDayStart = 0;

/* ------------------------------------------------------------------ */
/*  Helpers                                                           */
/* ------------------------------------------------------------------ */

/*
 * Compute the Unix epoch that corresponds to midnight (00:00:00)
 * of the current day in IST, given `nowEpoch` (any UTC Unix time).
 *
 * Algorithm:
 *   1. Shift epoch into IST by adding IST_OFFSET_SEC.
 *   2. Truncate to the start of that day (divide by 86400, multiply).
 *   3. Shift back to the equivalent UTC epoch.
 */
static uint32_t computeDayStartEpoch(uint32_t nowEpoch)
{
    int32_t ist = (int32_t)nowEpoch + IST_OFFSET_SEC;
    uint32_t dayStart = (uint32_t)((ist / 86400L) * 86400L);
    return (uint32_t)((int32_t)dayStart - IST_OFFSET_SEC);
}

/*
 * Load stored values from NVS.  Returns true if the stored date
 * matches `todayStart`, meaning the data belongs to today.
 */
static bool loadFromNVS(uint32_t todayStart)
{
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        Serial.println(F("[DailyTracker] NVS mount failed (read)"));
        return false;
    }

    storedDayStart = prefs.getUInt(KEY_DATE, 0UL);

    if (storedDayStart != todayStart) {
        /* Data is stale – from a previous day, or never written */
        Serial.println(F("[DailyTracker] Stored data is from a different day – discarding"));
        prefs.end();
        return false;
    }

    feedCount       = prefs.getUInt(KEY_FEED, 0UL);
    diaperCount     = prefs.getUInt(KEY_DIAPER, 0UL);
    lastFeedEpoch   = prefs.getUInt(KEY_LAST_FEED, 0UL);
    lastDiaperEpoch = prefs.getUInt(KEY_LAST_DIAPER, 0UL);

    Serial.print(F("[DailyTracker] Loaded – feed: "));
    Serial.print(feedCount);
    Serial.print(F(", diaper: "));
    Serial.print(diaperCount);
    Serial.print(F(", dayStart: "));
    Serial.println(storedDayStart);

    prefs.end();
    return true;
}

/*
 * Persist current in-memory state to NVS.  Called on every change
 * so data survives sudden power loss.
 */
static void saveToNVS(void)
{
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        Serial.println(F("[DailyTracker] NVS mount failed (write)"));
        return;
    }

    prefs.putUInt(KEY_DATE,        storedDayStart);
    prefs.putUInt(KEY_FEED,        feedCount);
    prefs.putUInt(KEY_DIAPER,      diaperCount);
    prefs.putUInt(KEY_LAST_FEED,   lastFeedEpoch);
    prefs.putUInt(KEY_LAST_DIAPER, lastDiaperEpoch);

    prefs.end();
}

/*
 * Reset all in-memory counters for a new day and persist immediately.
 */
static void resetForNewDay(uint32_t todayStart)
{
    feedCount       = 0;
    diaperCount     = 0;
    lastFeedEpoch   = 0;
    lastDiaperEpoch = 0;
    storedDayStart  = todayStart;

    saveToNVS();

    Serial.print(F("[DailyTracker] Reset for new day: "));
    Serial.println(todayStart);
}

/* ------------------------------------------------------------------ */
/*  Public API                                                        */
/* ------------------------------------------------------------------ */

/*
 * dailyTrackerBegin – call once during setup().
 *
 * Computes today's start-of-day epoch, compares against NVS,
 * and either restores today's data or starts fresh.
 */
void dailyTrackerBegin(void)
{
    uint32_t nowEpoch = rtcGetEpoch();
    uint32_t todayStart = computeDayStartEpoch(nowEpoch);
    currentDayStart = todayStart;

    if (!loadFromNVS(todayStart)) {
        /* No valid today data – initialise fresh */
        feedCount       = 0;
        diaperCount     = 0;
        lastFeedEpoch   = 0;
        lastDiaperEpoch = 0;
        storedDayStart  = todayStart;
        saveToNVS();
    }

    Serial.println(F("[DailyTracker] Initialised"));
}

/*
 * dailyTrackerUpdate – call once per loop().
 *
 * Checks whether midnight has been crossed; if so, resets the
 * daily counters.
 */
void dailyTrackerUpdate(void)
{
    uint32_t nowEpoch   = rtcGetEpoch();
    uint32_t todayStart = computeDayStartEpoch(nowEpoch);

    if (todayStart != currentDayStart) {
        currentDayStart = todayStart;
        resetForNewDay(todayStart);
    }
}

/*
 * dailyTrackerRecordFeed – increment feed count, stamp time, persist.
 */
void dailyTrackerRecordFeed(void)
{
    feedCount++;
    lastFeedEpoch = rtcGetEpoch();
    saveToNVS();

    Serial.print(F("[DailyTracker] Feed recorded – count: "));
    Serial.println(feedCount);
}

/*
 * dailyTrackerRecordDiaper – increment diaper count, stamp time, persist.
 */
void dailyTrackerRecordDiaper(void)
{
    diaperCount++;
    lastDiaperEpoch = rtcGetEpoch();
    saveToNVS();

    Serial.print(F("[DailyTracker] Diaper recorded – count: "));
    Serial.println(diaperCount);
}

/* ------------------------------------------------------------------ */
/*  Accessors                                                         */
/* ------------------------------------------------------------------ */

unsigned int dailyTrackerGetFeedCount(void)
{
    return (unsigned int)feedCount;
}

unsigned int dailyTrackerGetDiaperCount(void)
{
    return (unsigned int)diaperCount;
}

uint32_t dailyTrackerGetLastFeedEpoch(void)
{
    return lastFeedEpoch;
}

uint32_t dailyTrackerGetLastDiaperEpoch(void)
{
    return lastDiaperEpoch;
}
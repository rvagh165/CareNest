#include "System.h"

#include <Arduino.h>
#include "LCDManager.h"
#include "RTC.h"
#include "Variable.h"
#include "config.h"

/* ------------------------------------------------------------------ */
/*  Static state                                                      */
/* ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ */
/*  Helpers                                                           */
/* ------------------------------------------------------------------ */

static void markActivity(void)
{
    lastActivityMs = millis();
}

static const char *buttonName(ButtonId button)
{
    if (button == BUTTON_FEED) {
        return "Feed";
    } else if (button == BUTTON_DIAPER) {
        return "Diaper";
    } else if (button == BUTTON_MENU) {
        return "Menu";
    } else if (button == BUTTON_SELECT) {
        return "Select";
    }

    return "Unknown";
}

/*
 * Parse the compile-time __DATE__ and __TIME__ macros into a Unix epoch
 * and feed it into the RTC.  This allows you to set the time simply by
 * recompiling and uploading the firmware.
 *
 * __DATE__ format: "May 24 2026"
 * __TIME__ format: "14:15:48"
 */
static void systemSetTimeFromCompileTime(void)
{
    /* Only set the RTC from compile time if the hardware RTC had no
     * valid time on boot (rtcIsTimeSet() returns false).  If the
     * hardware battery-backed RTC already holds a valid time we keep it.
     */
    if (rtcIsTimeSet()) {
        Serial.println(F("[System] RTC already has valid time – skipping compile-time seed"));
        return;
    }

    /* Parse __DATE__ */
    const char *months[] = {
        "Jan","Feb","Mar","Apr","May","Jun",
        "Jul","Aug","Sep","Oct","Nov","Dec"
    };
    char monStr[4];
    int day, year, month = 0;
    sscanf(__DATE__, "%3s %d %d", monStr, &day, &year);
    for (int i = 0; i < 12; i++) {
        if (strcmp(monStr, months[i]) == 0) {
            month = i + 1;
            break;
        }
    }

    /* Parse __TIME__ */
    int hour, minute, second;
    sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);

    /* Feed into RTC */
    rtcSetTimeDT(year, month, day, hour, minute, second);

    Serial.print(F("[System] Compile-time seed: "));
    Serial.print(day);
    Serial.print('/');
    Serial.print(month);
    Serial.print('/');
    Serial.print(year);
    Serial.print(' ');
    Serial.print(hour);
    Serial.print(':');
    Serial.print(minute);
    Serial.print(':');
    Serial.println(second);
}

/* ------------------------------------------------------------------ */
/*  Public API                                                        */
/* ------------------------------------------------------------------ */

void systemBegin(void)
{
    markActivity();
    rtcBegin();
    systemSetTimeFromCompileTime();
}

void systemHandleButtonEvent(ButtonId button)
{
    markActivity();

    Serial.print("Button pressed: ");
    Serial.println(buttonName(button));

    if (button == BUTTON_FEED) {
        feedCount++;
        lastFeedMs = millis();
        lcdManagerShowStatus("Feed saved");
    } else if (button == BUTTON_DIAPER) {
        diaperCount++;
        lastDiaperMs = millis();
        lcdManagerShowStatus("Diaper saved");
    } else if (button == BUTTON_MENU) {
        lcdManagerNextMenuPage();
    } else if (button == BUTTON_SELECT) {
        lcdManagerShowStatus(rtcIsTimeSet() ? "Time Synced!" : "Selected");
    }
}

void systemUpdate(void)
{
    rtcUpdate();

    lcdManagerUpdate(feedCount, diaperCount, lastFeedMs, lastDiaperMs);

    // if (millis() - lastActivityMs > SLEEP_TIMEOUT_MS) {
    //     lcdManagerShowStatus("Idle mode");
    //     markActivity();
    // }
}

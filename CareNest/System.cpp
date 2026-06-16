#include "System.h"

#include <Arduino.h>
#include <esp_sleep.h>
#include "DailyTracker.h"
#include "LCDManager.h"
#include "RTC.h"
#include "Variable.h"
#include "CaptivePortal.h"
#include "config.h"
#include "SDManager.h"

/* ------------------------------------------------------------------ */
/*  Static state                                                      */
/* ------------------------------------------------------------------ */
static bool wasPortalRunning = false;
static bool isOnClockPage = false;

/* ------------------------------------------------------------------ */
/*  Helpers                                                           */
/* ------------------------------------------------------------------ */

static void markActivity(void)
{
    lastActivityMs = millis();
}

static void logWakeupReason(void)
{
    esp_sleep_wakeup_cause_t wakeupCause = esp_sleep_get_wakeup_cause();

    if (wakeupCause == ESP_SLEEP_WAKEUP_EXT1) {
        Serial.println(F("[System] Woke from deep sleep by button press"));
    } else if (wakeupCause != ESP_SLEEP_WAKEUP_UNDEFINED) {
        Serial.printf("[System] Wakeup cause: %d\n", (int)wakeupCause);
    }
}

static void systemEnterDeepSleep(void)
{
    if (!buttonsPrepareForDeepSleepWake()) {
        Serial.println(F("[System] Deep sleep skipped: keypad wake setup failed"));
        markActivity();
        return;
    }

    Serial.println(F("[System] Entering deep sleep - showing animation"));
    // Show sleeping animation for 2 seconds before sleeping to ensure user sees it.
    // Non-blocking alternative: set a flag and let lcdManagerUpdate handle it.
    lcdManagerShowSleepAnimation(2000, 100);

    delay(50);
    esp_deep_sleep_start();
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
    logWakeupReason();
    markActivity();
    rtcBegin();
    systemSetTimeFromCompileTime();
    dailyTrackerBegin();
    sdBegin();
}

void systemHandleButtonEvent(ButtonId button)
{
    markActivity();

    Serial.print("Button pressed: ");
    Serial.println(buttonName(button));

    // If MENU button pressed while on AP mode, close it and go home
    if (button == BUTTON_MENU && captivePortalIsRunning()) {
        captivePortalStop();
        lcdManagerShowHome();
        return;
    }

    if (captivePortalIsRunning()) {
        // Keep the user on the AP instructions screen while captive portal is active (for other buttons).
        return;
    }

    if (button == BUTTON_FEED) {
        dailyTrackerRecordFeed();
        lcdManagerShowStatus("Logging Feed...");
        if (!sdLogEvent("Feed ")) {
            const char* err = sdGetLastError();
            if (err) {
                lcdManagerShowStatus(err);
            } else {
                lcdManagerShowStatus("SD write failed");
            }
        } else {
            lcdManagerShowStatus("Feed ");
        }
        isOnClockPage = false;
    } else if (button == BUTTON_DIAPER) {
        dailyTrackerRecordDiaper();
        lcdManagerShowStatus("Logging Diaper...");
        if (!sdLogEvent("Diaper ")) {
            const char* err = sdGetLastError();
            if (err) {
                lcdManagerShowStatus(err);
            } else {
                lcdManagerShowStatus("SD write failed");
            }
        } else {
            lcdManagerShowStatus("Diaper ");
        }
        isOnClockPage = false;
    } else if (button == BUTTON_MENU) {
        if (lcdManagerIsClockScreen()) {
            captivePortalStart();
            lcdManagerShowApMode(captivePortalRemainingMs());
        } else {
            lcdManagerNextMenuPage();
        }
    } else if (button == BUTTON_SELECT) {
        lcdManagerShowClock();
    }
}

void systemUpdate(void)
{
    captivePortalUpdate();

    bool portalRunning = captivePortalIsRunning();
    if (portalRunning) {
        lcdManagerUpdateApRemaining(captivePortalRemainingMs());
        if (!lcdManagerIsApModeScreen()) {
            lcdManagerShowApMode(captivePortalRemainingMs());
        }
    } else if (wasPortalRunning) {
        // Portal just stopped (either time was set or it timed out)
        markActivity();
        lcdManagerShowHome();
        if (captivePortalWasTimeSet()) {
            lcdManagerShowStatus("Time set");
        } else {
            lcdManagerShowStatus("AP timeout");
        }
    }
    wasPortalRunning = portalRunning;

    rtcUpdate();
    dailyTrackerUpdate();

    lcdManagerUpdate(dailyTrackerGetFeedCount(),
                     dailyTrackerGetDiaperCount(),
                     dailyTrackerGetLastFeedEpoch(),
                     dailyTrackerGetLastDiaperEpoch());

    if (!portalRunning && (millis() - lastActivityMs) >= SLEEP_TIMEOUT_MS) {
        systemEnterDeepSleep();
    }
}

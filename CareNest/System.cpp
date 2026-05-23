#include "System.h"

#include <Arduino.h>
#include "LCDManager.h"
#include "Variable.h"
#include "config.h"

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

void systemBegin(void)
{
    markActivity();
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
        lcdManagerShowStatus("Selected");
    }
}

void systemUpdate(void)
{
    lcdManagerUpdate(feedCount, diaperCount, lastFeedMs, lastDiaperMs);

    // if (millis() - lastActivityMs > SLEEP_TIMEOUT_MS) {
    //     lcdManagerShowStatus("Idle mode");
    //     markActivity();
    // }
}

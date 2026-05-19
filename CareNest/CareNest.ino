#include <Arduino.h>
#include "Button.h"
#include "LCDManager.h"
#include "config.h"

static unsigned long lastActivityMs = 0;
static unsigned long lastFeedMs = 0;
static unsigned long lastDiaperMs = 0;
static unsigned int feedCount = 0;
static unsigned int diaperCount = 0;

static void markActivity(void)
{
    lastActivityMs = millis();
}

static void handleButtonEvent(ButtonId button)
{
    markActivity();

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

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);

    buttonsBegin();
    lcdManagerBegin();

    markActivity();
}

void loop()
{
    ButtonId pressedButton = BUTTON_NONE;

    buttonsUpdate();
    while (buttonsGetEvent(&pressedButton)) {
        handleButtonEvent(pressedButton);
    }

    lcdManagerUpdate(feedCount, diaperCount, lastFeedMs, lastDiaperMs);

    if (millis() - lastActivityMs > SLEEP_TIMEOUT_MS) {
        lcdManagerShowStatus("Idle mode");
        markActivity();
    }
}

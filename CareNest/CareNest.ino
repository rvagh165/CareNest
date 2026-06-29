#include <Arduino.h>
#include "Button.h"
#include "DailyTracker.h"
#include "LCDManager.h"
#include "System.h"
#include "config.h"
#include "RTC.h"

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    analogReadResolution(8);
    analogSetAttenuation(ADC_11db);


    buttonsBegin();
    lcdManagerBegin();
    systemBegin();
}

void loop()
{
    ButtonId pressedButton = BUTTON_NONE;

    buttonsUpdate();
    while (buttonsGetEvent(&pressedButton)) {
        systemHandleButtonEvent(pressedButton);
    }

    systemUpdate();
}

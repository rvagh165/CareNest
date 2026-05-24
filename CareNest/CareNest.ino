#include <Arduino.h>
#include "Button.h"
#include "LCDManager.h"
#include "System.h"
#include "config.h"
#include "RTC.h"

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);

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

#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

typedef enum {
    BUTTON_NONE = 0,
    BUTTON_FEED,
    BUTTON_DIAPER,
    BUTTON_MENU,
    BUTTON_SELECT
} ButtonId;

void buttonsBegin(void);
void buttonsUpdate(void);
bool buttonsGetEvent(ButtonId *button);

#endif

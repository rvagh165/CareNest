#ifndef SYSTEM_H
#define SYSTEM_H

#include "Button.h"

void systemBegin(void);
void systemHandleButtonEvent(ButtonId button);
void systemUpdate(void);

#endif

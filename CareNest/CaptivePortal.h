#ifndef CAPTIVE_PORTAL_H
#define CAPTIVE_PORTAL_H

#include <Arduino.h>

void captivePortalStart(void);
void captivePortalStop(void);
void captivePortalUpdate(void);
bool captivePortalIsRunning(void);
uint32_t captivePortalRemainingMs(void);
bool captivePortalWasTimeSet(void);

#endif

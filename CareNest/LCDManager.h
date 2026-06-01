#ifndef LCD_MANAGER_H
#define LCD_MANAGER_H

#include <Arduino.h>

void lcdManagerBegin(void);
void lcdManagerUpdate(unsigned int feedCount,
                      unsigned int diaperCount,
                      uint32_t lastFeedEpoch,
                      uint32_t lastDiaperEpoch);
void lcdManagerShowStatus(const char *message);
void lcdManagerNextMenuPage(void);
void lcdManagerShowClock(void);
bool lcdManagerIsClockScreen(void);
void lcdManagerShowApMode(uint32_t remainingMs);
void lcdManagerUpdateApRemaining(uint32_t remainingMs);
bool lcdManagerIsApModeScreen(void);
void lcdManagerShowHome(void);
#endif

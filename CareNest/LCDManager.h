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
#endif

#ifndef LCD_MANAGER_H
#define LCD_MANAGER_H

#include <Arduino.h>

void lcdManagerBegin(void);
void lcdManagerUpdate(unsigned int feedCount,
                      unsigned int diaperCount,
                      unsigned long lastFeedMs,
                      unsigned long lastDiaperMs);
void lcdManagerShowStatus(const char *message);
void lcdManagerNextMenuPage(void);

#endif

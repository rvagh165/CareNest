#ifndef SDMANAGER_H
#define SDMANAGER_H

#include <Arduino.h>

bool sdBegin(void);
void sdEnd(void);

// Log an event (e.g. "Feed", "Diaper") with current RTC time.
// Returns true on success.
bool sdLogEvent(const char *action);

#endif // SDMANAGER_H

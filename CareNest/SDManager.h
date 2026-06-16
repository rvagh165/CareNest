#ifndef SDMANAGER_H
#define SDMANAGER_H

#include <Arduino.h>

bool sdBegin(void);
void sdEnd(void);

// Log an event (e.g. "Feed", "Diaper") with current RTC time.
// Returns true on success.
bool sdLogEvent(const char *action);

// If sdLogEvent fails, use this to retrieve a short error message (persistent until next call)
const char* sdGetLastError(void);

#endif // SDMANAGER_H

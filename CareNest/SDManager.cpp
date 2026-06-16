#include "SDManager.h"
#include "config.h"
#include "RTC.h"

#include <Arduino.h>
#include "FS.h"
#include "SD_MMC.h"

static bool isInitialized = false;
static char lastErrorMsg[128] = "";

static String makeFilenameForToday()
{
    DateTime dt = rtcGetTime();
    char buf[64];
    // Put files under /CareNest directory
    snprintf(buf, sizeof(buf), "/CareNest/%04u-%02u-%02u.csv", dt.year(), dt.month(), dt.day());
    return String(buf);
}

static String makeCsvLine(const char *action)
{
    DateTime dt = rtcGetTime();
    char buf[128];
    // CSV line: date,time,action with trailing newline
    snprintf(buf, sizeof(buf), "%04u-%02u-%02u,%02u:%02u:%02u,%s\n",
             dt.year(), dt.month(), dt.day(), dt.hour(), dt.minute(), dt.second(), action);
    return String(buf);
}

// Ensure a directory exists, create if missing
static bool ensureDirectoryExists(const char *dir)
{
    if (SD_MMC.exists(dir)) return true;
    if (SD_MMC.mkdir(dir)) {
        Serial.print(F("[SD] Created directory: "));
        Serial.println(dir);
        return true;
    }
    Serial.print(F("[SD] Failed to create directory: "));
    Serial.println(dir);
    return false;
}

bool sdBegin(void)
{
    if (isInitialized) return true;

    // Try to set pins for SD_MMC if provided in config. This config call is optional
    // and may fail on some ESP32 variants; if setPins returns false we abort.
#ifdef SD_CLK_PIN
    if (!SD_MMC.setPins(SD_CLK_PIN, SD_CMD_PIN, SD_D0_PIN)) {
        Serial.println(F("[SD] SD_MMC.setPins failed"));
        // Don't return here — some platforms don't require setPins; try begin anyway.
    }
#endif

    // Mount using SD_MMC in 1-bit mode (SD_MMC.begin with esp32 mounts SD in SDMMC host)
    if (!SD_MMC.begin("/sdcard", true)) {
        Serial.println(F("[SD] Card Mount Failed (SD_MMC.begin)"));
        isInitialized = false;
        return false;
    }

    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
        Serial.println(F("[SD] No SD card attached"));
        isInitialized = false;
        return false;
    }

    // Prefer SD/SDHC; reject MMC if you don't want it
    if (cardType == CARD_MMC) {
        Serial.println(F("[SD] MMC card detected — unsupported. Use SD/SDHC in SDMMC (1-bit) mode."));
        isInitialized = false;
        SD_MMC.end();
        return false;
    }

    Serial.print(F("[SD] Card Type: "));
    if (cardType == CARD_SD) Serial.println(F("SDSC"));
    else if (cardType == CARD_SDHC) Serial.println(F("SDHC"));
    else Serial.println(F("UNKNOWN"));

    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    Serial.print(F("[SD] Card Size: "));
    Serial.print(cardSize);
    Serial.println(F(" MB"));

    isInitialized = true;
    return true;
}

void sdEnd(void)
{
    // Allow sdEnd to forcibly end mount if active
    if (isInitialized) {
        SD_MMC.end();
        isInitialized = false;
    }
}

const char* sdGetLastError(void)
{
    return lastErrorMsg[0] ? lastErrorMsg : nullptr;
}

bool sdLogEvent(const char *action)
{
    // Clear last error
    lastErrorMsg[0] = '\0';

    // Mount per-operation to minimize risk of corruption if card removed
#ifdef SD_CLK_PIN
    if (!SD_MMC.setPins(SD_CLK_PIN, SD_CMD_PIN, SD_D0_PIN)) {
        // not fatal; log for debug
        Serial.println(F("[SD] SD_MMC.setPins failed (non-fatal)"));
    }
#endif

    if (!SD_MMC.begin("/sdcard", true)) {
        Serial.println(F("[SD] Card Mount Failed (sdLogEvent)"));
        strncpy(lastErrorMsg, "SD mount failed", sizeof(lastErrorMsg)-1);
        SD_MMC.end();
        return false;
    }

    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
        Serial.println(F("[SD] No SD card attached (sdLogEvent)"));
        strncpy(lastErrorMsg, "No SD card", sizeof(lastErrorMsg)-1);
        SD_MMC.end();
        return false;
    }

    // Ensure directory
    if (!ensureDirectoryExists("/CareNest")) {
        strncpy(lastErrorMsg, "Failed create dir", sizeof(lastErrorMsg)-1);
        SD_MMC.end();
        return false;
    }

    String filepath = makeFilenameForToday();
    String line = makeCsvLine(action);

    // If file doesn't exist yet, create it and write CSV header
    if (!SD_MMC.exists(filepath.c_str())) {
        File f = SD_MMC.open(filepath.c_str(), FILE_WRITE);
        if (!f) {
            Serial.print(F("[SD] Failed to create file: "));
            Serial.println(filepath);
            strncpy(lastErrorMsg, "Create file failed", sizeof(lastErrorMsg)-1);
            SD_MMC.end();
            return false;
        }
        f.println("date,time,action");
        f.close();
        Serial.print(F("[SD] Created new log file: "));
        Serial.println(filepath);
    }

    // Append the line
    File file = SD_MMC.open(filepath.c_str(), FILE_APPEND);
    if (!file) {
        // Try creating/truncating and then append as fallback
        Serial.print(F("[SD] Failed to open file for append: "));
        Serial.println(filepath);

        File f2 = SD_MMC.open(filepath.c_str(), FILE_WRITE);
        if (!f2) {
            Serial.print(F("[SD] Fallback open failed: "));
            Serial.println(filepath);
            strncpy(lastErrorMsg, "Open file failed", sizeof(lastErrorMsg)-1);
            SD_MMC.end();
            return false;
        }
        // write header then the line
        f2.println("date,time,action");
        if (f2.print(line) == 0) {
            Serial.println(F("[SD] Fallback write failed"));
            strncpy(lastErrorMsg, "Write failed", sizeof(lastErrorMsg)-1);
            f2.close();
            SD_MMC.end();
            return false;
        }
        f2.close();

        Serial.print(F("[SD] Wrote via fallback to: "));
        Serial.println(filepath);
        SD_MMC.end();
        return true;
    }

    size_t written = file.print(line);
    file.flush();
    file.close();

    if (written == 0) {
        Serial.println(F("[SD] Write failed"));
        strncpy(lastErrorMsg, "Write failed", sizeof(lastErrorMsg)-1);
        SD_MMC.end();
        return false;
    }

    Serial.print(F("[SD] Logged: "));
    Serial.println(line);

    // Unmount the card after successful write to reduce corruption risk
    SD_MMC.end();

    return true;
}

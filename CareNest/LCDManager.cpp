#include "LCDManager.h"
#include "config.h"
#include "DailyTracker.h"
#include "StartupAnimation.h"
#include "RTC.h"
#include "sleeping_animation.h" 
#include "Battery.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

typedef enum {
    LCD_SCREEN_SPLASH = 0,
    LCD_SCREEN_HOME,
    LCD_SCREEN_TIMERS,
    LCD_SCREEN_CLOCK,
    LCD_SCREEN_AP_MODE,
    LCD_SCREEN_STATUS
} LcdScreen;

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);

static LcdScreen currentScreen = LCD_SCREEN_SPLASH;
static LcdScreen returnScreen = LCD_SCREEN_HOME;
static unsigned long screenStartedMs = 0;
static unsigned long lastRefreshMs = 0;
static uint8_t startupFrameIndex = 0;
static const char *statusMessage = "";
static uint32_t apRemainingMs = 0;
static bool displayReady = false;

static unsigned int cachedFeedCount = 0;
static unsigned int cachedDiaperCount = 0;
static uint32_t cachedLastFeedEpoch = 0;
static uint32_t cachedLastDiaperEpoch = 0;

/*
 * Print elapsed wall-clock time since `epochSec` (Unix epoch seconds).
 * Uses rtcGetEpoch() so the display is accurate even across reboots.
 */
static void printElapsedEpoch(uint32_t epochSec)
{
    uint32_t nowEpoch = rtcGetEpoch();
    uint32_t seconds;
    uint32_t minutes;

    if (epochSec == 0 || nowEpoch < epochSec) {
        display.print("Never");
        return;
    }

    seconds = nowEpoch - epochSec;
    minutes = seconds / 60UL;

    if (minutes < 1UL) {
        display.print(seconds);
        display.print("s ago");
    } else if (minutes < 60UL) {
        display.print(minutes);
        display.print("m ago");
    } else {
        display.print(minutes / 60UL);
        display.print("h ago");
    }
}

static void drawSplash(void)
{
    // const int16_t bitmapX = (SCREEN_WIDTH - STARTUP_ANIMATION_WIDTH) / 2;
    // const int16_t bitmapY = 12 + (startupFrameIndex % 4 == 1 ? -1 : 0);
    const int16_t pulseX = 34 + (startupFrameIndex * 8);

    display.clearDisplay();
    display.drawBitmap(0,
                       0,
                       startupAnimationFrame(startupFrameIndex),
                       STARTUP_ANIMATION_WIDTH,
                       STARTUP_ANIMATION_HEIGHT,
                       SSD1306_WHITE);
    // display.drawLine(34, 52, 94, 52, SSD1306_WHITE);
    // display.fillCircle(pulseX, 52, 2, SSD1306_WHITE);
    display.display();
}
// ── Battery icon: rounded-rect body + nub, with a fill bar for charge level ──
static void drawBatteryIcon(int x, int y, int w, int h, uint8_t percent, bool charging)
{
    if (percent > 100) percent = 100;

    // Terminal nub on top
    int nubW = 8;
    int nubH = 3;
    int nubX = x + (w - nubW) / 2;
    int nubY = y - nubH;
    display.fillRect(nubX, nubY, nubW, nubH, SSD1306_WHITE);

    // Outer body (rounded rect outline) — slimmer now
    display.drawRoundRect(x, y, w, h, 3, SSD1306_WHITE);

    // Inner area
    const int pad = 2;
    int innerX = x + pad;
    int innerY = y + pad;
    int innerW = w - (pad * 2);
    int innerH = h - (pad * 2);

if (charging) {
    // ── Lightning bolt made of two triangles sharing a vertex ──
    int bw = (int)(innerW * 0.60f);
    int bh = (int)(innerH * 0.65f);
    int boltX = innerX + (innerW - bw) / 2;
    int boltY = innerY + (innerH - bh) / 2;

    // Bolt points — top wedge larger, bottom wedge trimmed for balance
    int p0x = boltX + bw * 0.62f;  int p0y = boltY;                  // top tip
    int p1x = boltX;               int p1y = boltY + bh * 0.50f;     // left notch
    int p2x = boltX + bw * 0.42f;  int p2y = boltY + bh * 0.52f;     // center waist (shared)
    int p3x = boltX + bw * 0.40f;  int p3y = boltY + bh * 0.88f;     // bottom tip (pulled up/in)
    int p4x = boltX + bw * 0.90f;  int p4y = boltY + bh * 0.50f;     // right notch (pulled in)

    // Upper wedge
    display.fillTriangle(p0x, p0y, p1x, p1y, p2x, p2y, SSD1306_WHITE);
    // Lower wedge — shares vertex p2, so they touch exactly with no gap
    display.fillTriangle(p2x, p2y, p3x, p3y, p4x, p4y, SSD1306_WHITE);

    return;
}

    // Normal fill representing charge %, anchored to the bottom
    int fillH = (innerH * percent) / 100;
    int fillY = innerY + (innerH - fillH);

    if (fillH > 1) {
        display.fillRoundRect(innerX, fillY, innerW, fillH, 2, SSD1306_WHITE);
    } else if (percent > 0) {
        // sliver for very low charge so it's never totally invisible
        display.drawFastHLine(innerX, innerY + innerH - 1, innerW, SSD1306_WHITE);
    }
}

// ── Battery widget: icon on top, % text centered below it ──
static void drawBatteryWidget(void)
{
    float   vBatt = getBatteryVoltage();
    uint8_t pct   = batteryPercentage(vBatt);
    bool    charging = (vBatt >= MAX_VOLTAGE);

    const int battX = 99;   // centered in the empty gap right of the cards
    const int battY = 17;
    const int battW = 18;   // thinner body
    const int battH = 24;

    drawBatteryIcon(battX, battY, battW, battH, pct, charging);

    char buf[6];
    if (!charging) {
        snprintf(buf, sizeof(buf), "%u%%", pct);
    }
    else {
        snprintf(buf, sizeof(buf), "   ");
    }

    int16_t  x1, y1;
    uint16_t tw, th;
    display.getTextBounds(buf, 0, 0, &x1, &y1, &tw, &th);
    int textX = battX + (battW - tw) / 2;
    display.setCursor(textX, battY + battH + 3);
    display.print(buf);
}

static void drawHome(void)
{
    display.clearDisplay();
    display.setRotation(0);          // native landscape: 128×64
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);          // 6×8px per char at size 1

// ── HEADER ──────────────────────────────── y = 0..11

// Left heart (two small circles + triangle = classic pixel heart)
display.fillCircle(4, 3, 2, SSD1306_WHITE);
display.fillCircle(8, 3, 2, SSD1306_WHITE);
display.fillTriangle(2, 4, 10, 4, 6, 8, SSD1306_WHITE);

// Title
display.setCursor(18, 2);
display.print("    CareNest ");

// Right heart
display.fillCircle(116, 3, 2, SSD1306_WHITE);
display.fillCircle(120, 3, 2, SSD1306_WHITE);
display.fillTriangle(114, 4, 122, 4, 118, 8, SSD1306_WHITE);

display.drawLine(0, 11, 127, 11, SSD1306_WHITE);

    // ── FEEDS CARD ─────────────────────── y = 14..32  (h=18)
    display.drawRoundRect(2, 14, 86, 18, 3, SSD1306_WHITE);

    // Bottle icon  x=7, y=17  (7w × 13h fits inside card)
    display.drawRect(7, 20, 8, 9, SSD1306_WHITE);   // body
    display.drawRect(9, 17, 4, 3, SSD1306_WHITE);   // neck
    display.drawLine(8, 23, 14, 23, SSD1306_WHITE); // milk line

    // Label
    display.setCursor(22, 18);
    display.print("Feeds ");

    // Count badge (filled, dark text)
    display.fillRoundRect(65, 16, 19, 14, 3, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(70 , 20);
    display.print(cachedFeedCount);
    display.setTextColor(SSD1306_WHITE);

    // ── DIAPER CARD ────────────────────── y = 35..52  (h=18)
    display.drawRoundRect(2, 35, 86, 18, 3, SSD1306_WHITE);

    // Diaper icon x=7, y=38
    display.drawRect(7, 38, 12, 8, SSD1306_WHITE);
    display.drawLine(7,  46, 10, 50, SSD1306_WHITE);
    display.drawLine(19, 46, 16, 50, SSD1306_WHITE);

    // Label
    display.setCursor(22, 39);
    display.print("Diaper ");

    // Count badge
    display.fillRoundRect(65, 37, 19, 14, 3, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(70, 41);
    display.print(cachedDiaperCount);
    display.setTextColor(SSD1306_WHITE);

    // ── FOOTER BUTTONS ─────────────────── y = 55..63  (h=9)
    // display.drawLine(0, 54, 127, 54, SSD1306_WHITE);

    // TIME button (outline)
    display.drawRoundRect(2, 55, 58, 9, 2, SSD1306_WHITE);
    // clock icon
    display.drawCircle(9, 59, 3, SSD1306_WHITE);
    display.drawLine(9, 59, 9, 57, SSD1306_WHITE);
    display.drawLine(9, 59, 11, 60, SSD1306_WHITE);
    display.setCursor(16, 56);
    display.print("Time");

    // MENU button (filled)
    display.fillRoundRect(68, 55, 58, 9, 2, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    // hamburger icon
    display.drawLine(73, 57, 79, 57, SSD1306_BLACK);
    display.drawLine(73, 59, 79, 59, SSD1306_BLACK);
    display.drawLine(73, 61, 79, 61, SSD1306_BLACK);
    display.setCursor(84, 56);
    display.print("Menu");
    display.setTextColor(SSD1306_WHITE);

    drawBatteryWidget();

    display.display();
}


// ─── drawTimers ─────────────────────────────────────────────────
static void drawTimers(void)
{
    display.clearDisplay();
    display.setRotation(0);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    // Header
    display.setCursor(2, 1);
    display.print("Last Activity");
    display.drawLine(0, 11, 127, 11, SSD1306_WHITE);

    // ── Feed row ─── y=14..30
    display.drawRoundRect(2, 14, 124, 18, 3, SSD1306_WHITE);
    // Bottle icon (small)
    display.drawRect(6, 18, 7, 9, SSD1306_WHITE);
    display.drawRect(8, 16, 3, 2, SSD1306_WHITE);

    display.setCursor(20, 15);
    display.print("Last Feed");
    display.setCursor(20, 24);
    printElapsedEpoch(cachedLastFeedEpoch);   // prints "2h 14m ago" etc.

    // ── Diaper row ─── y=34..50
    display.drawRoundRect(2, 34, 124, 18, 3, SSD1306_WHITE);
    display.drawRect(6, 38, 10, 7, SSD1306_WHITE);
    display.drawLine(6,  45, 9,  49, SSD1306_WHITE);
    display.drawLine(16, 45, 13, 49, SSD1306_WHITE);

    display.setCursor(20, 35);
    display.print("Last Diaper");
    display.setCursor(20, 44);
    printElapsedEpoch(cachedLastDiaperEpoch);

    // Back hint
    display.setCursor(18, 56);
    display.print("< back to home >");

    display.display();
}


// ─── drawClock ──────────────────────────────────────────────────
static void drawClock(void)
{
    DateTime now = rtcGetTime();

    display.clearDisplay();
    display.setRotation(0);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    // Header
    display.setCursor(2, 1);
    display.print("Current Time");
    display.drawLine(0, 11, 127, 11, SSD1306_WHITE);

    // Date  DD / MM / YYYY  at y=18
    display.setCursor(22, 18);
    if (now.day()   < 10) display.print('0');
    display.print(now.day());
    display.print(" / ");
    if (now.month() < 10) display.print('0');
    display.print(now.month());
    display.print(" / ");
    display.print(now.year());

    // Time  HH:MM:SS  at y=32 with textSize 2 (12×16px per char)
    display.setTextSize(2);
    display.setCursor(17, 32);
    if (now.hour()   < 10) display.print('0');
    display.print(now.hour());
    display.print(':');
    if (now.minute() < 10) display.print('0');
    display.print(now.minute());
    display.print(':');
    if (now.second() < 10) display.print('0');
    display.print(now.second());

    // Footer buttons (like Home screen)
    display.setTextSize(1);

    // HOME button (outline)
    display.drawRoundRect(2, 55, 58, 9, 2, SSD1306_WHITE);
    // simple house icon
    display.drawTriangle(9, 60, 13, 56, 17, 60, SSD1306_WHITE);
    display.drawRect(11, 60, 5, 4, SSD1306_WHITE);
    display.setCursor(20, 56);
    display.print("Home");

    // SYNC button (filled) - Menu triggers sync/AP mode
    display.fillRoundRect(68, 55, 58, 9, 2, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    // sync icon
    display.drawCircle(75, 59, 3, SSD1306_BLACK);
    display.drawLine(75, 56, 77, 58, SSD1306_BLACK);
    display.drawLine(75, 56, 73, 58, SSD1306_BLACK);
    display.setCursor(84, 56);
    display.print("Sync");
    display.setTextColor(SSD1306_WHITE);

    display.display();
}

static void drawApMode(void)
{
    uint32_t remaining = apRemainingMs;
    uint32_t seconds = remaining / 1000UL;
    uint32_t mm = seconds / 60UL;
    uint32_t ss = seconds % 60UL;

    display.clearDisplay();
    display.setRotation(0);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    display.setCursor(2, 1);
    display.print("CareNest - AP Mode");
    display.drawLine(0, 11, 127, 11, SSD1306_WHITE);

    display.setCursor(2, 18);
    display.print("WiFi: CareNest");

    display.setCursor(2, 30);
    display.print("Open: 192.168.4.1");

    display.setCursor(2, 42);
    display.print("Select TZ + Set Time");

    display.setCursor(2, 54);
    display.print("Timeout: ");
    if (mm < 10) display.print('0');
    display.print(mm);
    display.print(':');
    if (ss < 10) display.print('0');
    display.print(ss);

    display.display();
}
// ── Helper: draw animated progress bar (call once per lcdManagerUpdate tick)
// barProgress: 0–120 (pixels filled), drawn at y=60

static void drawStatusProgressBar(uint8_t filledPx)
{
    display.drawLine(4, 60, 124, 60, SSD1306_WHITE);   // track
    if (filledPx > 0) {
        display.drawLine(4, 61, 4 + filledPx, 61, SSD1306_WHITE); // fill
        display.drawLine(4, 62, 4 + filledPx, 62, SSD1306_WHITE);
    }
}

// ── Context icon helpers (small, fits inside status card area) ──

static void drawIconBottle(uint8_t x, uint8_t y)
{
    display.drawRect(x,     y + 3, 10, 11, SSD1306_WHITE); // body
    display.drawRect(x + 3, y,      4,  4, SSD1306_WHITE); // neck
    display.drawLine(x, y + 7, x + 9, y + 7, SSD1306_WHITE); // milk line
}

static void drawIconDiaper(uint8_t x, uint8_t y)
{
    display.drawRect(x,     y,      14,  9, SSD1306_WHITE);
    display.drawLine(x,     y + 9,  x + 4, y + 13, SSD1306_WHITE);
    display.drawLine(x + 14,y + 9,  x + 10,y + 13, SSD1306_WHITE);
}

static void drawIconClock(uint8_t cx, uint8_t cy, uint8_t r)
{
    display.drawCircle(cx, cy, r, SSD1306_WHITE);
    display.drawLine(cx, cy, cx,     cy - r + 2, SSD1306_WHITE); // hour
    display.drawLine(cx, cy, cx + r - 2, cy + 1, SSD1306_WHITE); // minute
}

static void drawCheckmark(uint8_t cx, uint8_t cy, uint8_t r)
{
    display.drawCircle(cx, cy, r, SSD1306_WHITE);
    // tick: bottom-left leg then up-right leg
    display.drawLine(cx - r/2,     cy,          cx - r/5, cy + r/2,   SSD1306_WHITE);
    display.drawLine(cx - r/5,     cy + r/2,    cx + r/2, cy - r/3,   SSD1306_WHITE);
}

// ── Main drawStatus ─────────────────────────────────────────────

static void drawStatus(void)
{
    display.clearDisplay();
    display.setRotation(0);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);

    // ── Header (same style as other screens)
    display.setCursor(2, 1);
    display.print("CareNest");
    display.drawLine(0, 11, 127, 11, SSD1306_WHITE);

    // ── Detect which status message we have ──────────────────────
    bool isFeed   = (strcmp(statusMessage, "Feed ")   == 0);
    bool isDiaper = (strcmp(statusMessage, "Diaper ") == 0);
    bool isSync   = (strcmp(statusMessage, "Time ") == 0);

    // ── Context icon on the left (x=10..23, y=16..32) ───────────
    if (isFeed) {
        drawIconBottle(10, 16);
    } else if (isDiaper) {
        drawIconDiaper(10, 17);
    } else if (isSync) {
        drawIconClock(15, 26, 7);
    } else {
        // generic star / diamond for "Selected"
        display.drawLine(16, 16, 16, 36, SSD1306_WHITE);
        display.drawLine(8,  26, 24, 26, SSD1306_WHITE);
        display.drawLine(10, 18, 22, 34, SSD1306_WHITE);
        display.drawLine(22, 18, 10, 34, SSD1306_WHITE);
    }

    // ── Message text (right of icon) ────────────────────────────
    display.setCursor(36, 18);
    display.print(statusMessage);

    // Sub-caption
    display.setCursor(36, 28);
    if (isFeed)        display.print("Logged OK");
    else if (isDiaper) display.print("Logged OK");
    else if (isSync)   display.print("Sync");
    else               display.print("Confirmed");

    // ── Checkmark circle (top-right corner) ─────────────────────
    drawCheckmark(110, 24, 10);

    // ── Divider ──────────────────────────────────────────────────
    display.drawLine(4, 44, 124, 44, SSD1306_WHITE);

    // ── Return hint ──────────────────────────────────────────────
    display.setCursor(14, 48);
    display.print("returning home...");

    // ── Progress bar (animated — driven by lcdManagerUpdate) ─────
    // Compute how far through the STATUS_DURATION we are
    uint32_t elapsed = millis() - screenStartedMs;
    uint32_t total   = STATUS_DISPLAY_MS;          // e.g. 2000
    if (elapsed > total) elapsed = total;
    uint8_t filled = (uint8_t)(120UL * (total - elapsed) / total); // counts DOWN
    drawStatusProgressBar(filled);

    display.display();
}
static void drawCurrentScreen(void)
{
    if (!displayReady) {
        return;
    }

    if (currentScreen == LCD_SCREEN_SPLASH) {
        drawSplash();
    } else if (currentScreen == LCD_SCREEN_HOME) {
        drawHome();
    } else if (currentScreen == LCD_SCREEN_TIMERS) {
        drawTimers();
    } else if (currentScreen == LCD_SCREEN_CLOCK) {
        drawClock();
    } else if (currentScreen == LCD_SCREEN_AP_MODE) {
        drawApMode();
    } else {
        drawStatus();
    }
}

void lcdManagerBegin(void)
{

  pinMode(OLED_DC_PIN, OUTPUT);
  digitalWrite(OLED_DC_PIN, LOW);
  
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);
    }
    display.setRotation(0);
    display.clearDisplay();
    display.display();
    displayReady = true;
}

void lcdManagerUpdate(unsigned int feedCount,
                      unsigned int diaperCount,
                      uint32_t lastFeedEpoch,
                      uint32_t lastDiaperEpoch)
{
    unsigned long now = millis();
    bool dataChanged = false;

    if (cachedFeedCount != feedCount || cachedDiaperCount != diaperCount ||
        cachedLastFeedEpoch != lastFeedEpoch || cachedLastDiaperEpoch != lastDiaperEpoch) {
        cachedFeedCount = feedCount;
        cachedDiaperCount = diaperCount;
        cachedLastFeedEpoch = lastFeedEpoch;
        cachedLastDiaperEpoch = lastDiaperEpoch;
        dataChanged = true;
    }

    if (currentScreen == LCD_SCREEN_SPLASH &&
        (now - screenStartedMs) >= SPLASH_DURATION_MS) {
        currentScreen = LCD_SCREEN_HOME;
        screenStartedMs = now;
        drawCurrentScreen();
        return;
    }

    if (currentScreen == LCD_SCREEN_SPLASH &&
        (now - lastRefreshMs) >= STARTUP_ANIMATION_FRAME_INTERVAL_MS) {
        lastRefreshMs = now;
        startupFrameIndex = (startupFrameIndex + 1) % STARTUP_ANIMATION_FRAME_COUNT;
        drawSplash();
        return;
    }

    if (currentScreen == LCD_SCREEN_CLOCK &&
        (now - lastRefreshMs) >= CLOCK_REFRESH_MS) {
        lastRefreshMs = now;
        drawClock();
        return;
    }

    if (currentScreen == LCD_SCREEN_AP_MODE &&
        (now - lastRefreshMs) >= 1000UL) {
        lastRefreshMs = now;
        drawApMode();
        return;
    }

    if (currentScreen == LCD_SCREEN_STATUS &&
        (now - screenStartedMs) >= STATUS_DURATION_MS) {
        currentScreen = returnScreen;
        screenStartedMs = now;
        drawCurrentScreen();
        return;
    }

    if (dataChanged || (now - lastRefreshMs) >= SCREEN_REFRESH_MS) {
        lastRefreshMs = now;

        drawCurrentScreen();
    }
}

void lcdManagerShowStatus(const char *message)
{
    statusMessage = message;
    currentScreen = LCD_SCREEN_STATUS;
    screenStartedMs = millis();

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 20);
    display.print(message);
    display.display();
}

void lcdManagerNextMenuPage(void)
{
    if (currentScreen == LCD_SCREEN_HOME) {
        currentScreen = LCD_SCREEN_TIMERS;
    }
    else 
    {
        currentScreen = LCD_SCREEN_HOME;
    }

    returnScreen = currentScreen;
    screenStartedMs = millis();
    drawCurrentScreen();
}

void lcdManagerShowClock(void)
{
    if(currentScreen == LCD_SCREEN_HOME)
    {
    currentScreen = LCD_SCREEN_CLOCK;
    drawCurrentScreen();
    }
    else 
    {
        currentScreen = LCD_SCREEN_HOME;
    }
    
    returnScreen = currentScreen;
    screenStartedMs = millis();
    drawCurrentScreen();


}

bool lcdManagerIsClockScreen(void)
{
    return currentScreen == LCD_SCREEN_CLOCK;
}

void lcdManagerShowApMode(uint32_t remainingMs)
{
    apRemainingMs = remainingMs;
    currentScreen = LCD_SCREEN_AP_MODE;
    screenStartedMs = millis();
    lastRefreshMs = 0;
    drawCurrentScreen();
}

void lcdManagerUpdateApRemaining(uint32_t remainingMs)
{
    apRemainingMs = remainingMs;
}

bool lcdManagerIsApModeScreen(void)
{
    return currentScreen == LCD_SCREEN_AP_MODE;
}

void lcdManagerShowHome(void)
{
    currentScreen = LCD_SCREEN_HOME;
    returnScreen = currentScreen;
    screenStartedMs = millis();
    drawCurrentScreen();
}

void lcdManagerShowSleepAnimation(uint32_t durationMs, uint16_t frameDelayMs)
{
    if (!displayReady) return;

    const uint8_t* frames[] = {
        frame0, frame1, frame2, frame3, frame4, frame5, frame6, frame7,
        frame8, frame9, frame10, frame11, frame12, frame13, frame14, frame15,
        frame16, frame17, frame18, frame19, frame20, frame21, frame22, frame23,
        frame24, frame25, frame26, frame27
    };
    const uint8_t numFrames = sizeof(frames) / sizeof(frames[0]);

    uint32_t start = millis();
    uint8_t idx = 0;
    while ((millis() - start) < durationMs) {
        display.clearDisplay();
        display.drawBitmap(0, 0, frames[idx], SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
        display.display();
        idx = (idx + 1) % numFrames;
        delay(frameDelayMs);
    }
}

void lcdManagerEnd(void)
{
    if (!displayReady) {
        return;
    }

    display.clearDisplay();
    display.display();

    // Tell the OLED controller to power down before releasing the bus.
    Wire.beginTransmission(OLED_ADDRESS);
    Wire.write(0x00);
    Wire.write(0xAE); // SSD1306 display off
    Wire.endTransmission();

    displayReady = false;
    Wire.end();

    pinMode(OLED_DC_PIN, INPUT);
    pinMode(OLED_RESET_PIN, INPUT);
    pinMode(OLED_SDA_PIN, INPUT);
    pinMode(OLED_SCL_PIN, INPUT);
}

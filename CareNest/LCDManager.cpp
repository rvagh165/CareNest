#include "LCDManager.h"
#include "config.h"
#include "StartupAnimation.h"
#include "RTC.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

typedef enum {
    LCD_SCREEN_SPLASH = 0,
    LCD_SCREEN_HOME,
    LCD_SCREEN_TIMERS,
    LCD_SCREEN_CLOCK,
    LCD_SCREEN_STATUS
} LcdScreen;

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);

static LcdScreen currentScreen = LCD_SCREEN_SPLASH;
static LcdScreen returnScreen = LCD_SCREEN_HOME;
static unsigned long screenStartedMs = 0;
static unsigned long lastRefreshMs = 0;
static uint8_t startupFrameIndex = 0;
static const char *statusMessage = "";
static bool displayReady = false;

static unsigned int cachedFeedCount = 0;
static unsigned int cachedDiaperCount = 0;
static unsigned long cachedLastFeedMs = 0;
static unsigned long cachedLastDiaperMs = 0;

static void printElapsed(unsigned long eventMs)
{
    unsigned long seconds;
    unsigned long minutes;

    if (eventMs == 0) {
        display.print("Never");
        return;
    }

    seconds = (millis() - eventMs) / 1000UL;
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
    const int16_t bitmapX = (SCREEN_WIDTH - STARTUP_ANIMATION_WIDTH) / 2;
    const int16_t bitmapY = 12 + (startupFrameIndex % 4 == 1 ? -1 : 0);
    const int16_t pulseX = 34 + (startupFrameIndex * 8);

    display.clearDisplay();
    display.drawBitmap(bitmapX,
                       bitmapY,
                       startupAnimationFrame(startupFrameIndex),
                       STARTUP_ANIMATION_WIDTH,
                       STARTUP_ANIMATION_HEIGHT,
                       SSD1306_WHITE);
    display.drawLine(34, 52, 94, 52, SSD1306_WHITE);
    display.fillCircle(pulseX, 52, 2, SSD1306_WHITE);
    display.display();
}

static void drawHome(void)
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("CareNest Baby Care");
    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    display.setCursor(0, 18);
    display.print("Feeds today: ");
    display.print(cachedFeedCount);

    display.setCursor(0, 30);
    display.print("Diapers: ");
    display.print(cachedDiaperCount);

    display.setCursor(0, 44);
    display.print("Menu: next page");

    display.setCursor(0, 56);
    display.print("Select: confirm");
    display.display();
}

static void drawTimers(void)
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Last Activity");
    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    display.setCursor(0, 20);
    display.print("Feed: ");
    printElapsed(cachedLastFeedMs);

    display.setCursor(0, 34);
    display.print("Diaper: ");
    printElapsed(cachedLastDiaperMs);

    display.setCursor(0, 52);
    display.print("Baby milk tracker");
    display.display();
}

static void drawClock(void)
{
    DateTime now = rtcGetTime();

    display.clearDisplay();
    display.setTextSize(1);

    /* Header */
    display.setCursor(0, 0);
    display.print("Current Time");
    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

    /* Date line - DD/MM/YYYY */
    display.setCursor(0, 18);
    display.setTextSize(1);
    if (now.day() < 10) display.print('0');
    display.print(now.day());
    display.print('/');
    if (now.month() < 10) display.print('0');
    display.print(now.month());
    display.print('/');
    display.print(now.year());

    /* Time line - HH:MM:SS  (larger text) */
    display.setCursor(10, 38);
    display.setTextSize(2);
    if (now.hour() < 10) display.print('0');
    display.print(now.hour());
    display.print(':');
    if (now.minute() < 10) display.print('0');
    display.print(now.minute());
    display.print(':');
    if (now.second() < 10) display.print('0');
    display.print(now.second());

    display.display();
}

static void drawStatus(void)
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("CareNest");
    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
    display.setCursor(0, 28);
    display.print(statusMessage);
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
        Serial.println("OLED init failed");
        displayReady = false;
        return;
    }

    displayReady = true;
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    currentScreen = LCD_SCREEN_SPLASH;
    screenStartedMs = millis();
    lastRefreshMs = 0;
    startupFrameIndex = 0;
    drawCurrentScreen();
}

void lcdManagerUpdate(unsigned int feedCount,
                      unsigned int diaperCount,
                      unsigned long lastFeedMs,
                      unsigned long lastDiaperMs)
{
    unsigned long now = millis();
    bool dataChanged = false;

    if (cachedFeedCount != feedCount || cachedDiaperCount != diaperCount ||
        cachedLastFeedMs != lastFeedMs || cachedLastDiaperMs != lastDiaperMs) {
        cachedFeedCount = feedCount;
        cachedDiaperCount = diaperCount;
        cachedLastFeedMs = lastFeedMs;
        cachedLastDiaperMs = lastDiaperMs;
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
    if (currentScreen != LCD_SCREEN_STATUS) {
        returnScreen = currentScreen;
    }

    statusMessage = message;
    currentScreen = LCD_SCREEN_STATUS;
    screenStartedMs = millis();
    drawCurrentScreen();
}

void lcdManagerNextMenuPage(void)
{
    if (currentScreen == LCD_SCREEN_HOME) {
        currentScreen = LCD_SCREEN_TIMERS;
    } else if (currentScreen == LCD_SCREEN_TIMERS) {
        currentScreen = LCD_SCREEN_CLOCK;
    } else {
        currentScreen = LCD_SCREEN_HOME;
    }

    returnScreen = currentScreen;
    screenStartedMs = millis();
    drawCurrentScreen();
}


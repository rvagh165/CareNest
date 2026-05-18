#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "esp_sleep.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_SDA 8
#define OLED_SCL 9
#define OLED_DC 18

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
RTC_PCF8523 rtc;   // replace if using another library

//----------------------------------
// Button Pins
//----------------------------------
#define FEED_BUTTON      4
#define DIAPER_BUTTON    5
#define MENU_BUTTON      6
#define SELECT_BUTTON    7

//----------------------------------
// SD Card
//----------------------------------
#define SD_CS 10

//----------------------------------
// Sleep timeout
//----------------------------------
unsigned long lastActivity = 0;
const unsigned long sleepTimeout = 60000;   // 1 minute

String lastFeedTime = "None";
String lastDiaperTime = "None";

void initOLED()
{
    pinMode(OLED_DC, OUTPUT);

    // Default command mode during init
    digitalWrite(OLED_DC, LOW);

    Wire.begin(OLED_SDA, OLED_SCL);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println("OLED init failed");
        // while(1);
    }


    

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

    display.setCursor(0,0);
    display.println("OLED Ready");
    display.display();

    Serial.println("OLED Initialized");
}


//----------------------------------
// Display Home Screen
//----------------------------------
void showHomeScreen()
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,0);

    display.println("Baby Health Monitor");
    display.println("------------------");
    display.print("Last Feed: ");
    display.println(lastFeedTime);

    display.print("Last Diaper: ");
    display.println(lastDiaperTime);

    display.display();
}


//----------------------------------
// Log Event to CSV
//----------------------------------
void logEvent(String eventType)
{
    DateTime now = rtc.now();

    String timestamp =
        String(now.year()) + "-" +
        String(now.month()) + "-" +
        String(now.day()) + " " +
        String(now.hour()) + ":" +
        String(now.minute()) + ":" +
        String(now.second());

    File file = SD.open("/baby_log.csv", FILE_APPEND);

    if(file)
    {
        file.print(timestamp);
        file.print(",");
        file.println(eventType);
        file.close();

        Serial.println("Event Logged");
    }
    else
    {
        Serial.println("SD Write Failed");
    }

    if(eventType == "FEED")
        lastFeedTime = timestamp;

    if(eventType == "DIAPER")
        lastDiaperTime = timestamp;
}


//----------------------------------
// Feed Event
//----------------------------------
void handleFeed()
{
    logEvent("FEED");

    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Feed Logged!");
    display.display();

    delay(1500);
}


//----------------------------------
// Diaper Event
//----------------------------------
void handleDiaper()
{
    logEvent("DIAPER");

    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Diaper Logged!");
    display.display();

    delay(1500);
}


//----------------------------------
// Menu Placeholder
//----------------------------------
void handleMenu()
{
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Menu");
    display.println("1.View Logs");
    display.println("2.Settings");
    display.display();

    delay(1500);
}


//----------------------------------
// Deep Sleep
//----------------------------------
void goToSleep()
{
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Sleeping...");
    display.display();

    delay(1000);

    display.ssd1306_command(SSD1306_DISPLAYOFF);

    esp_sleep_enable_ext0_wakeup(
        (gpio_num_t)FEED_BUTTON,
        0
    );

    esp_deep_sleep_start();
}


//----------------------------------
// Setup
//----------------------------------
void setup()
{
    Serial.begin(115200);

    pinMode(FEED_BUTTON, INPUT_PULLUP);
    pinMode(DIAPER_BUTTON, INPUT_PULLUP);
    pinMode(MENU_BUTTON, INPUT_PULLUP);
    pinMode(SELECT_BUTTON, INPUT_PULLUP);

    initOLED();

    if(!rtc.begin())
    {
        Serial.println("RTC Failed");
    }

    if(!SD.begin(SD_CS))
    {
        Serial.println("SD Failed");
    }

    lastActivity = millis();

    showHomeScreen();
}


//----------------------------------
// Loop
//----------------------------------
void loop()
{
    if(digitalRead(FEED_BUTTON) == LOW)
    {
        handleFeed();
        lastActivity = millis();
        showHomeScreen();
    }

    if(digitalRead(DIAPER_BUTTON) == LOW)
    {
        handleDiaper();
        lastActivity = millis();
        showHomeScreen();
    }

    if(digitalRead(MENU_BUTTON) == LOW)
    {
        handleMenu();
        lastActivity = millis();
        showHomeScreen();
    }

    if(digitalRead(SELECT_BUTTON) == LOW)
    {
        Serial.println("Select pressed");
        lastActivity = millis();
    }

    if(millis() - lastActivity > sleepTimeout)
    {
        goToSleep();
    }

    delay(100);
}
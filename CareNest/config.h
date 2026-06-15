#ifndef CONFIG_H
#define CONFIG_H

#define SERIAL_BAUD_RATE 115200

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C

#define OLED_SDA_PIN 8
#define OLED_SCL_PIN 9
#define OLED_DC_PIN 18
#define OLED_RESET_PIN 35

// 2x2 Keypad Matrix Pins
#define KEYPAD_ROW0_PIN 13
#define KEYPAD_ROW1_PIN 10
#define KEYPAD_COL0_PIN 12
#define KEYPAD_COL1_PIN 11

#define KEYPAD_ROWS 2
#define KEYPAD_COLS 2
#define BUTTON_COUNT 4
#define BUTTON_EVENT_QUEUE_SIZE 5
#define BUTTON_DEBOUNCE_MS 35

#define SPLASH_DURATION_MS 3500
#define STATUS_DURATION_MS 1400
#define SCREEN_REFRESH_MS 250
#define SLEEP_TIMEOUT_MS 30000UL

#define CLOCK_REFRESH_MS 1000
#define STATUS_DISPLAY_MS 3000

// --- SD Card configuration ---
// Physical pins for SD (set according to wiring):
// CLK = SD_CLK_PIN (SCK), CMD = SD_CMD_PIN (MOSI), D0 = SD_D0_PIN (MISO)
#define SD_CLK_PIN 6
#define SD_CMD_PIN 7
#define SD_D0_PIN 5
// Chip select (CS/SS) pin for the SD card. Update if wired differently.
#define SD_CS_PIN 36

// Optional: if you want to use the default SPI pins, comment out REASSIGN_SD_PINS
#define REASSIGN_SD_PINS 1

#endif

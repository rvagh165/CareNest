#include "Button.h"
#include "config.h"

// 2x2 Keypad matrix mapping
// [row][col] = ButtonId
static const ButtonId keypadMap[KEYPAD_ROWS][KEYPAD_COLS] = {
    {BUTTON_DIAPER,   BUTTON_SELECT},  // Row 0: S1=FEED(0,0), S2=SELECT(0,1)
    {BUTTON_MENU,     BUTTON_FEED  }   // Row 1: S3=MENU(1,0), S4=DIAPER(1,1)
};

// Row and column pin arrays
static const uint8_t rowPins[KEYPAD_ROWS] = {KEYPAD_ROW0_PIN, KEYPAD_ROW1_PIN};
static const uint8_t colPins[KEYPAD_COLS] = {KEYPAD_COL0_PIN, KEYPAD_COL1_PIN};

// Button state tracking for debouncing
typedef struct {
    uint8_t stableState;
    uint8_t lastReading;
    unsigned long lastChangeMs;
} ButtonKeypadState;

static ButtonKeypadState keypadState[KEYPAD_ROWS][KEYPAD_COLS];

static ButtonId eventQueue[BUTTON_EVENT_QUEUE_SIZE];
static uint8_t eventHead = 0;
static uint8_t eventTail = 0;

static void queueEvent(ButtonId id)
{
    uint8_t nextHead = (uint8_t)((eventHead + 1) % BUTTON_EVENT_QUEUE_SIZE);

    if (nextHead != eventTail) {
        eventQueue[eventHead] = id;
        eventHead = nextHead;
    }
}

void buttonsBegin(void)
{
    uint8_t row, col;

    // Initialize row pins as outputs (will drive LOW for scanning)
    for (row = 0; row < KEYPAD_ROWS; row++) {
        pinMode(rowPins[row], OUTPUT);
        digitalWrite(rowPins[row], HIGH);
    }

    // Initialize column pins as inputs with pull-ups
    for (col = 0; col < KEYPAD_COLS; col++) {
        pinMode(colPins[col], INPUT_PULLUP);
    }

    // Initialize keypad state
    for (row = 0; row < KEYPAD_ROWS; row++) {
        for (col = 0; col < KEYPAD_COLS; col++) {
            keypadState[row][col].stableState = HIGH;
            keypadState[row][col].lastReading = HIGH;
            keypadState[row][col].lastChangeMs = millis();
        }
    }
}

void buttonsUpdate(void)
{
    uint8_t row, col;
    unsigned long now = millis();
    uint8_t reading;

    // Scan each row
    for (row = 0; row < KEYPAD_ROWS; row++) {
        // Set current row LOW (active), all others HIGH
        digitalWrite(rowPins[row], LOW);

        // Brief delay to allow column signals to settle
        delayMicroseconds(5);

        // Read each column for this row
        for (col = 0; col < KEYPAD_COLS; col++) {
            reading = digitalRead(colPins[col]);

            // Debounce logic
            if (reading != keypadState[row][col].lastReading) {
                keypadState[row][col].lastReading = reading;
                keypadState[row][col].lastChangeMs = now;
            }

            if ((now - keypadState[row][col].lastChangeMs) >= BUTTON_DEBOUNCE_MS &&
                reading != keypadState[row][col].stableState) {
                keypadState[row][col].stableState = reading;

                // Key press detected (LOW = pressed)
                if (keypadState[row][col].stableState == LOW) {
                    queueEvent(keypadMap[row][col]);
                }
            }
        }

        // Set row back HIGH to deselect
        digitalWrite(rowPins[row], HIGH);
    }
}

bool buttonsGetEvent(ButtonId *button)
{
    if (button == NULL || eventHead == eventTail) {
        return false;
    }

    *button = eventQueue[eventTail];
    eventTail = (uint8_t)((eventTail + 1) % BUTTON_EVENT_QUEUE_SIZE);
    return true;
}

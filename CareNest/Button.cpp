#include "Button.h"
#include "config.h"

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
static bool sw2Pressed = false;
static bool sw3Pressed = false;
static unsigned long ignoreEventsUntilMs = 0;
static bool eventsEnabled = false;

static void deselectRows(void)
{
    uint8_t row;

    for (row = 0; row < KEYPAD_ROWS; row++) {
        pinMode(rowPins[row], INPUT);
    }
}

static void selectRow(uint8_t selectedRow)
{
    deselectRows();
    pinMode(rowPins[selectedRow], OUTPUT);
    digitalWrite(rowPins[selectedRow], LOW);
}

static void queueEvent(ButtonId id)
{
    uint8_t nextHead = (uint8_t)((eventHead + 1) % BUTTON_EVENT_QUEUE_SIZE);

    if (!eventsEnabled) {
        return;
    }

    if (id != BUTTON_NONE && nextHead != eventTail) {
        eventQueue[eventHead] = id;
        eventHead = nextHead;
    }
}

static ButtonId buttonFromPosition(uint8_t row, uint8_t col)
{
    if (row == 0 && col == 1) {
        return BUTTON_SELECT;
    } else if (row == 1 && col == 0) {
        return BUTTON_FEED;
    } else if ((row == 0 && col == 0) || (row == 1 && col == 1)) {
        return BUTTON_DIAPER;
    }

    return BUTTON_NONE;
}

void buttonsBegin(void)
{
    uint8_t row, col;

    // Keep inactive rows floating so a pressed key cannot back-feed another row.
    for (row = 0; row < KEYPAD_ROWS; row++) {
        pinMode(rowPins[row], INPUT);
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

    ignoreEventsUntilMs = millis() + 150;
}

static void enableEventsAfterStartup(void)
{
    uint8_t row, col;

    if (eventsEnabled || millis() < ignoreEventsUntilMs) {
        return;
    }

    for (row = 0; row < KEYPAD_ROWS; row++) {
        for (col = 0; col < KEYPAD_COLS; col++) {
            keypadState[row][col].stableState = HIGH;
            keypadState[row][col].lastReading = HIGH;
            keypadState[row][col].lastChangeMs = millis();
        }
    }

    sw2Pressed = false;
    sw3Pressed = false;
    eventsEnabled = true;
}

void buttonsUpdate(void)
{
    uint8_t row, col;
    unsigned long now = millis();
    uint8_t reading;
    bool feedPressedNow;
    bool diaperPressedNow;
    bool sw3PressedNow;
    bool keyPressed[KEYPAD_ROWS][KEYPAD_COLS] = {false};

    enableEventsAfterStartup();

    // Scan each row
    for (row = 0; row < KEYPAD_ROWS; row++) {
        // Set current row LOW (active), all others high impedance.
        selectRow(row);

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
                    Serial.print("Raw button: ");
                    Serial.print(row);
                    Serial.print(",");
                    Serial.println(col);
                    keyPressed[row][col] = true;
                }
            }
        }

        // Deselect this row before scanning the next one.
        pinMode(rowPins[row], INPUT);
    }

    deselectRows();

    feedPressedNow = (keypadState[1][0].stableState == LOW);
    diaperPressedNow = (keypadState[1][1].stableState == LOW);
    sw3PressedNow = (keypadState[0][0].stableState == LOW);

    if (feedPressedNow && diaperPressedNow) {
        if (!sw2Pressed) {
            queueEvent(BUTTON_MENU);
            sw2Pressed = true;
        }
    } else {
        if (sw3PressedNow) {
            if (!sw3Pressed) {
                queueEvent(BUTTON_DIAPER);
                sw3Pressed = true;
            }
        } else {
            sw3Pressed = false;
        }

        for (row = 0; row < KEYPAD_ROWS; row++) {
            for (col = 0; col < KEYPAD_COLS; col++) {
                if (keyPressed[row][col] && !(row == 0 && col == 0)) {
                    queueEvent(buttonFromPosition(row, col));
                }
            }
        }

        sw2Pressed = false;
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

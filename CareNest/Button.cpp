#include "Button.h"
#include "config.h"

#include <Keypad.h>

static char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2'},
    {'3', '4'}
};

static byte rowPins[KEYPAD_ROWS] = {KEYPAD_ROW0_PIN, KEYPAD_ROW1_PIN};
static byte colPins[KEYPAD_COLS] = {KEYPAD_COL0_PIN, KEYPAD_COL1_PIN};

static Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);

static ButtonId eventQueue[BUTTON_EVENT_QUEUE_SIZE];
static uint8_t eventHead = 0;
static uint8_t eventTail = 0;
static unsigned long ignoreEventsUntilMs = 0;
static bool eventsEnabled = false;

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

static ButtonId buttonFromKey(char key)
{
    if (key == '4') {
        return BUTTON_FEED;
    } else if (key == '3') {
        return BUTTON_DIAPER;
    } else if (key == '2') {
        return BUTTON_MENU;
    } else if (key == '1') {
        return BUTTON_SELECT;
    }

    return BUTTON_NONE;
}

static void enableEventsAfterStartup(void)
{
    if (!eventsEnabled && millis() >= ignoreEventsUntilMs) {
        eventsEnabled = true;
    }
}

void buttonsBegin(void)
{
    keypad.setDebounceTime(BUTTON_DEBOUNCE_MS);
    ignoreEventsUntilMs = millis() + 150;
    eventsEnabled = false;
    eventHead = 0;
    eventTail = 0;
}

void buttonsUpdate(void)
{
    char key;
    ButtonId button;

    enableEventsAfterStartup();

    key = keypad.getKey();
    if (key) {
        Serial.print("Key Pressed: ");
        Serial.println(key);

        button = buttonFromKey(key);
        queueEvent(button);
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

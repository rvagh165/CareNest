#include "Button.h"
#include "config.h"

typedef struct {
    ButtonId id;
    uint8_t pin;
    uint8_t stableState;
    uint8_t lastReading;
    unsigned long lastChangeMs;
} ButtonState;

static ButtonState buttons[BUTTON_COUNT] = {
    {BUTTON_FEED, FEED_BUTTON_PIN, HIGH, HIGH, 0},
    {BUTTON_DIAPER, DIAPER_BUTTON_PIN, HIGH, HIGH, 0},
    {BUTTON_MENU, MENU_BUTTON_PIN, HIGH, HIGH, 0},
    {BUTTON_SELECT, SELECT_BUTTON_PIN, HIGH, HIGH, 0}
};

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
    uint8_t i;

    for (i = 0; i < BUTTON_COUNT; i++) {
        pinMode(buttons[i].pin, INPUT_PULLUP);
        buttons[i].stableState = digitalRead(buttons[i].pin);
        buttons[i].lastReading = buttons[i].stableState;
        buttons[i].lastChangeMs = millis();
    }
}

void buttonsUpdate(void)
{
    uint8_t i;
    unsigned long now = millis();

    for (i = 0; i < BUTTON_COUNT; i++) {
        uint8_t reading = digitalRead(buttons[i].pin);

        if (reading != buttons[i].lastReading) {
            buttons[i].lastReading = reading;
            buttons[i].lastChangeMs = now;
        }

        if ((now - buttons[i].lastChangeMs) >= BUTTON_DEBOUNCE_MS &&
            reading != buttons[i].stableState) {
            buttons[i].stableState = reading;

            if (buttons[i].stableState == LOW) {
                queueEvent(buttons[i].id);
            }
        }
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

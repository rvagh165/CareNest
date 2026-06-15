#include "Button.h"
#include "config.h"

#include <Keypad.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>

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

static void restoreKeypadPins(void)
{
    const gpio_num_t keypadPins[] = {
        (gpio_num_t)KEYPAD_ROW0_PIN,
        (gpio_num_t)KEYPAD_ROW1_PIN,
        (gpio_num_t)KEYPAD_COL0_PIN,
        (gpio_num_t)KEYPAD_COL1_PIN
    };

    gpio_deep_sleep_hold_dis();

    for (size_t index = 0; index < (sizeof(keypadPins) / sizeof(keypadPins[0])); ++index) {
        gpio_num_t pin = keypadPins[index];
        if (rtc_gpio_is_valid_gpio(pin)) {
            rtc_gpio_hold_dis(pin);
            rtc_gpio_deinit(pin);
        }
    }
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

static ButtonId buttonFromKey(char key)
{
    if (key == '4') {
        return BUTTON_FEED;
    } else if (key == '3') {
        return BUTTON_DIAPER;
    } else if (key == '2') {
        return BUTTON_SELECT;
    } else if (key == '1') {
        return BUTTON_MENU;
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
    restoreKeypadPins();
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

bool buttonsPrepareForDeepSleepWake(void)
{
    const gpio_num_t drivePins[] = {
        (gpio_num_t)KEYPAD_ROW0_PIN,
        (gpio_num_t)KEYPAD_ROW1_PIN
    };
    const gpio_num_t wakePins[] = {
        (gpio_num_t)KEYPAD_COL0_PIN,
        (gpio_num_t)KEYPAD_COL1_PIN
    };
    uint64_t wakeMask = 0;

    restoreKeypadPins();

    for (size_t index = 0; index < (sizeof(drivePins) / sizeof(drivePins[0])); ++index) {
        if (!rtc_gpio_is_valid_gpio(drivePins[index])) {
            Serial.printf("[Button] GPIO %d cannot be used for deep sleep wake drive\n", (int)drivePins[index]);
            return false;
        }
    }

    for (size_t index = 0; index < (sizeof(wakePins) / sizeof(wakePins[0])); ++index) {
        if (!rtc_gpio_is_valid_gpio(wakePins[index])) {
            Serial.printf("[Button] GPIO %d cannot be used for deep sleep wake input\n", (int)wakePins[index]);
            return false;
        }
        wakeMask |= (1ULL << wakePins[index]);
    }

    eventHead = 0;
    eventTail = 0;
    eventsEnabled = false;

    for (size_t index = 0; index < (sizeof(drivePins) / sizeof(drivePins[0])); ++index) {
        gpio_num_t pin = drivePins[index];
        rtc_gpio_init(pin);
        rtc_gpio_set_direction(pin, RTC_GPIO_MODE_OUTPUT_ONLY);
        rtc_gpio_pullup_dis(pin);
        rtc_gpio_pulldown_dis(pin);
        rtc_gpio_set_level(pin, 1);
        rtc_gpio_hold_en(pin);
    }

    for (size_t index = 0; index < (sizeof(wakePins) / sizeof(wakePins[0])); ++index) {
        gpio_num_t pin = wakePins[index];
        rtc_gpio_init(pin);
        rtc_gpio_set_direction(pin, RTC_GPIO_MODE_INPUT_ONLY);
        rtc_gpio_pullup_dis(pin);
        rtc_gpio_pulldown_en(pin);
        rtc_gpio_hold_en(pin);
    }

    gpio_deep_sleep_hold_en();

    esp_err_t err = esp_sleep_enable_ext1_wakeup_io(wakeMask, ESP_EXT1_WAKEUP_ANY_HIGH);
    if (err != ESP_OK) {
        Serial.printf("[Button] Failed to enable EXT1 wakeup: %d\n", (int)err);
        restoreKeypadPins();
        return false;
    }

    return true;
}

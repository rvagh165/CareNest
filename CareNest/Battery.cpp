/*
   ESP32-S3 Battery Voltage Monitor

   Battery --> 100k --> ADC(GPIO3) --> 100k --> GND

   Divider Ratio = 2:1
*/

#include <Arduino.h>

#include "Battery.h"

#define BATTERY_PIN     3
#define ADC_SAMPLES     32

#define EMA_ALPHA       0.1f   // smaller = smoother but slower to react (try 0.05 - 0.2)

static float smoothedVoltage = -1.0f;  // persists across calls
static int8_t lastPercent = -1;        // persists across calls

//------------------------------------------------------------

float getBatteryVoltage()
{
    uint32_t sum = 0;

    for (int i = 0; i < ADC_SAMPLES; i++)
    {
        sum += analogReadMilliVolts(BATTERY_PIN);
    }

    float adcVoltage = (float)sum / ADC_SAMPLES;
    float rawVoltage = adcVoltage * 2.0f;   // undo divider

    if(rawVoltage >= MAX_VOLTAGE) {
        return MAX_VOLTAGE;        
    }

    // --- Temporal smoothing (low-pass filter across calls) ---
    if (smoothedVoltage < 0)
        smoothedVoltage = rawVoltage;       // first run, no history yet
    else
        smoothedVoltage = (EMA_ALPHA * rawVoltage) + ((1.0f - EMA_ALPHA) * smoothedVoltage);

    Serial.printf("battery pin (mv) : %.1f  raw: %.1f  smoothed: %.1f\n",
                  adcVoltage, rawVoltage, smoothedVoltage);

    return smoothedVoltage;
}

//------------------------------------------------------------

uint8_t batteryPercentage(float voltage)
{
    if (voltage >= MAX_VOLTAGE) return 100;
    if (voltage <= MIN_VOLTAGE) return 0;

    float percent = ((voltage - MIN_VOLTAGE) * 100.0f) / (MAX_VOLTAGE - MIN_VOLTAGE);
    int8_t rounded = (int8_t)(percent + 0.5f);   // round, don't truncate

    // --- Hysteresis: ignore ±1% jitter around the last shown value ---
    if (lastPercent == -1 || abs(rounded - lastPercent) >= 1)
        lastPercent = rounded;

    return (uint8_t)lastPercent;
}
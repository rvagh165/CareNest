/*
   ESP32-S3 Battery Voltage Monitor

   Battery --> 100k --> ADC(GPIO3) --> 100k --> GND

   Divider Ratio = 2:1
*/

#include <Arduino.h>

#define MIN_VOLTAGE  3000
#define MAX_VOLTAGE  4200

#define BATTERY_PIN    3
#define ADC_SAMPLES    32



//------------------------------------------------------------

float getBatteryVoltage()
{
    uint32_t sum = 0;

    for (int i = 0; i < ADC_SAMPLES; i++)
    {
        sum += analogReadMilliVolts(BATTERY_PIN);
        Serial.printf("SUM : %d  \n",sum);
        // delay(2);
    }

    Serial.printf("battery pin (mv) : %d  \n", (sum / ADC_SAMPLES) );
    float adcVoltage = sum / ADC_SAMPLES;

    // Undo the divider
    return adcVoltage * 2.0;
}

//------------------------------------------------------------

uint8_t batteryPercentage(float voltage)
{


    if (voltage >= MAX_VOLTAGE)
        return 100;

    if (voltage <= MIN_VOLTAGE)
        return 0;

    return (uint8_t)(((voltage - MIN_VOLTAGE) * 100.0) /
                     (MAX_VOLTAGE - MIN_VOLTAGE));
}


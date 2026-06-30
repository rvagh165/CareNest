#ifndef BATTERY_H
#define BATTERY_H

#define MIN_VOLTAGE     3000
#define MAX_VOLTAGE     4160

float getBatteryVoltage();
uint8_t batteryPercentage(float voltage);

#endif
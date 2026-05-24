#ifndef STARTUP_ANIMATION_H
#define STARTUP_ANIMATION_H

#include <Arduino.h>

#define STARTUP_ANIMATION_WIDTH 128
#define STARTUP_ANIMATION_HEIGHT 64
#define STARTUP_ANIMATION_FRAME_COUNT 28
#define STARTUP_ANIMATION_FRAME_INTERVAL_MS 100UL

const uint8_t *startupAnimationFrame(uint8_t frameIndex);

#endif

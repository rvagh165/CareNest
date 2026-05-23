#ifndef STARTUP_ANIMATION_H
#define STARTUP_ANIMATION_H

#include <Arduino.h>

#define STARTUP_ANIMATION_WIDTH 64
#define STARTUP_ANIMATION_HEIGHT 32
#define STARTUP_ANIMATION_FRAME_COUNT 8
#define STARTUP_ANIMATION_FRAME_INTERVAL_MS 100UL

const uint8_t *startupAnimationFrame(uint8_t frameIndex);

#endif

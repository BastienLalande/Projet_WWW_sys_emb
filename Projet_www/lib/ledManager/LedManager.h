// led manager .h
#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>
#include <ChainableLED.h>

typedef struct {
    uint8_t r1, g1, b1;
    uint8_t r2, g2, b2;
    float frequency;
    float ratio;
} LedPattern;

typedef enum : uint8_t {
    ERROR_RTC_ACCESS,
    ERROR_GPS_ACCESS,
    ERROR_SENSOR_ACCESS,
    ERROR_SENSOR_INCOHERENT,
    ERROR_SD_FULL,
    ERROR_SD_ACCESS,
    ERROR_COUNT
} ErrorCode;

// === Fonctions publiques ===
void LedManager_Init(uint8_t dataPin, uint8_t clockPin, uint8_t ledCount = 1);
void LedManager_Update();
void LedManager_Clear();
void LedManager_Feedback(ErrorCode error_id);

void LedManager_SetModeColor(uint8_t r, uint8_t g, uint8_t b);
void LedManager_RestoreModeColor();

bool LedManager_IsBusy();
void LedManager_SetColor(uint8_t r, uint8_t g, uint8_t b);

#endif

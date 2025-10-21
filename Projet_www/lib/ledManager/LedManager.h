#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>
#include <ChainableLED.h>

// === Types ===
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
void Led_Init(uint8_t dataPin = 7, uint8_t clockPin = 8, uint8_t ledCount = 1);
void Led_SetColor(uint8_t r, uint8_t g, uint8_t b);
void Led_SetModeColor(uint8_t r, uint8_t g, uint8_t b);
void Led_RestoreModeColor();
void Led_Feedback(ErrorCode error_id);
void Led_Clear();
void Led_Update();
bool Led_IsBusy();

#endif

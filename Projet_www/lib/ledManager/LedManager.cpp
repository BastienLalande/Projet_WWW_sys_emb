#include "LedManager.h"
#include <avr/pgmspace.h>

static ChainableLED led(7, 8, 1); // valeurs par défaut
static ErrorCode current_error = (ErrorCode)-1;
static bool showing_first_color = true;
static unsigned long last_update_time = 0;
static uint8_t cycles_done = 0;
static const uint8_t MAX_CYCLES = 2;

static uint8_t mode_r = 0, mode_g = 0, mode_b = 0;

static const LedPattern error_patterns[ERROR_COUNT] PROGMEM = {
    {255, 0, 0, 0, 0, 255, 1, 1.0},   // RTC
    {255, 0, 0, 255, 255, 0, 1.0, 1.0}, // GPS
    {255, 0, 0, 0, 255, 0, 1.0, 1.0}, // Capteur accès
    {255, 0, 0, 0, 255, 0, 1.0, 2.0}, // Capteur incohérent
    {255, 0, 0, 255, 255, 255, 1.0, 1.0}, // SD pleine
    {255, 0, 0, 255, 255, 255, 1.0, 2.0}  // SD accès
};

void Led_Init(uint8_t dataPin, uint8_t clockPin, uint8_t ledCount) {
    led = ChainableLED(dataPin, clockPin, ledCount);
    Led_SetColor(0, 0, 0);
}

void Led_SetColor(uint8_t r, uint8_t g, uint8_t b) {
    led.setColorRGB(0, r, g, b);
}

bool Led_IsBusy() {
    return current_error != (ErrorCode)-1;
}

void Led_SetModeColor(uint8_t r, uint8_t g, uint8_t b) {
    mode_r = r;
    mode_g = g;
    mode_b = b;
    Led_SetColor(r, g, b);
}

void Led_RestoreModeColor() {
    Led_SetColor(mode_r, mode_g, mode_b);
}

void Led_Feedback(ErrorCode error_id) {
    if (error_id >= ERROR_COUNT) return;
    if (Led_IsBusy()) return;

    current_error = error_id;
    showing_first_color = true;
    last_update_time = millis();
    cycles_done = 0;

    LedPattern pattern;
    memcpy_P(&pattern, &error_patterns[error_id], sizeof(LedPattern));
    Led_SetColor(pattern.r1, pattern.g1, pattern.b1);
}

void Led_Clear() {
    current_error = (ErrorCode)-1;
    cycles_done = 0;
    Led_RestoreModeColor();
}

void Led_Update() {
    if (!Led_IsBusy()) return;

    LedPattern pattern;
    memcpy_P(&pattern, &error_patterns[current_error], sizeof(LedPattern));

    const unsigned long now = millis();
    const float invFreq = 1000.0f / pattern.frequency;
    const float t1 = invFreq / (1.0f + pattern.ratio);
    const float t2 = t1 * pattern.ratio;

    if (showing_first_color) {
        if (now - last_update_time >= (unsigned long)t1) {
            Led_SetColor(pattern.r2, pattern.g2, pattern.b2);
            showing_first_color = false;
            last_update_time = now;
        }
    } else {
        if (now - last_update_time >= (unsigned long)t2) {
            Led_SetColor(pattern.r1, pattern.g1, pattern.b1);
            showing_first_color = true;
            last_update_time = now;
            if (++cycles_done >= MAX_CYCLES) Led_Clear();
        }
    }
}

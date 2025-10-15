#include "LedManager.h"
#include <avr/pgmspace.h>

const LedPattern LedManager::error_patterns[ERROR_COUNT] PROGMEM = {
    {255, 0, 0,   0, 0, 255,     1, 1.0},   // RTC
    {255, 0, 0,   255, 255, 0,   1.0, 1.0}, // GPS
    {255, 0, 0,   0, 255, 0,     1.0, 1.0}, // Capteur accès
    {255, 0, 0,   0, 255, 0,     1.0, 2.0}, // Capteur incohérent
    {255, 0, 0,   255, 255, 255, 1.0, 1.0}, // SD pleine
    {255, 0, 0,   255, 255, 255, 1.0, 2.0}  // SD accès
};

LedManager::LedManager(uint8_t dataPin, uint8_t clockPin, uint8_t ledCount)
    : led(dataPin, clockPin, ledCount),
      current_error((ErrorCode)-1),
      showing_first_color(true),
      last_update_time(0),
      cycles_done(0)
{}

void LedManager::Init_Led() {
    setColor(0, 0, 0);
}


inline void LedManager::setColor(uint8_t r, uint8_t g, uint8_t b) {
    led.setColorRGB(0, r, g, b);
}


inline bool LedManager::isBusy() const {
    return current_error != (ErrorCode)-1;
}


void LedManager::feedback(const ErrorCode error_id) {
    if (error_id >= ERROR_COUNT) return;

    if (isBusy()) {
        Serial.print(F("[INFO] Pattern déjà en cours ("));
        Serial.print(current_error);
        Serial.println(F("), ignoré"));
        return;
    }

    current_error = error_id;
    showing_first_color = true;
    last_update_time = millis();
    cycles_done = 0;

    // Lecture du pattern depuis la mémoire Flash
    LedPattern pattern;
    memcpy_P(&pattern, &error_patterns[error_id], sizeof(LedPattern));

    setColor(pattern.r1, pattern.g1, pattern.b1);

    Serial.print(F("[ERROR] Pattern "));
    Serial.print(error_id);
    Serial.println(F(" activé"));
}

// === Effacement du pattern ===
void LedManager::clear() {
    current_error = (ErrorCode)-1;
    cycles_done = 0;
    Init_Led();
}

// === Mise à jour du pattern ===
void LedManager::update() {
    if (!isBusy()) return;

    LedPattern pattern;
    memcpy_P(&pattern, &error_patterns[current_error], sizeof(LedPattern));

    const unsigned long now = millis();
    const float invFreq = 1000.0f / pattern.frequency;
    const float t1 = invFreq / (1.0f + pattern.ratio);
    const float t2 = t1 * pattern.ratio;

    if (showing_first_color) {
        if (now - last_update_time >= (unsigned long)t1) {
            setColor(pattern.r2, pattern.g2, pattern.b2);
            showing_first_color = false;
            last_update_time = now;
        }
    } else {
        if (now - last_update_time >= (unsigned long)t2) {
            setColor(pattern.r1, pattern.g1, pattern.b1);
            showing_first_color = true;
            last_update_time = now;
            if (++cycles_done >= MAX_CYCLES)
                clear();
        }
    }
}

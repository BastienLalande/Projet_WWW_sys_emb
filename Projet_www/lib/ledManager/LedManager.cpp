// led manager .cpp

#include "LedManager.h"
#include <avr/pgmspace.h>

// === Constantes ===
static const LedPattern error_patterns[ERROR_COUNT] PROGMEM = {
    {255, 0, 0,   0, 0, 255,     1, 1.0},   // RTC
    {255, 0, 0,   255, 255, 0,   1.0, 1.0}, // GPS
    {255, 0, 0,   0, 255, 0,     1.0, 1.0}, // Capteur acces
    {255, 0, 0,   0, 255, 0,     1.0, 2.0}, // Capteur incoherent
    {255, 0, 0,   255, 255, 255, 1.0, 1.0}, // SD pleine
    {255, 0, 0,   255, 255, 255, 1.0, 2.0}  // SD acces
};

// === Variables globales ===
static ChainableLED* led = nullptr;
static ErrorCode current_error = (ErrorCode)-1;
static bool showing_first_color = true;
static unsigned long last_update_time = 0;
static uint8_t cycles_done = 0;
static const uint8_t MAX_CYCLES = 2;

static uint8_t mode_r = 0, mode_g = 0, mode_b = 0;

// === Fonctions internes ===
void LedManager_SetColor(uint8_t r, uint8_t g, uint8_t b) {
    if (led) led->setColorRGB(0, r, g, b);
}

bool LedManager_IsBusy() {
    return current_error != (ErrorCode)-1;
}

// === Initialisation ===
void LedManager_Init(uint8_t dataPin, uint8_t clockPin, uint8_t ledCount) {
    if (led) delete led;
    led = new ChainableLED(dataPin, clockPin, ledCount);
    LedManager_SetColor(0, 0, 0);
    Serial.println(F("[INFO] LedManager initialisé"));
}

// === Mode couleur ===
void LedManager_SetModeColor(uint8_t r, uint8_t g, uint8_t b) {
    mode_r = r;
    mode_g = g;
    mode_b = b;
    LedManager_SetColor(r, g, b);
}

void LedManager_RestoreModeColor() {
    LedManager_SetColor(mode_r, mode_g, mode_b);
}

// === Feedback d’erreur ===
void LedManager_Feedback(ErrorCode error_id) {
    if (error_id >= ERROR_COUNT) return;

    if (LedManager_IsBusy()) {
        Serial.print(F("[INFO] Pattern dejà en cours ("));
        Serial.print(current_error);
        Serial.println(F("), ignore"));
        return;
    }

    current_error = error_id;
    showing_first_color = true;
    last_update_time = millis();
    cycles_done = 0;

    LedPattern pattern;
    memcpy_P(&pattern, &error_patterns[error_id], sizeof(LedPattern));
    LedManager_SetColor(pattern.r1, pattern.g1, pattern.b1);

    Serial.print(F("[ERROR] Pattern "));
    Serial.print(error_id);
    Serial.println(F(" active"));
}

// === Effacement du pattern ===
void LedManager_Clear() {
    current_error = (ErrorCode)-1;
    cycles_done = 0;
    LedManager_RestoreModeColor();
}

// === Mise à jour ===
void LedManager_Update() {
    if (!LedManager_IsBusy()) return;

    LedPattern pattern;
    memcpy_P(&pattern, &error_patterns[current_error], sizeof(LedPattern));

    const unsigned long now = millis();
    const float invFreq = 1000.0f / pattern.frequency;
    const float t1 = invFreq / (1.0f + pattern.ratio);
    const float t2 = t1 * pattern.ratio;

    if (showing_first_color) {
        if (now - last_update_time >= (unsigned long)t1) {
            LedManager_SetColor(pattern.r2, pattern.g2, pattern.b2);
            showing_first_color = false;
            last_update_time = now;
        }
    } else {
        if (now - last_update_time >= (unsigned long)t2) {
            LedManager_SetColor(pattern.r1, pattern.g1, pattern.b1);
            showing_first_color = true;
            last_update_time = now;
            if (++cycles_done >= MAX_CYCLES) LedManager_Clear();
        }
    }
}

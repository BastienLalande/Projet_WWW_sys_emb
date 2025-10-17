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

class LedManager {
public:
    LedManager(uint8_t dataPin, uint8_t clockPin, uint8_t ledCount = 1);

    void Init_Led();

    inline void setColor(uint8_t r, uint8_t g, uint8_t b);
    inline bool isBusy() const;

    void feedback(const ErrorCode error_id);
    void clear();
    void update();

    // === Nouveau ===
    void setModeColor(uint8_t r, uint8_t g, uint8_t b);
    void restoreModeColor();

private:
    ChainableLED led;
    ErrorCode current_error;
    bool showing_first_color;
    unsigned long last_update_time;
    uint8_t cycles_done;
    static constexpr uint8_t MAX_CYCLES = 2;

    // Sauvegarde de la couleur du mode courant
    uint8_t mode_r, mode_g, mode_b;

    static const LedPattern error_patterns[ERROR_COUNT];
};

#endif

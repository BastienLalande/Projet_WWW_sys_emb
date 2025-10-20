#include <Arduino.h>
#include <Adafruit_BME280.h>
#include <SoftwareSerial.h>
#include <LedManager.h>

struct SensorData {
  float temperature;
  float humidity;
  float pressure;
  int luminosity;
  bool tempError;
  bool hygrError;
  bool pressError;
  bool luminError;
};

class CapteurManager {
public:
  CapteurManager(uint8_t pinGPSRx, uint8_t pinGPSTx, uint8_t pinLuminosity, LedManager& led);

  bool begin();
  SensorData readSensors();
  String readGPS();
  String dataToCSV(const SensorData& d);
  bool isBMEOK() const { return bmeOK; }

private:
  Adafruit_BME280 bme;
  SoftwareSerial gpsSerial;
  LedManager& ledManager;
  uint8_t pinLum;
  bool bmeOK;

  struct SensorParams {
    bool TEMP_AIR;
    int8_t MIN_TEMP_AIR, MAX_TEMP_AIR;
    bool HYGR;
    int8_t HYGR_MINT, HYGR_MAXT;
    bool PRESSURE;
    float PRESSURE_MIN, PRESSURE_MAX;
    bool LUMIN;
    uint16_t LUMIN_LOW, LUMIN_HIGH;
  } params;
};

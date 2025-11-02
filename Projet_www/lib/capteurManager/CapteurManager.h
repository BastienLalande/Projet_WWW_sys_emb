#ifndef CAPTEURMANAGER_H
#define CAPTEURMANAGER_H

#include <Arduino.h>

// --- Structure des données capteurs ---
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

// --- Structure des paramètres ---
struct SensorParams {
  bool tempCheck; float MIN_TEMP_AIR; float MAX_TEMP_AIR;
  bool hygrCheck; float HYGR_MINT; float HYGR_MAXT;
  bool pressCheck; float PRESSURE_MIN; float PRESSURE_MAX;
  bool luminCheck; int LUMIN_LOW; int LUMIN_HIGH;
};

// --- Déclarations des fonctions ---
bool init_capteur();
SensorData readSensors();
bool readGPS(float& lat, float& lon);

// --- Variables globales externes ---
extern SensorParams sensorParams;
extern bool bmeOK;

#endif // CAPTEURMANAGER_H
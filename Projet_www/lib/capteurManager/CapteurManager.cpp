#include <Arduino.h>
#include <Adafruit_BME280.h>
#include <SoftwareSerial.h>
#include <LedManager.h>
#include <EEPROM.h>
#include <ConfigManager.h>
#include <TinyGPSPlus.h>

#define GPS_RX 7
#define GPS_TX 8
#define LUMINOSITY_PIN A0

// --- Objets globaux ---
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);
TinyGPSPlus gps;
Adafruit_BME280 bme;

bool bmeOK = false;

Parametres configParams;

struct SensorData {
  float  temperature;   // °C
  float  humidity;      // %RH
  float  pressure;      // hPa
  int    luminosity;    // 0..1023
  bool   tempError;
  bool   hygrError;
  bool   pressError;
  bool   luminError;
};

// ------------------ Initialisation ------------------
bool init_capteur() {
  gpsSerial.begin(9600);
  Wire.begin();

  bmeOK = bme.begin(0x76);
  if (!bmeOK) {
    Serial.println(F("[ERROR] capteur BME280 non detecte !"));
    LedManager_Feedback(ERROR_SENSOR_ACCESS);
    return false;
  }

  pinMode(LUMINOSITY_PIN, INPUT);
  Serial.println(F("[INFO] CapteurManager initialisé"));
  return true;
}

// ------------------ Lecture capteurs ------------------
SensorData readSensors() 
{
  SensorData d = {};
  if (!bmeOK) {
    LedManager_Feedback(ERROR_SENSOR_ACCESS);
    return d;
  }

  EEPROM.get(0, configParams);

  if( configParams.TEMP_AIR ){  d.temperature = bme.readTemperature();}else{d.temperature=0.0;}
  if( configParams.HYGR ){ d.humidity = bme.readHumidity();}else{d.humidity=0.0;}
  if( configParams.PRESSURE ){ d.pressure = bme.readPressure() * 0.01F;}else{d.pressure =215.0;}
  if( configParams.LUMIN ){ d.luminosity = analogRead(LUMINOSITY_PIN);}else{d.luminosity=0;}

  d.tempError = (d.temperature < configParams.MIN_TEMP_AIR || d.temperature > configParams.MAX_TEMP_AIR);
  d.hygrError = (d.temperature < configParams.HYGR_MINT || d.temperature > configParams.HYGR_MAXT);
  d.pressError = (d.pressure < configParams.PRESSURE_MIN || d.pressure > configParams.PRESSURE_MAX);
  d.luminError = (d.luminosity < configParams.LUMIN_LOW || d.luminosity > configParams.LUMIN_HIGH);

  if (d.tempError || d.pressError)
  { 
    LedManager_Feedback(ERROR_SENSOR_INCOHERENT);
  }

  return d;
}

float convertToDecimal(String coord, String dir) {
  float raw = coord.toFloat();
  int deg = (int)(raw / 100);
  float min = raw - (deg * 100);
  float decimal = deg + (min / 60.0);
  if (dir == F("S") || dir == F("W")) decimal *= -1;
  return decimal;
}

bool readGPS(float &lat, float &lon)
{
  while (gpsSerial.available())
  {
    char c = gpsSerial.read();
    gps.encode(c);
  }
  if (gps.location.isValid())
  {
    lat = gps.location.lat();
    lon = gps.location.lng();
    return true;
  }
  return false;
}
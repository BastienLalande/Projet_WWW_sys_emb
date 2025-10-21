#include <Adafruit_BME280.h>
#include <SoftwareSerial.h>
#include <LedManager.h>
#include <Wire.h>


#define GPS_RX 7
#define GPS_TX 8
#define LUMINOSITY_PIN A0

// --- Objets globaux ---
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);
Adafruit_BME280 bme;

bool bmeOK = false;

struct SensorParams {
  bool TEMP_AIR;
  int8_t MIN_TEMP_AIR, MAX_TEMP_AIR;
  bool HYGR;
  int8_t HYGR_MINT, HYGR_MAXT;
  bool PRESSURE;
  float PRESSURE_MIN, PRESSURE_MAX;
  bool LUMIN;
  uint16_t LUMIN_LOW, LUMIN_HIGH;
} sensorParams;

struct SensorData {
  float temperature;
  float humidity;
  float pressure;
  unsigned int luminosity;
  bool tempError;
  bool hygrError;
  bool pressError;
  bool luminError;
};

bool init_capteur() {
  gpsSerial.begin(9600);
  Wire.begin();
  bmeOK = bme.begin(0x76);
  if (!bmeOK) {
    Serial.println(F("Erreur : capteur BME280 non détecté !"));
    Led_Feedback(ERROR_SENSOR_ACCESS);
    return false;
  }

  pinMode(LUMINOSITY_PIN, INPUT);
  gpsSerial.begin(9600);
  Serial.println(F("CapteurManager prêt"));
  return true;
}

SensorData readSensors() {
  SensorData d = {};
  if (!bmeOK) {
    Led_Feedback(ERROR_SENSOR_ACCESS);
    return d;
  }

  d.temperature = bme.readTemperature();
  d.humidity = bme.readHumidity();
  d.pressure = bme.readPressure() * 0.01F;
  d.luminosity = analogRead(LUMINOSITY_PIN);

  d.tempError = (d.temperature < sensorParams.MIN_TEMP_AIR || d.temperature > sensorParams.MAX_TEMP_AIR);
  d.hygrError = (d.humidity < sensorParams.HYGR_MINT || d.humidity > sensorParams.HYGR_MAXT);
  d.pressError = (d.pressure < sensorParams.PRESSURE_MIN || d.pressure > sensorParams.PRESSURE_MAX);
  d.luminError = (d.luminosity < sensorParams.LUMIN_LOW || d.luminosity > sensorParams.LUMIN_HIGH);

  if (d.tempError || d.pressError)
    Led_Feedback(ERROR_SENSOR_INCOHERENT);

  return d;
}

String readGPS() {
  while (gpsSerial.available()) {
    String line = gpsSerial.readStringUntil('\n');
    if (line.startsWith("$GPGGA")) return line;
  }
  Led_Feedback(ERROR_GPS_ACCESS);
  return "No GPS data";
}
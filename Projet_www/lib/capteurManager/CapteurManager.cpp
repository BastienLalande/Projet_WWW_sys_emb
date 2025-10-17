#include "CapteurManager.h"

CapteurManager::CapteurManager(uint8_t pinGPSRx, uint8_t pinGPSTx, uint8_t pinLuminosity, LedManager& led)
: gpsSerial(pinGPSRx, pinGPSTx), ledManager(led), pinLum(pinLuminosity), bmeOK(false)
{
  params = { true, -10, 60, true, 0, 50, true, 850, 1080, true, 200, 700 };
}

bool CapteurManager::begin() {
  gpsSerial.begin(9600);
  bmeOK = bme.begin(0x76);
  if (!bmeOK) {
    Serial.println(F("Erreur : capteur BME280 non détecté !"));
    ledManager.feedback(ERROR_SENSOR_ACCESS);
    return false;
  }
  pinMode(pinLum, INPUT);
  Serial.println(F("CapteurManager prêt"));
  return true;
}

SensorData CapteurManager::readSensors() {
  SensorData d = {};
  if (!bmeOK) {
    ledManager.feedback(ERROR_SENSOR_ACCESS);
    return d;
  }

  d.temperature = bme.readTemperature();
  d.humidity = bme.readHumidity();
  d.pressure = bme.readPressure() * 0.01F;
  d.luminosity = analogRead(pinLum);

  d.tempError = (d.temperature < params.MIN_TEMP_AIR || d.temperature > params.MAX_TEMP_AIR);
  d.hygrError = (d.humidity < params.HYGR_MINT || d.humidity > params.HYGR_MAXT);
  d.pressError = (d.pressure < params.PRESSURE_MIN || d.pressure > params.PRESSURE_MAX);
  d.luminError = (d.luminosity < params.LUMIN_LOW || d.luminosity > params.LUMIN_HIGH);

  if (d.tempError || d.pressError)
    ledManager.feedback(ERROR_SENSOR_INCOHERENT);

  return d;
}

String CapteurManager::readGPS() {
  while (gpsSerial.available()) {
    String line = gpsSerial.readStringUntil('\n');
    if (line.startsWith("$GPGGA")) return line;
  }
  ledManager.feedback(ERROR_GPS_ACCESS);
  return "No GPS data";
}

String CapteurManager::dataToCSV(const SensorData &d) {
  return String(d.temperature, 1) + ";" +
         String(d.humidity, 1) + ";" +
         String(d.pressure, 1) + ";" +
         String(d.luminosity) + ";" +
         String(d.tempError) + ";" +
         String(d.hygrError) + ";" +
         String(d.pressError) + ";" +
         String(d.luminError);
}

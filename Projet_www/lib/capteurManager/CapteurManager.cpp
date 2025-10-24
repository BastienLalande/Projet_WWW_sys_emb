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
    Serial.println(F("Erreur : capteur BME280 non detecte !"));
    LedManager_Feedback(ERROR_SENSOR_ACCESS);
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
    LedManager_Feedback(ERROR_SENSOR_ACCESS);
    return d;
  }

  if( sensorParams.TEMP_AIR ){  d.temperature = bme.readTemperature();}else{d.temperature=0.0;}
  if( sensorParams.HYGR ){ d.humidity = bme.readHumidity();}else{d.humidity=0.0;}
  if( sensorParams.PRESSURE ){ d.pressure = bme.readPressure() * 0.01F;}else{d.pressure =0.0;}
  if( sensorParams.LUMIN ){ d.luminosity = analogRead(LUMINOSITY_PIN);}else{d.luminosity=0;}

  d.tempError = (d.temperature < sensorParams.MIN_TEMP_AIR || d.temperature > sensorParams.MAX_TEMP_AIR);
  d.hygrError = (d.temperature < sensorParams.HYGR_MINT || d.temperature > sensorParams.HYGR_MAXT);
  d.pressError = (d.pressure < sensorParams.PRESSURE_MIN || d.pressure > sensorParams.PRESSURE_MAX);
  d.luminError = (d.luminosity < sensorParams.LUMIN_LOW || d.luminosity > sensorParams.LUMIN_HIGH);

  if (d.tempError || d.pressError)
    LedManager_Feedback(ERROR_SENSOR_INCOHERENT);

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

bool readGPS(float &lat, float &lon) {
  while (gpsSerial.available()) {
    String line = gpsSerial.readStringUntil('\n');
    //Serial.println(line);
    
    if (line.indexOf(F("$GPGGA"))!=-1 || line.indexOf(F("$GPRMC"))!=-1) {
      if (line.indexOf(F("$GPGGA"))!=-1) line = line.substring(line.indexOf(F("$GPGGA")),line.length());
      else line = line.substring(line.indexOf(F("$GPRMC")),line.length());
      // Decouper la trame
      int index = 0;
      String fields[15];
      for (int i = 0; i < 15; i++) {
        int commaIndex = line.indexOf(',', index);
        if (commaIndex == -1) break;
        fields[i] = line.substring(index, commaIndex);
        index = commaIndex + 1;
      }

      String latStr = fields[2];
      String latDir = fields[3];
      String lonStr = fields[4];
      String lonDir = fields[5];
      if (line.indexOf(F("$GPRMC"))!=-1) {
        latStr = fields[3];
        latDir = fields[4];
        lonStr = fields[5];
        lonDir = fields[6];
      }

      if (latStr.length() > 0 && lonStr.length() > 0) {
        lat = convertToDecimal(latStr, latDir);
        lon = convertToDecimal(lonStr, lonDir);
        return true; // Succes
      }
    }
  }

  LedManager_Feedback(ERROR_GPS_ACCESS);
  return false; // echec
}
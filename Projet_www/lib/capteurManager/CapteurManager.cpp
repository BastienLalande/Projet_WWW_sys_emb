#include <Arduino.h>
#include <Adafruit_BME280.h>
#include <SoftwareSerial.h>
#include <LedManager.h>
//#include <Wire.h>

#define GPS_RX 7
#define GPS_TX 8
#define LUMINOSITY_PIN A0

// --- Objets globaux ---
SoftwareSerial gpsSerial(GPS_RX, GPS_TX);
Adafruit_BME280 bme;

bool bmeOK = false;

struct SensorParams {
  bool   TEMP_AIR;
  int8_t MIN_TEMP_AIR, MAX_TEMP_AIR;
  bool   HYGR;
  int8_t HYGR_MINT, HYGR_MAXT;
  bool   PRESSURE;
  float  PRESSURE_MIN, PRESSURE_MAX; // en hPa
  bool   LUMIN;
  uint16_t LUMIN_LOW, LUMIN_HIGH;
} sensorParams;

struct SensorData {
  float  temperature;   // °C
  float  humidity;      // %RH
  float  pressure;      // hPa
  unsigned int luminosity; // 0..1023
  bool   tempError;
  bool   hygrError;
  bool   pressError;
  bool   luminError;
};

// ------------------ Utils GPS (sans String) ------------------

// Convertit "DDMM.MMMM" / "DDDMM.MMMM" + dir ('N','S','E','W') en degrés décimaux
static float convertToDecimal_c(const char* coord, char dir) {
  if (!coord || !*coord) return 0.0f;
  double raw = atof(coord);                // ex: 4807.038
  int deg = (int)(raw / 100.0);            // 48
  double minutes = raw - (deg * 100.0);    // 7.038
  float decimal = (float)(deg + (minutes / 60.0));
  if (dir == 'S' || dir == 'W') decimal = -decimal;
  return decimal;
}

// Wrapper conservé pour compatibilité si jamais tu l’appelles ailleurs
float convertToDecimal(String coord, String dir) {
  char d = (dir.length() > 0) ? dir[0] : 0;
  return convertToDecimal_c(coord.c_str(), d);
}

// Lit une ligne NMEA (<= 82 chars) dans lineBuf; retourne true si une ligne complète a été lue
static bool readNMEALine(char* lineBuf, uint8_t maxLen) {
  static uint8_t idx = 0;

  while (gpsSerial.available()) {
    char c = (char)gpsSerial.read();
    if (c == '\r') continue;

    if (c == '\n') {
      if (idx == 0) continue; // ignore ligne vide
      lineBuf[idx] = '\0';
      idx = 0;
      return true;            // ligne complète prête
    }
    if (idx < maxLen - 1) {
      lineBuf[idx++] = c;
    } else {
      // Overflow : on remet à zéro pour ne pas dépasser le buffer
      idx = 0;
    }
  }
  return false;
}

static bool startsWith(const char* s, const char* prefix) {
  while (*prefix) { if (*s++ != *prefix++) return false; }
  return true;
}

// ------------------ Initialisation ------------------
bool init_capteur() {
  gpsSerial.begin(9600);  // 1 seule fois
  Wire.begin();

  bmeOK = bme.begin(0x76);
  if (!bmeOK) {
    Serial.println(F("Erreur : capteur BME280 non detecte !"));
    LedManager_Feedback(ERROR_SENSOR_ACCESS);
    return false;
  }

  pinMode(LUMINOSITY_PIN, INPUT);
  Serial.println(F("CapteurManager prêt"));
  return true;
}

// ------------------ Lecture capteurs ------------------
SensorData readSensors() {
  SensorData d = {};
  if (!bmeOK) {
    LedManager_Feedback(ERROR_SENSOR_ACCESS);
    return d;
  }

  // Mesures BME280
  d.temperature = bme.readTemperature();      // °C
  d.humidity    = bme.readHumidity();         // %RH
  d.pressure    = bme.readPressure() * 0.01f; // hPa
  d.luminosity  = (unsigned int)analogRead(LUMINOSITY_PIN);

  d.tempError = sensorParams.TEMP_AIR &&
    (d.temperature < sensorParams.MIN_TEMP_AIR || d.temperature > sensorParams.MAX_TEMP_AIR);
  d.hygrError = sensorParams.HYGR &&
    (d.humidity < sensorParams.HYGR_MINT || d.humidity > sensorParams.HYGR_MAXT);
  d.pressError = sensorParams.PRESSURE &&
    (d.pressure < sensorParams.PRESSURE_MIN || d.pressure > sensorParams.PRESSURE_MAX);
  d.luminError = sensorParams.LUMIN &&
    (d.luminosity < sensorParams.LUMIN_LOW || d.luminosity > sensorParams.LUMIN_HIGH);
  if (d.tempError || d.pressError) {
    LedManager_Feedback(ERROR_SENSOR_INCOHERENT);
  }

  return d;
}

// ------------------ Lecture GPS (sans String) ------------------
bool readGPS(float &lat, float &lon) {
  static char line[83];      // NMEA max ~82 + '\0'
  bool sawNMEA = false;

  // Lire toutes les lignes prêtes et ne traiter que GPGGA/GPRMC
  while (readNMEALine(line, sizeof(line))) {
    sawNMEA = true;

    if (!startsWith(line, "$GPGGA") && !startsWith(line, "$GPRMC")) {
      continue;
    }

    // Tokenisation CSV en place (remplacement des ',' par '\0')
    const int MAX_FIELDS = 16;
    const char* fields[MAX_FIELDS] = {0};
    int f = 0;
    fields[f++] = line; // 0

    for (char* p = line; *p && f < MAX_FIELDS; ++p) {
      if (*p == ',') {
        *p = '\0';
        fields[f++] = p + 1;
      }
    }

    // Indices lat/lon selon type de trame
    int latIdx, latDirIdx, lonIdx, lonDirIdx;
    if (startsWith(line, "$GPGGA")) {
      // $GPGGA, 1:time, 2:lat, 3:N/S, 4:lon, 5:E/W, ...
      latIdx = 2; latDirIdx = 3; lonIdx = 4; lonDirIdx = 5;
    } else {
      // $GPRMC, 1:time, 2:status, 3:lat, 4:N/S, 5:lon, 6:E/W, ...
      latIdx = 3; latDirIdx = 4; lonIdx = 5; lonDirIdx = 6;
    }

    if (f > lonDirIdx) {
      const char* latStr = fields[latIdx];
      const char* latDir = fields[latDirIdx];
      const char* lonStr = fields[lonIdx];
      const char* lonDir = fields[lonDirIdx];

      if (latStr && *latStr && lonStr && *lonStr && latDir && *latDir && lonDir && *lonDir) {
        float la = convertToDecimal_c(latStr, latDir[0]);
        float lo = convertToDecimal_c(lonStr, lonDir[0]);

        // On vérifie qu'on a bien des nombres valides
        if (!isnan(la) && !isnan(lo) && la != 0.0f && lo != 0.0f) {
          lat = la;
          lon = lo;
          return true;
        }
      }
    }
  }

  // Si on a vu des trames mais pas de coord valides : feedback d'erreur
  if (sawNMEA) {
    LedManager_Feedback(ERROR_GPS_ACCESS);
  }
  return false;
}
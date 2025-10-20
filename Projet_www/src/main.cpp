#include "DS1307.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SoftwareSerial.h>
#include <LedManager.h>
#include <fileManager.h>

/*PINS
  bouton rouge : 2
  bouton vert : 3
  led data : 7
  led clock : 8
  GPS : 5 et 6
  carte SD : 4 (obligatoire)
  RTC : I2C
  BME : A4-SDA et A5-SCL
*/

// --- Définition des broches ---
#define  BTN_ROUGE  2
#define BTN_VERT  3

// --- Objets matériels ---
LedManager ledManager(7, 8, 1); //datapin, clockpin, nombre de led
SoftwareSerial gpsSerial(5, 6);
Adafruit_BME280 bme;

// --- Modes de fonctionnement ---
enum Mode : uint8_t {
  MODE_ETEINT,
  MODE_STANDARD,
  MODE_CONFIG,
  MODE_MAINTENANCE,
  MODE_ECO
};

struct ModeInfo {
  uint8_t r, g, b;
  const char* msg;
};

const ModeInfo modeInfo[] = {
  {0, 0, 0,   "Mode Veille (LED éteinte)"},
  {0, 255, 0, "Mode Standard actif"},
  {255, 255, 0, "Mode Configuration actif (3 min max)"},
  {255, 165, 0, "Mode Maintenance actif"},
  {0, 0, 255, "Mode Économique actif"}
};

// --- Variables globales ---
Mode mode = MODE_ETEINT;
Mode previousMode = MODE_STANDARD;

unsigned long modeStart = 0;
unsigned long timer_rougeStart = 0;
unsigned long timer_vertStart  = 0;
bool rougeHeldActionDone = false;
bool vertHeldActionDone  = false;

volatile unsigned int secondesEcoulees = 0;
volatile unsigned int secondesData = 0;
volatile bool retourAutoFlag = false;
volatile bool aquireDataFlag = false;
volatile bool skipNextAcq = false;

const unsigned int TEMPS_RETOUR_AUTO_CONFIG = 1800;
const unsigned int LOG_INTERVAL = 10;

// --- Prototypes ---
void initPins();
void setMode(Mode newMode);
void handleModeChange();
void handleLongPress(int pin, unsigned long &timerStart, bool &heldDone, void (*callback)());
void handleDataAcquisition();
void configTimer1();
void aquireData();
String Lecture_GPS();

// --- Initialisation ---
void setup() {
  initPins();
  Serial.begin(9600);
  gpsSerial.begin(9600);
  ledManager.Init_Led();
  init_SD();


  if (!bme.begin(0x76)) {
    Serial.println("Erreur : capteur BME280 non détecté !");
    ledManager.feedback(ERROR_SENSOR_ACCESS);
    while (1) ledManager.update();
  }

  configTimer1();
  setMode(MODE_ETEINT);
  Serial.println("Système en veille - attendre appui bouton");
}

// --- Boucle principale ---
void loop() {
  ledManager.update();
  handleModeChange();

  if (mode == MODE_ETEINT) {
    if (digitalRead(BTN_ROUGE) == LOW) {
      Serial.println("Appui sur ROUGE → démarrage en mode CONFIG");
      setMode(MODE_CONFIG);
    }
    else if (digitalRead(BTN_VERT) == LOW) {
      Serial.println("Appui sur VERT → démarrage en mode STANDARD");
      setMode(MODE_STANDARD);
    }
    return;
  }

  if (retourAutoFlag) {
    retourAutoFlag = false;
    Serial.println("Fin mode configuration -> retour Standard");
    setMode(MODE_STANDARD);
  }

  if (aquireDataFlag && (mode != MODE_CONFIG && mode != MODE_ETEINT))
    handleDataAcquisition();
}

// --- Gestion des boutons longs ---
void handleModeChange() {
  handleLongPress(BTN_ROUGE, timer_rougeStart, rougeHeldActionDone, [](){
    if (mode == MODE_MAINTENANCE) setMode(previousMode);
    else { previousMode = mode; setMode(MODE_MAINTENANCE); }
  });

  handleLongPress(BTN_VERT, timer_vertStart, vertHeldActionDone, [](){
    if (mode == MODE_ECO) setMode(MODE_STANDARD);
    else if (mode == MODE_STANDARD) setMode(MODE_ECO);
  });
}

void handleLongPress(int pin, unsigned long &timerStart, bool &heldDone, void (*callback)()) {
  unsigned long now = millis();
  if (digitalRead(pin) == LOW) {
    if (timerStart == 0) timerStart = now;
    if (!heldDone && (now - timerStart > 5000)) {
      heldDone = true;
      callback();
    }
  } else {
    timerStart = 0;
    heldDone = false;
  }
}

// --- Changement de mode ---
void setMode(Mode newMode) {
  mode = newMode;
  modeStart = millis();
  secondesEcoulees = 0;
  secondesData = 0;

  const ModeInfo& info = modeInfo[newMode];
  ledManager.setColor(info.r, info.g, info.b);
  Serial.println(info.msg);

  if (newMode == MODE_MAINTENANCE) {
    Serial.print("Retournera sur ");
    Serial.println(previousMode == MODE_ECO ? "ECO" : "STANDARD");
  }
}

void initPins() {
  pinMode(BTN_ROUGE, INPUT_PULLUP);
  pinMode(BTN_VERT, INPUT_PULLUP);
}

// --- Gestion du timer ---
void configTimer1() {
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A = 15624;               // 1 seconde à 16 MHz / 1024 prescaler
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS12) | (1 << CS10);
  TIMSK1 |= (1 << OCIE1A);
  interrupts();
}

ISR(TIMER1_COMPA_vect) {
  if (mode == MODE_ETEINT) return;

  if (mode == MODE_CONFIG) {
    if (++secondesEcoulees >= TEMPS_RETOUR_AUTO_CONFIG) {
      secondesEcoulees = 0;
      retourAutoFlag = true;
    }
  } else {
    int wait_value = (mode == MODE_ECO || (mode == MODE_MAINTENANCE && previousMode == MODE_ECO)) ? LOG_INTERVAL * 2 : LOG_INTERVAL;
    if (++secondesData >= wait_value) {
      secondesData = 0;
      if (mode == MODE_ECO || (mode == MODE_MAINTENANCE && previousMode == MODE_ECO)) {
        skipNextAcq = !skipNextAcq;
        if (!skipNextAcq) aquireDataFlag = true;
      } else {
        aquireDataFlag = true;
      }
    }
  }
}

// --- Acquisition des données ---
void handleDataAcquisition() {
  aquireDataFlag = false;
  Serial.println(mode == MODE_MAINTENANCE ? "data in serial port" : "data saved in SD card");
  aquireData();
}

void aquireData() {
  int lightSensor = analogRead(A0);
  Serial.println(Lecture_GPS());
  Serial.print("Lumière : "); Serial.println(lightSensor);
  Serial.print("Température : "); Serial.print(bme.readTemperature()); Serial.println(" °C");
  Serial.print("Humidité : "); Serial.print(bme.readHumidity()); Serial.println(" %");
  Serial.print("Pression : "); Serial.print(bme.readPressure() / 100.0F); Serial.println(" hPa");
}

// --- Lecture GPS ---
String Lecture_GPS() {
  while (gpsSerial.available()) {
    String line = gpsSerial.readStringUntil('\n');
    if (line.startsWith("$GPGGA")) return line;
  }
  return "No GPS data";
}

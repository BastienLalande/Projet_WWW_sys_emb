#include <LedManager.h>
#include "CapteurManager.h"
#include <ConfigManager.h>
#include <CapteurManager.h>
#include <fileManager.h>
#include <Wire.h>

#define BTN_ROUGE 2
#define BTN_VERT 3


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


Mode mode = MODE_ETEINT;
Mode previousMode = MODE_STANDARD;
unsigned long timerRougeStart = 0, timerVertStart = 0;
bool rougeHeldDone = false, vertHeldDone = false;

volatile bool retourAutoFlag = false;
volatile bool aquireDataFlag = false;
volatile unsigned int secondesEcoulees = 0;
volatile unsigned int secondesData = 0;

const unsigned int TEMPS_RETOUR_AUTO_CONFIG = 1800;
const unsigned int LOG_INTERVAL = 10;


void initPins();
void setMode(Mode newMode);
void handleModeChange();
void handleDataAcquisition();
void configTimer1();
void handleButtons();

void setup() {
  Serial.begin(9600);
  initPins();
  LedManager_Init(5,6);
  //init_capteur();
  //init_SD();
  ConfigManager_init();
  configTimer1();
  setMode(MODE_ETEINT);
  Serial.println(F("Système en veille - attendre appui bouton"));

  Serial.print("SDA: "); Serial.println(digitalRead(SDA));
  Serial.print("SCL: "); Serial.println(digitalRead(SCL));
}

void loop() {
  LedManager_Update();
  handleButtons();

  if (mode == MODE_ETEINT) {
    if (digitalRead(BTN_ROUGE) == LOW) setMode(MODE_CONFIG);
    else if (digitalRead(BTN_VERT) == LOW) setMode(MODE_STANDARD);
    return;
  }

  if(mode == MODE_CONFIG)
  {
    ConfigManager_loop();
    if (retourAutoFlag) {
      retourAutoFlag = false;
      setMode(MODE_STANDARD);
    }

  }


  if (aquireDataFlag && mode != MODE_CONFIG && mode != MODE_ETEINT)
    handleDataAcquisition();
}

void handleButtons() {
  unsigned long now = millis();

  if (digitalRead(BTN_ROUGE) == LOW) {
    if (timerRougeStart == 0) timerRougeStart = now;
    if (!rougeHeldDone && now - timerRougeStart > 5000) 
    {
      rougeHeldDone = true;
      if (mode == MODE_MAINTENANCE) setMode(previousMode);
      else 
      { 
        previousMode = mode; 
        setMode(MODE_MAINTENANCE); 
      }
    }
  } else 
  {
    timerRougeStart = 0;
    rougeHeldDone = false;
  }

  if (digitalRead(BTN_VERT) == LOW) 
  {
    if (timerVertStart == 0) timerVertStart = now;
    if (!vertHeldDone && now - timerVertStart > 5000) 
    {
      vertHeldDone = true;
      if (mode == MODE_ECO) setMode(MODE_STANDARD);
      else if (mode == MODE_STANDARD) setMode(MODE_ECO);
    }
  } else 
  {
    timerVertStart = 0;
    vertHeldDone = false;
  }
}



void setMode(Mode newMode) {
  mode = newMode;
  secondesEcoulees = 0;
  secondesData = 0;
  const ModeInfo& info = modeInfo[newMode];
  //ledManager.setColor(info.r, info.g, info.b);
  LedManager_SetModeColor(info.r, info.g, info.b);

  Serial.println(info.msg);
}

void initPins() {
  pinMode(BTN_ROUGE, INPUT_PULLUP);
  pinMode(BTN_VERT, INPUT_PULLUP);
}

void configTimer1() {
  noInterrupts();
  TCCR1A = 0; TCCR1B = 0; TCNT1 = 0;
  OCR1A = 15624;
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
    unsigned int wait_value = (mode == MODE_ECO || (mode == MODE_MAINTENANCE && previousMode == MODE_ECO)) ? LOG_INTERVAL * 2 : LOG_INTERVAL;
    if (++secondesData >= wait_value) {
      secondesData = 0;
      aquireDataFlag = true;
    }
  }
}

void handleDataAcquisition() {
  aquireDataFlag = false;
  SensorData data = readSensors();

  if (mode == MODE_MAINTENANCE){
    Serial.println("Données (maintenance): ");
  }else{
    Serial.println("Données enregistrées: "+ String(data.humidity));
    printRoot();
  }
}

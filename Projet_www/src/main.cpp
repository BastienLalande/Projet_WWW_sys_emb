
#include <LedManager.h>
#include <CapteurManager.h>
#include <ConfigManager.h>
#include <clockManager.h>
#include <fileManager.h>
#include <Wire.h>
#include <clockManager.h>

#define BTN_ROUGE 2
#define BTN_VERT 3

//Permet d'utiliser les capteurs ou la SD sans faire deborder la RAM
#define USE_SD 0


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
  {0, 0, 0,   "[INFO] Mode Veille (LED eteinte)"},
  {0, 255, 0, "[INFO] Mode Standard actif"},
  {255, 255, 0, "[INFO] Mode Configuration actif (3 min max)"},
  {255, 80, 0, "[INFO] Mode Maintenance actif"},
  {0, 0, 255, "[INFO] Mode economique actif"}
};


Mode mode = MODE_ETEINT;
Mode previousMode = MODE_STANDARD;
unsigned long timerRougeStart = 0, timerVertStart = 0;
bool rougeHeldDone = false, vertHeldDone = false;

volatile bool retourAutoFlag = false;

volatile bool aquireDataFlag = false;

volatile unsigned int secondesData = 0;

const unsigned int LOG_INTERVAL = 10;


void initPins();
void setMode(Mode newMode);
void handleDataAcquisition();
void configTimer1();
void handleButtons();

void setup() {
  Serial.begin(9600);
  initPins();
  LedManager_Init(5,6);
  ConfigManager_init();
  configTimer1();
  init_clock();
  setMode(MODE_ETEINT);
  Serial.println(F("[INFO] Systeme en veille - attendre appui bouton"));



#if USE_SD == 1
  init_SD();
#elif USE_SD == 0
  init_capteur();
#endif



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
 
    if (retourAutoFlag) {
      retourAutoFlag = false;
      setMode(MODE_STANDARD);
    }
    else 
    {
      ConfigManager_Update();

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
  LedManager_SetModeColor(info.r, info.g, info.b);

  Serial.println(info.msg);
}

void initPins() {

  pinMode(BTN_ROUGE, INPUT_PULLUP);
  pinMode(BTN_VERT, INPUT_PULLUP);
  Serial.println(F("[INFO] Pins initialisés"));
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
    //Serial.println(("Timer tick - mode config" + String(secondesEcoulees)));
    if (++secondesEcoulees >= TEMP_RETOUR_AUTO) {
      secondesEcoulees = 0;
      retourAutoFlag = true;
    }
  } else {
    unsigned int wait_value = (mode == MODE_ECO || (mode == MODE_MAINTENANCE && previousMode == MODE_ECO)) ? LOG_INTERVAL * 4 : LOG_INTERVAL;
    if (++secondesData >= wait_value) {
      secondesData = 0;
      aquireDataFlag = true;
    }
  }
}

void handleDataAcquisition() {
  aquireDataFlag = false;
#if USE_SD == 0

  SensorData data = readSensors();
  float lat, lon;
  readGPS(lat, lon);

  if (mode == MODE_MAINTENANCE){
    Serial.println("[INFO] Donnees (maintenance): " );
    Serial.println("| Temperature :  "+String(data.temperature));
    Serial.println("| Humidite :     "+String(data.humidity));
    Serial.println("| Luminosite :   "+String(data.luminosity));
    Serial.println("| Pression :     "+String(data.pressure));
    Serial.print(F("Lat: ")); Serial.print(lat, 6);
    Serial.print(F("  Lon: ")); Serial.println(lon, 6);
  }
  else
  {
    Serial.println("[INFO] Data Sauvergardée sur la carte SD");
  }
#elif USE_SD == 1

  SensorData data;

  // Valeurs arbitraires pour simuler une aquisition de donnée
  data.temperature = 25.0;
  data.humidity = 50.0;
  data.pressure = 1013.25;
  data.luminosity = 500;

  float lat, lon;
  lat = 48.8566;
  lon = 2.3522;


  if(mode == MODE_MAINTENANCE)
  {
    String print_data = 
      "| Temperature :  "+String(data.temperature)+"\n"+
      "| Humidite :     "+String(data.humidity)+"\n"+
      "| Luminosite :   "+String(data.luminosity)+"\n"+
      "L Pression :     "+String(data.pressure)
    ;
    Serial.println("[INFO] Donnees (maintenance): \n" + print_data);
    Serial.print(F("Lat: ")); Serial.print(lat, 6);
    Serial.print(F("  Lon: ")); Serial.println(lon, 6);
  }
  else
  {
    Serial.println("saved data");
    char datachar[100];
    char tmpdata[10];

    dtostrf(data.temperature, 6, 2, tmpdata);
    strcpy(datachar, "temperature:");
    strcpy(datachar, tmpdata);
    strcpy(datachar, ";");

    dtostrf(data.humidity, 6, 2, tmpdata);
    strcpy(datachar, "humidity:");
    strcpy(datachar, tmpdata);
    strcpy(datachar, ";");

    itoa(data.luminosity, tmpdata, 10);
    strcpy(datachar, "luminosity:");
    strcpy(datachar, tmpdata);
    strcpy(datachar, ";");

    dtostrf(data.pressure, 6, 2, tmpdata);
    strcpy(datachar, "pressure:");
    strcpy(datachar, tmpdata);
    strcpy(datachar, ";");

    dtostrf(lat, 6, 2, tmpdata);
    strcpy(datachar, "lat:");
    strcpy(datachar, tmpdata);
    strcpy(datachar, "  ");
    dtostrf(lon, 6, 2, tmpdata);
    strcpy(datachar, "lon:");
    strcpy(datachar, tmpdata);
    strcpy(datachar, "/n");

    saveData(datachar);
  }


#endif
}

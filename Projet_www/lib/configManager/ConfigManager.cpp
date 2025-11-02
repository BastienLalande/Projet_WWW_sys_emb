#include "ConfigManager.h"
#include <EEPROM.h>
#include <stdlib.h>
#include <string.h>
#include <clockManager.h>

#define CMD_BUFFER 64
static char cmdBuffer[CMD_BUFFER];
unsigned int secondesEcoulees = 0;
unsigned long TEMP_RETOUR_AUTO = 60  /*Secondes*/;

Parametres params;

// --- Valeurs par defaut en memoire flash ---
const Parametres defaultParams PROGMEM = {
  10, //LOG_INTERVAL
  4096, //FILE_MAX_SIZE
  30, //TIMEOUT
  1, //LUMIN
  255, //LUMIN_LOW
  768, //LUMIN_HIGH
  1, //TEMP_AIR
  -10, //MIN_TEMP_AIR
  60, //MAX_TEMP_AIR
  1, //HYGR
  0, //HYGR_MINT
  50, //HYGR_MAXT
  1, //PRESSURE
  850, //PRESSURE_MIN
  1080 //PRESSURE_MAX
};

// --- Declarations internes ---
static void traiterCommande(char *cmd);
void ConfigManager_save();
void ConfigManager_load();
void ConfigManager_reset();

// --- Initialisation ---
void ConfigManager_init() {
  ConfigManager_load();
  Serial.println(F("[INFO] ConfigManager initialisé"));
}

// --- Boucle principale ---
void ConfigManager_Update() {
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\r') continue;

    if (c == '\n') {
      traiterCommande(cmdBuffer);
      Serial.print(F("> "));
      cmdBuffer[0] = '\0';
      secondesEcoulees = 0;
    } else {
      int len = strlen(cmdBuffer);
      if (len < CMD_BUFFER - 1) {
        cmdBuffer[len] = c;
        cmdBuffer[len + 1] = '\0';
        //Serial.print(c);
      }
    }
  }


}

// --- Commandes serie ---
static void traiterCommande(char *cmd) {
  char *arg1 = strtok(cmd, " ");
  char *arg2 = strtok(NULL, " ");
  char *arg3 = strtok(NULL, " ");

  if (!arg1) return;

  if (!strcasecmp(arg1, "set")) {
    if (!arg2 || !arg3) {
      Serial.println(F("[ERROR] Syntaxe: SET <param> <valeur>"));
      return;
    }
    int val = atoi(arg3);
    bool ok = false;

    if (!strcasecmp(arg2, "LOG_INTERVAL")) { params.LOG_INTERVAL = val; ok = true; }
    else if (!strcasecmp(arg2, "FILE_MAX_SIZE")) { params.FILE_MAX_SIZE = val; ok = true; }
    else if (!strcasecmp(arg2, "TIMEOUT")) { params.TIMEOUT = val; ok = true; }
    else if (!strcasecmp(arg2, "LUMIN")) { params.LUMIN = val; ok = true; }
    else if (!strcasecmp(arg2, "LUMIN_LOW")) { params.LUMIN_LOW = val; ok = true; }
    else if (!strcasecmp(arg2, "LUMIN_HIGH")) { params.LUMIN_HIGH = val; ok = true; }
    else if (!strcasecmp(arg2, "TEMP_AIR")) { params.TEMP_AIR = val; ok = true; }
    else if (!strcasecmp(arg2, "MIN_TEMP_AIR")) { params.MIN_TEMP_AIR = val; ok = true; }
    else if (!strcasecmp(arg2, "MAX_TEMP_AIR")) { params.MAX_TEMP_AIR = val; ok = true; }
    else if (!strcasecmp(arg2, "HYGR")) { params.HYGR = val; ok = true; }
    else if (!strcasecmp(arg2, "HYGR_MINT")) { params.HYGR_MINT = val; ok = true; }
    else if (!strcasecmp(arg2, "HYGR_MAXT")) { params.HYGR_MAXT = val; ok = true; }
    else if (!strcasecmp(arg2, "PRESSURE")) { params.PRESSURE = val; ok = true; }
    else if (!strcasecmp(arg2, "PRESSURE_MIN")) { params.PRESSURE_MIN = val; ok = true; }
    else if (!strcasecmp(arg2, "PRESSURE_MAX")) { params.PRESSURE_MAX = val; ok = true; }
    else if(!strcasecmp(arg2, "CLOCK"))
    {
      char *token1 = strtok(arg3,"-");
      char *token2 = strtok(NULL,"-");
      char *token3 = strtok(NULL,"-");
      char *token4 = strtok(NULL,"-");
      char *token5 = strtok(NULL,"-");
      char *token6 = strtok(NULL,"-");
      if(!token1 || !token2 || !token3 || !token4 || !token5 || !token6)
      {
        Serial.println(F("[ERROR] Syntaxe: SET CLOCK <YYYY-MM-DD-HH-MM-SS>"));
        return;
      }
      setupTime(
        atoi(token1),
        atoi(token2),
        atoi(token3),
        atoi(token4),
        atoi(token5),
        atoi(token6)
      );

      Serial.print(F("[INFO] Horloge mise à jour à :"));
      printTime();
      ok = true;
    }

    if (ok) 
    {
      ConfigManager_save();
      Serial.println(F("[INFO] Parametre mis à jour !"));
    } else Serial.println(F("[ERROR] Parametre inconnu !"));
  }

  else if (!strcasecmp(arg1, "get")) {
    if (!arg2) {
      Serial.println(F("[ERROR] Syntaxe: GET <param>"));
      return;
    }

    if (!strcasecmp(arg2, "LOG_INTERVAL")) Serial.println(params.LOG_INTERVAL);
    else if (!strcasecmp(arg2, "FILE_MAX_SIZE")) Serial.println(params.FILE_MAX_SIZE);
    else if (!strcasecmp(arg2, "TIMEOUT")) Serial.println(params.TIMEOUT);
    else if (!strcasecmp(arg2, "LUMIN")) Serial.println(params.LUMIN);
    else if (!strcasecmp(arg2, "LUMIN_LOW")) Serial.println(params.LUMIN_LOW);
    else if (!strcasecmp(arg2, "LUMIN_HIGH")) Serial.println(params.LUMIN_HIGH);
    else if (!strcasecmp(arg2, "TEMP_AIR")) Serial.println(params.TEMP_AIR);
    else if (!strcasecmp(arg2, "MIN_TEMP_AIR")) Serial.println(params.MIN_TEMP_AIR);
    else if (!strcasecmp(arg2, "MAX_TEMP_AIR")) Serial.println(params.MAX_TEMP_AIR);
    else if (!strcasecmp(arg2, "HYGR")) Serial.println(params.HYGR);
    else if (!strcasecmp(arg2, "HYGR_MINT")) Serial.println(params.HYGR_MINT);
    else if (!strcasecmp(arg2, "HYGR_MAXT")) Serial.println(params.HYGR_MAXT);
    else if (!strcasecmp(arg2, "PRESSURE")) Serial.println(params.PRESSURE);
    else if (!strcasecmp(arg2, "PRESSURE_MIN")) Serial.println(params.PRESSURE_MIN);
    else if (!strcasecmp(arg2, "PRESSURE_MAX")) Serial.println(params.PRESSURE_MAX);
    else Serial.println(F("[ERROR] Parametre inconnu !"));
  }

  else if (!strcasecmp(arg1, "reset")) ConfigManager_reset();
  else if (!strcasecmp(arg1, "version")) Serial.println(F("Version: 1.0"));
  else if (!strcasecmp(arg1, "params")) ConfigManager_printParams();
    else if (!strcasecmp(arg1, "exit")) {
    retourAutoFlag = true;
    Serial.println(F("[INFO] Sortie immédiate du mode configuration..."));
  }
  else Serial.println(F("[ERROR] Commande inconnue !"));
}

// --- Fonctions memoire ---
void ConfigManager_save() {
  EEPROM.put(0, params);
  Serial.println(F("[INFO] Parametres sauvegardes."));
}

void ConfigManager_load() {
  EEPROM.get(0, params);

  if (params.LOG_INTERVAL <= 0 || params.LOG_INTERVAL > 3600) {
    memcpy_P(&params, &defaultParams, sizeof(Parametres));
    ConfigManager_save();
  }
  Serial.println(F("[INFO] Parametres charges depuis EEPROM."));
}

void ConfigManager_reset() {
  memcpy_P(&params, &defaultParams, sizeof(Parametres));
  ConfigManager_save();
  Serial.println(F("[INFO] Reinitialisation terminee."));
}

void ConfigManager_printParams() {
  Serial.println(F("=== Parametres actuels ==="));
  Serial.print(F("LOG_INTERVAL: ")); Serial.println(params.LOG_INTERVAL);
  Serial.print(F("FILE_MAX_SIZE: ")); Serial.println(params.FILE_MAX_SIZE);
  Serial.print(F("TIMEOUT: ")); Serial.println(params.TIMEOUT);
  Serial.print(F("LUMIN: ")); Serial.println(params.LUMIN);
  Serial.print(F("LUMIN_LOW: ")); Serial.println(params.LUMIN_LOW);
  Serial.print(F("LUMIN_HIGH: ")); Serial.println(params.LUMIN_HIGH);
  Serial.print(F("TEMP_AIR: ")); Serial.println(params.TEMP_AIR);
  Serial.print(F("MIN_TEMP_AIR: ")); Serial.println(params.MIN_TEMP_AIR);
  Serial.print(F("MAX_TEMP_AIR: ")); Serial.println(params.MAX_TEMP_AIR);
  Serial.print(F("HYGR: ")); Serial.println(params.HYGR);
  Serial.print(F("HYGR_MINT: ")); Serial.println(params.HYGR_MINT);
  Serial.print(F("HYGR_MAXT: ")); Serial.println(params.HYGR_MAXT);
  Serial.print(F("PRESSURE: ")); Serial.println(params.PRESSURE);
  Serial.print(F("PRESSURE_MIN: ")); Serial.println(params.PRESSURE_MIN);
  Serial.print(F("PRESSURE_MAX: ")); Serial.println(params.PRESSURE_MAX);
  Serial.println(F("=========================="));
}

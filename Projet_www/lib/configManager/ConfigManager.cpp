#include "ConfigManager.h"
#include <EEPROM.h>
#include <stdlib.h>
#include <string.h>

#define CMD_BUFFER 64
static char cmdBuffer[CMD_BUFFER];
static unsigned long lastCommandTime = 0;
const unsigned long TIMEOUT_INACTIVITE = 30UL * 60UL * 1000UL;

Parametres params;

// --- Valeurs par défaut en mémoire flash ---
const Parametres defaultParams PROGMEM = {
  10, 4096, 30,
  1, 255, 768,
  1, -10, 60,
  1, 0, 50,
  1, 850, 1080
};

// --- Déclarations internes ---
static void traiterCommande(char *cmd);
void ConfigManager_save();
void ConfigManager_load();
void ConfigManager_reset();

// --- Initialisation ---
void ConfigManager_init() {
  Serial.begin(9600);
  ConfigManager_load();
  Serial.println(F("[INFO] Mode Configuration initializé"));
}

// --- Boucle principale ---
void ConfigManager_loop() {
  while (Serial.available()) {
    char c = Serial.read();

    if (c == '\r') continue;

    if (c == '\n') {
      traiterCommande(cmdBuffer);
      Serial.print(F("> "));
      cmdBuffer[0] = '\0';
      lastCommandTime = millis();
    } else {
      int len = strlen(cmdBuffer);
      if (len < CMD_BUFFER - 1) {
        cmdBuffer[len] = c;
        cmdBuffer[len + 1] = '\0';
        //Serial.print(c);
      }
    }
  }

  if (millis() - lastCommandTime > TIMEOUT_INACTIVITE) {
    Serial.println(F("\n[Inactivité] Retour au mode standard..."));
    while (true);
  }
}

// --- Commandes série ---
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

    if (ok) 
    {
      ConfigManager_save();
      Serial.println(F("[INFO] Paramètre mis à jour !"));
    } else Serial.println(F("[ERROR] Paramètre inconnu !"));
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
    else Serial.println(F("[ERROR] Paramètre inconnu !"));
  }

  else if (!strcasecmp(arg1, "reset")) ConfigManager_reset();
  else if (!strcasecmp(arg1, "version")) Serial.println(F("Version: 1.0"));
  else if (!strcasecmp(arg1, "params")) ConfigManager_printParams();
  else if (!strcasecmp(arg1, "exit")) Serial.println(F("[INFO] Sortie du mode configuration..."));
  else Serial.println(F("[ERROR] Commande inconnue !"));
}

// --- Fonctions mémoire ---
void ConfigManager_save() {
  EEPROM.put(0, params);
  Serial.println(F("[INFO] Paramètres sauvegardés."));
}

void ConfigManager_load() {
  EEPROM.get(0, params);

  if (params.LOG_INTERVAL <= 0 || params.LOG_INTERVAL > 3600) {
    memcpy_P(&params, &defaultParams, sizeof(Parametres));
    ConfigManager_save();
  }
  Serial.println(F("[INFO] Paramètres chargés depuis EEPROM."));
}

void ConfigManager_reset() {
  memcpy_P(&params, &defaultParams, sizeof(Parametres));
  ConfigManager_save();
  Serial.println(F("[INFO] Réinitialisation terminée."));
}

void ConfigManager_printParams() {
  Serial.println(F("=== Paramètres actuels ==="));
  Serial.print(F("LOG_INTERVAL: ")); Serial.println(params.LOG_INTERVAL);
  Serial.print(F("LUMIN_LOW: ")); Serial.println(params.LUMIN_LOW);
  Serial.print(F("LUMIN_HIGH: ")); Serial.println(params.LUMIN_HIGH);
  Serial.print(F("TEMP_AIR: ")); Serial.println(params.TEMP_AIR);
  Serial.println(F("=========================="));
}

#include "ConfigManager.h"
#include <EEPROM.h>
#include <avr/pgmspace.h>
#include <clockManager.h>

// ----------------------
// Réglages RAM
// ----------------------

// Plus petit mais suffisant pour: "set CLOCK YYYY-MM-DD-HH-MM-SS"
#ifndef CMD_BUFFER
  #define CMD_BUFFER 48
#endif
static char cmdBuffer[CMD_BUFFER];
static uint8_t cmdLen = 0;             // évite strlen() à chaque char

unsigned int  secondesEcoulees = 0;
unsigned long TEMP_RETOUR_AUTO = 10UL;  /* Secondes */

// Paramètres
Parametres params;

// --- Valeurs par défaut en mémoire flash ---
const Parametres defaultParams PROGMEM = {
  10,    // LOG_INTERVAL
  4096,  // FILE_MAX_SIZE
  30,    // TIMEOUT
  1,     // LUMIN
  255,   // LUMIN_LOW
  768,   // LUMIN_HIGH
  1,     // TEMP_AIR
  -10,   // MIN_TEMP_AIR
  60,    // MAX_TEMP_AIR
  1,     // HYGR
  0,     // HYGR_MINT
  50,    // HYGR_MAXT
  1,     // PRESSURE
  850,   // PRESSURE_MIN
  1080   // PRESSURE_MAX
};

// --- Déclarations internes ---
static void traiterCommande(char *cmd);
void ConfigManager_save();
void ConfigManager_load();
void ConfigManager_reset();
void ConfigManager_printParams();

// ----------------------
// Helpers sans surcoût RAM
// ----------------------

// Uppercase ASCII rapide (sans ctype)
static inline char up(char c) {
  if (c >= 'a' && c <= 'z') c -= 32;
  return c;
}

// Compare s (RAM) et p (PROGMEM), insensible à la casse
static bool iequals_P(const char* s, PGM_P p) {
  for (;;) {
    char a = up(*s++);
    char b = up((char)pgm_read_byte(p++));
    if (a != b) return false;
    if (a == '\0') return true;
  }
}

// Convertit une chaîne en int (simple, sans ctype/locale)
static int toIntFast(const char* s) {
  bool neg = false;
  int val = 0;
  if (*s == '-') { neg = true; ++s; }
  while (*s >= '0' && *s <= '9') {
    val = val * 10 + (*s - '0');
    ++s;
  }
  return neg ? -val : val;
}

// Coupe la chaîne à l’espace, renvoie début et avance *next
static char* nextToken(char*& next) {
  // skip leading spaces
  while (*next == ' ') ++next;
  if (*next == '\0') return nullptr;
  char* start = next;
  while (*next && *next != ' ') ++next;
  if (*next == ' ') { *next = '\0'; ++next; }
  return start;
}

// Parse "YYYY-MM-DD-HH-MM-SS" -> y,m,d,H,M,S (retourne true si ok)
static bool parseClockSpec(char* spec, int& Y, int& M, int& D, int& h, int& m, int& s) {
  if (!spec) return false;
  char* p = spec;
  char* t[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
  for (int i = 0; i < 6; ++i) {
    t[i] = p;
    // avance jusqu'au '-' ou fin
    while (*p && *p != '-') ++p;
    if (*p == '-') { *p = '\0'; ++p; }
  }
  if (!t[0] || !t[1] || !t[2] || !t[3] || !t[4] || !t[5]) return false;

  Y = toIntFast(t[0]);
  M = toIntFast(t[1]);
  D = toIntFast(t[2]);
  h = toIntFast(t[3]);
  m = toIntFast(t[4]);
  s = toIntFast(t[5]);
  return true;
}

// ----------------------
// Initialisation
// ----------------------
void ConfigManager_init() {
  ConfigManager_load();
  Serial.println(F("[INFO] Mode Configuration initialize"));
  Serial.print(F("> "));
}

// ----------------------
// Boucle principale
// ----------------------
void ConfigManager_Update() {
  while (Serial.available()) {
    char c = (char)Serial.read();

    if (c == '\r') continue;

    if (c == '\n') {
      // Termine la ligne et traite
      cmdBuffer[cmdLen] = '\0';
      traiterCommande(cmdBuffer);

      Serial.print(F("> "));
      cmdLen = 0;
      secondesEcoulees = 0;
    } else {
      if (cmdLen < (uint8_t)(CMD_BUFFER - 1)) {
        cmdBuffer[cmdLen++] = c;
      } else {
        // débordement -> ignorer le reste de la ligne
      }
    }
  }
}

// ----------------------
// Commandes série
// ----------------------
static void traiterCommande(char *cmd) {
  if (!cmd || !*cmd) return;

  char* cursor = cmd;
  char* arg1 = nextToken(cursor);
  char* arg2 = nextToken(cursor);
  char* arg3 = nextToken(cursor);

  if (!arg1) return;

  if (iequals_P(arg1, PSTR("set"))) {
    if (!arg2 || !arg3) {
      Serial.println(F("[ERROR] Syntaxe: SET <param> <valeur>"));
      return;
    }

    bool ok = false;

    // SET CLOCK YYYY-MM-DD-HH-MM-SS
    if (iequals_P(arg2, PSTR("CLOCK"))) {
      int Y, M, D, h, m, s;
      if (!parseClockSpec(arg3, Y, M, D, h, m, s)) {
        Serial.println(F("[ERROR] Syntaxe: SET CLOCK <YYYY-MM-DD-HH-MM-SS>"));
        return;
      }
      setupTime(Y, M, D, h, m, s);
      Serial.print(F("[INFO] Horloge mise à jour à :"));
      printTime();
      ok = true;
    } else {
      // sinon -> param numérique
      int val = toIntFast(arg3);

      if      (iequals_P(arg2, PSTR("LOG_INTERVAL")))   { params.LOG_INTERVAL   = val; ok = true; }
      else if (iequals_P(arg2, PSTR("FILE_MAX_SIZE")))  { params.FILE_MAX_SIZE  = val; ok = true; }
      else if (iequals_P(arg2, PSTR("TIMEOUT")))        { params.TIMEOUT        = val; ok = true; }
      else if (iequals_P(arg2, PSTR("LUMIN")))          { params.LUMIN          = val; ok = true; }
      else if (iequals_P(arg2, PSTR("LUMIN_LOW")))      { params.LUMIN_LOW      = val; ok = true; }
      else if (iequals_P(arg2, PSTR("LUMIN_HIGH")))     { params.LUMIN_HIGH     = val; ok = true; }
      else if (iequals_P(arg2, PSTR("TEMP_AIR")))       { params.TEMP_AIR       = val; ok = true; }
      else if (iequals_P(arg2, PSTR("MIN_TEMP_AIR")))   { params.MIN_TEMP_AIR   = val; ok = true; }
      else if (iequals_P(arg2, PSTR("MAX_TEMP_AIR")))   { params.MAX_TEMP_AIR   = val; ok = true; }
      else if (iequals_P(arg2, PSTR("HYGR")))           { params.HYGR           = val; ok = true; }
      else if (iequals_P(arg2, PSTR("HYGR_MINT")))      { params.HYGR_MINT      = val; ok = true; }
      else if (iequals_P(arg2, PSTR("HYGR_MAXT")))      { params.HYGR_MAXT      = val; ok = true; }
      else if (iequals_P(arg2, PSTR("PRESSURE")))       { params.PRESSURE       = val; ok = true; }
      else if (iequals_P(arg2, PSTR("PRESSURE_MIN")))   { params.PRESSURE_MIN   = val; ok = true; }
      else if (iequals_P(arg2, PSTR("PRESSURE_MAX")))   { params.PRESSURE_MAX   = val; ok = true; }
      else {
        Serial.println(F("[ERROR] Parametre inconnu !"));
      }
    }

    if (ok) {
      ConfigManager_save();
      Serial.println(F("[INFO] Parametre mis à jour !"));
    }
    return;
  }

  if (iequals_P(arg1, PSTR("get"))) {
    if (!arg2) {
      Serial.println(F("[ERROR] Syntaxe: GET <param>"));
      return;
    }

    if      (iequals_P(arg2, PSTR("LOG_INTERVAL")))   Serial.println(params.LOG_INTERVAL);
    else if (iequals_P(arg2, PSTR("FILE_MAX_SIZE")))  Serial.println(params.FILE_MAX_SIZE);
    else if (iequals_P(arg2, PSTR("TIMEOUT")))        Serial.println(params.TIMEOUT);
    else if (iequals_P(arg2, PSTR("LUMIN")))          Serial.println(params.LUMIN);
    else if (iequals_P(arg2, PSTR("LUMIN_LOW")))      Serial.println(params.LUMIN_LOW);
    else if (iequals_P(arg2, PSTR("LUMIN_HIGH")))     Serial.println(params.LUMIN_HIGH);
    else if (iequals_P(arg2, PSTR("TEMP_AIR")))       Serial.println(params.TEMP_AIR);
    else if (iequals_P(arg2, PSTR("MIN_TEMP_AIR")))   Serial.println(params.MIN_TEMP_AIR);
    else if (iequals_P(arg2, PSTR("MAX_TEMP_AIR")))   Serial.println(params.MAX_TEMP_AIR);
    else if (iequals_P(arg2, PSTR("HYGR")))           Serial.println(params.HYGR);
    else if (iequals_P(arg2, PSTR("HYGR_MINT")))      Serial.println(params.HYGR_MINT);
    else if (iequals_P(arg2, PSTR("HYGR_MAXT")))      Serial.println(params.HYGR_MAXT);
    else if (iequals_P(arg2, PSTR("PRESSURE")))       Serial.println(params.PRESSURE);
    else if (iequals_P(arg2, PSTR("PRESSURE_MIN")))   Serial.println(params.PRESSURE_MIN);
    else if (iequals_P(arg2, PSTR("PRESSURE_MAX")))   Serial.println(params.PRESSURE_MAX);
    else Serial.println(F("[ERROR] Parametre inconnu !"));
    return;
  }

  if (iequals_P(arg1, PSTR("reset")))   { ConfigManager_reset(); return; }
  if (iequals_P(arg1, PSTR("version"))) { Serial.println(F("Version: 1.0")); return; }
  if (iequals_P(arg1, PSTR("params")))  { ConfigManager_printParams(); return; }

  if (iequals_P(arg1, PSTR("exit"))) {
    Serial.println(F("[INFO] Sortie du mode configuration..."));
    secondesEcoulees = (uint16_t)(TEMP_RETOUR_AUTO - 1);
    return;
  }

  Serial.println(F("[ERROR] Commande inconnue !"));
}

// ----------------------
// Fonctions mémoire
// ----------------------
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
  Serial.print(F("LOG_INTERVAL: "));   Serial.println(params.LOG_INTERVAL);
  Serial.print(F("FILE_MAX_SIZE: "));  Serial.println(params.FILE_MAX_SIZE);
  Serial.print(F("TIMEOUT: "));        Serial.println(params.TIMEOUT);
  Serial.print(F("LUMIN: "));          Serial.println(params.LUMIN);
  Serial.print(F("LUMIN_LOW: "));      Serial.println(params.LUMIN_LOW);
  Serial.print(F("LUMIN_HIGH: "));     Serial.println(params.LUMIN_HIGH);
  Serial.print(F("TEMP_AIR: "));       Serial.println(params.TEMP_AIR);
  Serial.print(F("MIN_TEMP_AIR: "));   Serial.println(params.MIN_TEMP_AIR);
  Serial.print(F("MAX_TEMP_AIR: "));   Serial.println(params.MAX_TEMP_AIR);
  Serial.print(F("HYGR: "));           Serial.println(params.HYGR);
  Serial.print(F("HYGR_MINT: "));      Serial.println(params.HYGR_MINT);
  Serial.print(F("HYGR_MAXT: "));      Serial.println(params.HYGR_MAXT);
  Serial.print(F("PRESSURE: "));       Serial.println(params.PRESSURE);
  Serial.print(F("PRESSURE_MIN: "));   Serial.println(params.PRESSURE_MIN);
  Serial.print(F("PRESSURE_MAX: "));   Serial.println(params.PRESSURE_MAX);
  Serial.println(F("=========================="));
}
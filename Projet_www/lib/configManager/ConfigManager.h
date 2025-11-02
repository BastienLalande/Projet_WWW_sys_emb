#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <avr/pgmspace.h>

// --- Structure des paramètres ---
typedef struct {
  int LOG_INTERVAL;
  unsigned int FILE_MAX_SIZE;
  int TIMEOUT;

  int LUMIN;
  int LUMIN_LOW;
  int LUMIN_HIGH;

  int TEMP_AIR;
  int MIN_TEMP_AIR;
  int MAX_TEMP_AIR;

  int HYGR;
  int HYGR_MINT;
  int HYGR_MAXT;

  int PRESSURE;
  int PRESSURE_MIN;
  int PRESSURE_MAX;
} Parametres;

extern unsigned long TEMP_RETOUR_AUTO ;
extern unsigned int secondesEcoulees;
extern volatile bool retourAutoFlag;


// --- Déclaration de la variable globale ---
extern Parametres configParams;

// --- Fonctions publiques ---
void ConfigManager_init();
void ConfigManager_loop();
void ConfigManager_save();
void ConfigManager_Update();
void ConfigManager_reset();
void ConfigManager_printParams();



#endif // CONFIG_MANAGER_H

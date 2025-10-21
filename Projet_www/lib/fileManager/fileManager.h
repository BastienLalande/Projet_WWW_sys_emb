#ifndef SDMANAGER_H
#define SDMANAGER_H

#include <Arduino.h>

bool init_SD();

// Écriture dans un fichier sur la carte SD
void writeFile(const String filename, const String msg);

// Lecture du contenu d’un fichier sur la carte SD
void readFile(const String fileName);

// suppression d'un fichier
void removeFile(const String fileName);

void saveData(const String data);

void printRoot();
#endif // SDMANAGER_H

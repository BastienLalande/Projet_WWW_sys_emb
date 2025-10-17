#ifndef SDMANAGER_H
#define SDMANAGER_H

#include <Arduino.h>

void init_SD();

// Écriture dans un fichier sur la carte SD
void wrightFile(const char* fileName, const char* data);

// Lecture du contenu d’un fichier sur la carte SD
void readFile(const char* fileName);

// suppression d'un fichier
void removeFile(const char* fileName);

void printRoot();
#endif // SDMANAGER_H

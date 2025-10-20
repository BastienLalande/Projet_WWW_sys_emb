#include <SD.h>

const int chipSelect = 4;

void init_SD() {
  Serial.print(F("Initializing SD card..."));
  if (!SD.begin(chipSelect)) {
    Serial.println(F("Initialization failed!"));
    Serial.println(F("Check: card inserted, wiring, chipSelect pin."));
    while (true); // Stop execution
  }
  Serial.println(F("Initialization done."));
}

void writeFile(const char* filename, const char* msg) {
  File myFile = SD.open(filename, FILE_WRITE);
  if (myFile) {
    myFile.println(msg);
    myFile.close();
  } else {
    Serial.println(F("Error opening file for writing"));
  }
}

void readFile(const char* filename) {
  File myFile = SD.open(filename);
  if (myFile) {
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    myFile.close();
  } else {
    Serial.println(F("Error opening file for reading"));
  }
}

void printDirectory(File dir, uint8_t numTabs) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;

    for (uint8_t i = 0; i < numTabs; i++) Serial.print('\t');
    Serial.print(entry.name());

    if (entry.isDirectory()) {
      Serial.println(F("/"));
      printDirectory(entry, numTabs + 1);
    } else {
      Serial.print(F("\t\t"));
      Serial.println(entry.size());
    }
    entry.close();
  }
}

void printRoot() {
  File root = SD.open("/");
  printDirectory(root, 0);
  root.close();
}

void removeFile(const char* filename) {
  SD.remove(filename);
}

/*
Example d'usage:
  init_SD(); //necessaire pour utiliser la carte SD
  writeFile("test.txt","test 3"); // si existant ajoute du contenu sinon créer le fichier
  readFile("test.txt");
  removeFile("test.txt");
  printRoot();
*/

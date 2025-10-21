#include <SD.h>
#include <clockManager.h>
#include <ConfigManager.h>
#include <Wire.h>

#define CHIPSELECT 4

long maxFileSize = params.FILE_MAX_SIZE;

void init_SD() {
  Serial.print(F("Initializing SD card..."));
  if (!SD.begin(CHIPSELECT)) {
    Serial.println(F("Initialization failed!"));
    Serial.println(F("Check: card inserted, wiring, chipSelect pin."));
  }
  Serial.println(F("Initialization done."));
}

void writeFile(const String file, const String msg) {
  File myFile = SD.open(file, FILE_WRITE);
  if (myFile) {
    myFile.print(msg);
    myFile.close();
  } else {
    Serial.println(F("Error opening file for writing"));
  }
}

void readFile(const String file) {
  File myFile = SD.open(file);
  if (myFile) {
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    myFile.close();
  } else {
    Serial.println(F("Error opening file for reading"));
  }
}

void printDirectory(File dir, uint8_t numTabs) { // !!! fonction récursive peut remplir la pile si les répertoires sont très grand !!!
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
  File root = SD.open(F("/"));
  printDirectory(root, 0);
  root.close();
}

void createDirectory(const String dirName) {
  if (SD.mkdir(dirName)) {
    Serial.print(F("Directory created: "));
    Serial.println(dirName);
  } else {
    Serial.print(F("Failed to create directory: "));
    Serial.println(dirName);
  }
}

void removeFile(const String dirName) {// !!! fonction récursive !!!
  File dir = SD.open(dirName);
  if (!dir) {
    Serial.print(F("Directory not found: "));
    Serial.println(dirName);
    return;
  }

  if (!dir.isDirectory()) {
    SD.remove(dirName);
    dir.close();
    return;
  }

  // Supprimer tous les fichiers et sous-répertoires
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;

    String entryName = String(dirName) + "/" + entry.name();
    if (entry.isDirectory()) {
      entry.close();
      removeFile(entryName); // Appel récursif
    } else {
      entry.close();
      SD.remove(entryName);
      Serial.print(F("File removed: "));
      Serial.println(entryName);
    }
  }
  dir.close();

  // Supprimer le répertoire lui-même
  if (SD.rmdir(dirName)) {
    Serial.print(F("Directory removed: "));
    Serial.println(dirName);
  } else {
    Serial.print(F("Failed to remove directory: "));
    Serial.println(dirName);
  }
}


void saveData(const String data){
  String date = getAAMMJJ();
  for (unsigned char i = 0; i < 9; i++)
  {
    String f = date+F("_")+String(i)+F(".log");
    File myFile = SD.open(f);
    if (myFile.size()+(sizeof(data)/sizeof(data[0]))<=maxFileSize){
      writeFile(f,data);
      break;
    }
    myFile.close();
  }
}

/*
Example d'usage:
  init_SD(); //necessaire pour utiliser la carte SD
  writeFile("test.txt","test ecriture SD"); // si existant ajoute du contenu sinon créer le fichier
  readFile("test.txt");
  removeFile("test.txt");
  printRoot();
*/

#include <SD.h>
#include <clockManager.h>
#include <ConfigManager.h>

#define CHIPSELECT   4
#define SD_PATH_MAX 4096
#define IO_BUF_SIZE 64

bool init_SD() {
  if (!SD.begin(CHIPSELECT)) {
    Serial.println(F("[ERROR] Check: card inserted, wiring, chipSelect pin."));
    return false;
  }
  Serial.println(F("[INFO] FileManager initialisé"));
  return true;
}


bool writeFile(const char* path, const uint8_t* data, size_t len) {
  File f = SD.open(path, FILE_WRITE); // append si existe, sinon cree
  if (!f) {
    Serial.println(F("[ERROR] Error opening file for writing"));
    return false;
  }
  size_t written = f.write(data, len);
  f.close();
  return (written == len);
}
bool writeFile(const char* path, const char* msg) {
  return writeFile(path, reinterpret_cast<const uint8_t*>(msg), strlen(msg));
}

void readFile(const char* path) {
  File f = SD.open(path, FILE_READ);
  if (!f) {
    Serial.println(F("[ERROR] Error opening file for reading"));
    return;
  }
  uint8_t buf[IO_BUF_SIZE];
  while (true) {
    int n = f.read(buf, sizeof(buf));
    if (n <= 0) break;
    Serial.write(buf, n);
  }
  f.close();
}

void createDirectory(const char* dirName) {
  if (SD.mkdir(dirName)) {
    Serial.print(F("[INFO] Directory created: "));
    Serial.println(dirName);
  } else {
    Serial.print(F("[ERROR] Failed to create directory: "));
    Serial.println(dirName);
  }
}


void saveData(const char* data) {
  String dateStr = getAAMMJJ();           // -> ex: "250922"
  char date[8];
  dateStr.toCharArray(date, sizeof(date));

  const size_t dataLen = strlen(data);

  char path[SD_PATH_MAX];
  for (uint8_t i = 0; i < 9; i++) {
    int n = snprintf(path, sizeof(path), "%s_%u.log", date, (unsigned)i);
    if (n <= 0 || n >= (int)sizeof(path)) {
      Serial.println(F("[ERROR] Filename too long, aborting save"));
      return;
    }

    uint32_t currentSize = 0;
    File fr = SD.open(path, FILE_READ);
    if (fr) {
      currentSize = fr.size();
      fr.close();
    }

    if (currentSize + dataLen <= configParams.FILE_MAX_SIZE) {
      if (!writeFile(path, (const uint8_t*)data, dataLen)) {
        Serial.println(F("[ERROR] Write failed"));
      }
      return;
    }
  }
  Serial.println(F("[ERROR] Aucun fichier n'avait assez d'espace"));
}

/*
Exemples d’usage:
  init_SD(); // necessaire pour utiliser la carte SD
  writeFile("test.txt", "test ecriture SD\n"); // append si existe, sinon creation
  readFile("test.txt");
  createDirectory("logs");
  saveData("Hello world\n");
*/
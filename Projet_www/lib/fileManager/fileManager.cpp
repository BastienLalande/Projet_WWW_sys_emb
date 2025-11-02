#include <SD.h>
#include <clockManager.h>
#include <ConfigManager.h>

#define CHIPSELECT 4

unsigned int maxFileSize = configParams.FILE_MAX_SIZE;

bool init_SD() {
  if (!SD.begin(CHIPSELECT)) {
    Serial.println(F("[ERROR] Check: card inserted, wiring, chipSelect pin."));
    return false;
  }
  Serial.println(F("[INFO] FileManager initialisé"));
  return true;
}

bool saveData(char data[256]) {
  char date[7];
  getAAMMJJ(date);
  Serial.print("date : ");
  Serial.println(date);
  
  // Nom du fichier : AAMMJJ_0.LOG
  char name0[20];
  snprintf(name0, sizeof(name0), "%s_0.LOG", date);

  // Crée le fichier s'il n'existe pas
  if (!SD.exists(name0)) {
    File nf = SD.open(name0, FILE_WRITE);
    if (!nf) return false;
    nf.close();
  }

  // Vérifie la taille et archive si nécessaire
  {
    File f = SD.open(name0, FILE_READ);
    if (!f) return false;
    size_t sz = f.size()+256;
    f.close();

    if (sz >= maxFileSize) {
      // Cherche la prochaine révision libre
      int rev = 1;
      char nameX[20];
      for (; rev < 1000; ++rev) {
        snprintf(nameX, sizeof(nameX), "%s_%d.LOG", date, rev);
        if (!SD.exists(nameX)) break;
      }
      if (rev == 1000) return false; // sécurité

      // Copie _0.LOG -> _n.LOG
      File src = SD.open(name0, FILE_READ);
      if (!src) return false;
      File dst = SD.open(nameX, FILE_WRITE);
      if (!dst) { src.close(); return false; }

      uint8_t buf[64];
      while (src.available()) {
        int n = src.read(buf, sizeof(buf));
        if (n <= 0) break;
        if (dst.write(buf, n) != n) { src.close(); dst.close(); return false; }
      }
      src.close();
      dst.close();

      // Réinitialise _0.LOG
      SD.remove(name0);
      File nf = SD.open(name0, FILE_WRITE);
      if (!nf) return false;
      nf.close();
    }
  }

  // Écrit la ligne dans le fichier courant
  size_t len = 0;
  while (len < 256 && data[len] != '\0') ++len;

  File f = SD.open(name0, FILE_WRITE);
  if (!f) return false;

  bool ok = true;
  if (f.write((const uint8_t*)data, len) != (int)len) ok = false;

  // Ajoute un saut de ligne si absent
  if (ok) {
    char last = (len > 0) ? data[len - 1] : '\0';
    if (last != '\n' && last != '\r') {
      if (f.write('\n') != 1) ok = false;
    }
  }

  f.close();
  return ok;
}
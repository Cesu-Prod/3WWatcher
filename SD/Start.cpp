#include "SD.h"
#include "SPI.h"
#include <Arduino.h>

const int chipSelect = 4;  // Ajustez si nécessaire pour votre shield SD


void printDirectory(File dir, int numTabs) {
  while (true) {
    File entry =  dir.openNextFile();
    if (!entry) {
      // Plus de fichiers
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // Les fichiers ont des tailles, les répertoires non
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}


void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // Attendre que le port série se connecte. Nécessaire uniquement pour le port USB natif
  }

  Serial.print("Initialisation de la carte SD...");

  if (!SD.begin(chipSelect)) {
    Serial.println("échec de l'initialisation !");
    return;
  }
  Serial.println("initialisation réussie.");

  Serial.println("Contenu de la carte SD :");
  printDirectory(SD.open("/"), 0);
  Serial.println("Fin de la liste.");
}

void loop() {
  // Rien à faire ici
}


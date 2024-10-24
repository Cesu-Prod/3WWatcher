// Petit FS test.   
// For minimum flash use edit pffconfig.h and only enable
// _USE_READ and either _FS_FAT16 or _FS_FAT32




// Petit code rapide pour tester l'emuserial.

#include "Arduino.h"
#include "PF.h"

extern "C" void __attribute__((weak)) yield(void) {}


unsigned char buffer[64];         // Tableau pour stocker les données reçues sur le port série
int count = 0;                    // Compteur pour le tableau buffer

// The SD chip select pin is currently defined as 10
// in pffArduino.h.  Edit pffArduino.h to change the CS pin.

FATFS fs;     /* File system object */
//------------------------------------------------------------------------------
void errorHalt(char* msg) {
  Serial.print("Error: ");
  Serial.println(msg);
  while(1);
}
//------------------------------------------------------------------------------
void test() {
  uint8_t buf[32];
  
  // Initialize SD and file system.
  if (PF.begin(&fs)) errorHalt("pf_mount");
  
  // Open test file.
  if (PF.open("TEST.TXT")) errorHalt("pf_open");
  
  // Dump test file to Serial.
  while (1) {
    UINT nr;
    if (PF.readFile(buf, sizeof(buf), &nr)) errorHalt("pf_read");
    if (nr == 0) break;
    Serial.write(buf, nr);
  }
}
//------------------------------------------------------------------------------

// Fonction pour vider le tableau buffer
void clearBufferArray() 
{
    for (int i = 0; i < count; i++) 
    {
        buffer[i] = NULL;  // Effacer le contenu du tableau en le remplissant avec des valeurs NULL
    }}
    
    
void setup() {
  Serial.begin(9600);
  test();
  Serial.println("\nDone!");
}
void loop() {
    // Si des données sont disponibles sur le port série matériel (venant du GPS sur les broches 0 et 1)
    if (Serial.available()) 
    {
        // Lire les données dans un tableau tant qu'elles sont disponibles
        while (Serial.available()) 
        {
            buffer[count++] = Serial.read();  // Lecture des données et stockage dans le tableau buffer
            if (count == 64) break;           // Limite à 64 caractères pour éviter un débordement de mémoire
        }
        Serial.write(buffer, count);          // Afficher le contenu du buffer dans le moniteur série
        clearBufferArray();                   // Appeler la fonction pour vider le buffer
        count = 0;                            // Réinitialiser le compteur à zéro
        delay(1000);
    }
}



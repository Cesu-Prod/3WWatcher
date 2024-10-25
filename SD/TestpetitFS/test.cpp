// Petit FS test.   
// For minimum flash use edit pffconfig.h and only enable
// _USE_READ and either _FS_FAT16 or _FS_FAT32

#include "Arduino.h"
#include "PF.h"


extern "C" void __attribute__((weak)) yield(void) {}


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

void listFiles() {
    FRESULT res;
    DIR dir;
    FILINFO fno;

    // Initialize SD and file system.
    if (PF.begin(&fs)) errorHalt("pf_mount");

    // Open the root directory.
    res = PF.openDirectory(&dir, "/");
    if (res != FR_OK) errorHalt("pf_opendir");

    // Read and display each file in the directory.
    while (1) {
        res = PF.readDirectory(&dir, &fno);
        if (res != FR_OK) {
            errorHalt("pf_readdir");
        }
        if (fno.fname[0] == 0) break; // End of directory.

        // Display the file name.
        Serial.print(fno.fname);
        if (fno.fattrib & AM_DIR) {
            Serial.println(" (Directory)");
        } else {
            Serial.print(" (File, size: ");
            Serial.print(fno.fsize);
            Serial.println(" bytes)");
        }
    }
}

//------------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  test();
  listFiles();
  Serial.println("\nDone!");
}
void loop() {}
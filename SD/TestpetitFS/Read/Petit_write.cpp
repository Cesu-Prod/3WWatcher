// Petit FS test.   
// For minimum flash use edit pffconfig.h and only enable
// _USE_READ and either _FS_FAT16 or _FS_FAT32

#include "PF.h"
#include "PetitSerial.h"

extern "C" void __attribute__((weak)) yield(void) {}

#define FA_READ             0x01
#define FA_WRITE            0x02
#define FA_CREATE_NEW       0x04
#define FA_CREATE_ALWAYS    0x08
#define FA_OPEN_ALWAYS      0x10
#define FA_OPEN_APPEND      0x30


PetitSerial PS;
// Use PetitSerial instead of Serial.
#define Serial PS

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
void readtest() {
  uint8_t buf[32];
  
  // Initialize SD and file system.
  if (PF.begin(&fs)) errorHalt("pf_mount");
  
  // Open test file.
  if (PF.open("RANDOM.TXT")) errorHalt("pf_open");
  
  // Dump test file to Serial.
  while (1) {
    UINT nr;
    if (PF.readFile(buf, sizeof(buf), &nr)) errorHalt("pf_read");
    if (nr == 0) break;
    Serial.write(buf, nr);
  }
}
//------------------------------------------------------------------------------
void writetest() {
  uint8_t buf[32];
  UINT bw;
  const char* randomText = "The quick brown fox jumps over the damn lazy dogs, is a really nice sentence to write...";
  
  // Initialize SD and file system.
  if (PF.begin(&fs)) errorHalt("pf_mount");
  
  // Open test file in write mode.
  if (PF.open("RANDOM.TXT", FA_WRITE | FA_CREATE_ALWAYS)) errorHalt("pf_open");
  
  if (PF.writeFile(randomText, strlen(randomText), &bw)) errorHalt("pf_write");

  // PetitFS might not have a close method, so we skip it.
  // if (PF.close()) errorHalt("pf_close");
}
//------------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  writetest();
  readtest();
  Serial.println("\nDone!");
}
void loop() {}
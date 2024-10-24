#include "PF.h"
#include "PetitSerial.h"
#include <stdlib.h> // For rand()

extern "C" void __attribute__((weak)) yield(void) {}

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
void createRandomFile() {
  // Initialize SD and file system.
  if (PF.begin(&fs)) errorHalt("pf_mount");

  // Open file for writing.
  if (PF.open("RANDOM.TXT", FA_WRITE | FA_CREATE_ALWAYS)) errorHalt("pf_open");

  // Write random text to file.
  const char* randomText = "This is some random text.\n";
  UINT bw;
  if (PF.writeFile(randomText, strlen(randomText), &bw)) errorHalt("pf_write");

  // Close the file.
  if (PF.close()) errorHalt("pf_close");
}
//------------------------------------------------------------------------------
void readRandomFile() {
  uint8_t buf[32];
  
  // Open file for reading.
  if (PF.open("RANDOM.TXT", FA_READ)) errorHalt("pf_open");
  
  // Dump file to Serial.
  while (1) {
    UINT nr;
    if (PF.readFile(buf, sizeof(buf), &nr)) errorHalt("pf_read");
    if (nr == 0) break;
    Serial.write(buf, nr);
  }

  // Close the file.
  if (PF.close()) errorHalt("pf_close");
}
//------------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  createRandomFile();
  readRandomFile();
  Serial.println("\nDone!");
}
void loop() {}

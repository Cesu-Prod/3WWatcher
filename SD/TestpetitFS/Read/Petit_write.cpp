// Petit FS test.   
// For minimum flash use edit pffconfig.h and only enable
// _USE_READ and either _FS_FAT16 or _FS_FAT32

#include "PF.h"


extern "C" void __attribute__((weak)) yield(void) {}

#define FA_READ             0x01
#define FA_WRITE            0x02
#define FA_CREATE_NEW       0x04
#define FA_CREATE_ALWAYS    0x08
#define FA_OPEN_ALWAYS      0x10
#define FA_OPEN_APPEND      0x30


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
  if (PF.open("jsp.txt")) errorHalt("pf_open");
  
  // Dump test file to Serial.
  while (1) {
    UINT nr;
    if (PF.readFile(buf, sizeof(buf), &nr)) errorHalt("pf_read");
    if (nr == 0) break;
    Serial.write(buf, nr);
  }
}
//------------------------------------------------------------------------------
void createFile(const char* filename) {
  char fullFilename[20];
  sprintf(fullFilename, "%s.txt", filename);
  
  // Initialize SD and file system.
  if (PF.begin(&fs)) errorHalt("pf_mount");
  
  // Open test file in write mode.
  if (PF.open(fullFilename, FA_WRITE | FA_CREATE_ALWAYS)) errorHalt("pf_open");
}

void addTextToFile(const char* filename, const char* text) {
  char fullFilename[20];
  sprintf(fullFilename, "%s.txt", filename);
  
  // Initialize SD and file system.
  if (PF.begin(&fs)) errorHalt("pf_mount");
  
  // Open test file in append mode.
  if (PF.open(fullFilename, FA_WRITE | FA_OPEN_APPEND)) errorHalt("pf_open");
  
  UINT bw;
  if (PF.writeFile(text, strlen(text), &bw)) errorHalt("pf_write");
}
//------------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  createFile("jsp.txt");
  readtest();
  Serial.println("Adding \"IDK MAN\" to file ")
  addTextToFile("jsp.txt", "IDK MAN")
  readtest();
  Serial.println("\nDone!");
}
void loop() {}
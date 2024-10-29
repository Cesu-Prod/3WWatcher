// Petit FS test.   
// For minimum flash use edit pffconfig.h and only enable
// _USE_READ and either _FS_FAT16 or _FS_FAT32

#include "PF.h"
#include "Arduino.h"

extern "C" void __attribute__((weak)) yield(void) {}

FATFS fs;    // File system object
FIL file;    // File object

// Initialize SD card with PetitFS
bool initializeSD() {
  if (pf_mount(&fs) != FR_OK) {
    return false;
  }
  return true;
}

// Create a text file
bool createFile(const char* filename) {
  FRESULT res;
  
  // Open file with write permissions (will create if doesn't exist)
  res = pf_open(filename);
  if (res != FR_OK) {
    return false;
  }
  
  // Close the file
  return true;
}

// Write to text file
bool writeToFile(const char* filename, const char* data) {
  FRESULT res;
  UINT written;
  
  // Open file
  res = pf_open(filename);
  if (res != FR_OK) {
    return false;
  }
  
  // Write data
  res = pf_write(data, strlen(data), &written);
  if (res != FR_OK || written != strlen(data)) {
    return false;
  }
  
  // Finalize write operation
  res = pf_write(0, 0, &written);
  return (res == FR_OK);
}

// Get file size
unsigned long getFileSize(const char* filename) {
  FRESULT res;
  
  // Open file
  res = pf_open(filename);
  if (res != FR_OK) {
    return 0;
  }
  
  // Return file size
  return file.fsize;
}

// Rename file (Note: PetitFS doesn't have direct rename, we need to simulate it)
bool renameFile(const char* oldName, const char* newName) {
  FRESULT res;
  UINT bytesRead, bytesWritten;
  char buffer[32];  // Buffer for copying
  
  // Open source file
  res = pf_open(oldName);
  if (res != FR_OK) {
    return false;
  }
  
  // Create destination file
  res = pf_open(newName);
  if (res != FR_OK) {
    return false;
  }
  
  // Copy data in chunks
  while (1) {
    res = pf_read(buffer, sizeof(buffer), &bytesRead);
    if (res != FR_OK || bytesRead == 0) break;
    
    res = pf_write(buffer, bytesRead, &bytesWritten);
    if (res != FR_OK || bytesRead != bytesWritten) {
      return false;
    }
  }
  
  // Finalize write
  res = pf_write(0, 0, &bytesWritten);
  
  // Note: PetitFS doesn't support delete, so old file will remain
  return (res == FR_OK);
}

// Get available space (Note: PetitFS doesn't provide direct free space checking)
unsigned long getAvailableSpace() {
  // PetitFS doesn't provide a direct way to check free space
  // This is a simplified version that returns a constant
  return 0xFFFFFFFF;  // Returns max value to indicate unknown
}

// Disconnect SD card
void disconnectSD() {
  // PetitFS doesn't require explicit unmounting
  // But we can reset SPI pins if needed
  pinMode(SS, INPUT);
  SPCR = 0;
  SPSR = 0;
}
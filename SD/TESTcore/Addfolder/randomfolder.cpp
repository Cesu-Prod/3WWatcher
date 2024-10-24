#include "SDCore.h"
#include "Arduino.h"
#include <stdlib.h>

extern "C" void __attribute__((weak)) yield(void) {}

struct FAT32BootSector {
  byte jmp[3];
  char oem[8];
  uint16_t bytesPerSector;
  uint8_t sectorsPerCluster;
  uint16_t reservedSectors;
  uint8_t numberOfFATs;
  uint16_t rootEntries;          
  uint16_t totalSectors16;       
  uint8_t mediaDescriptor;
  uint16_t sectorsPerFAT16;      
  uint16_t sectorsPerTrack;
  uint16_t numberOfHeads;
  uint32_t hiddenSectors;
  uint32_t totalSectors32;
  uint32_t sectorsPerFAT32;
  uint16_t flags;
  uint16_t version;
  uint32_t rootCluster;
  uint16_t fsInfo;
  uint16_t backupBoot;
  byte reserved[12];
  uint8_t driveNumber;
  uint8_t reserved1;
  uint8_t bootSignature;
  uint32_t volumeID;
  char volumeLabel[11];
  char fileSystemType[8];
} __attribute__((packed));

struct DirectoryEntry {
  char name[8];
  char ext[3];
  uint8_t attributes;
  uint8_t reserved;
  uint8_t creationTimeTs;
  uint16_t creationTime;
  uint16_t creationDate;
  uint16_t lastAccessDate;
  uint16_t firstClusterHigh;
  uint16_t lastModTime;
  uint16_t lastModDate;
  uint16_t firstClusterLow;
  uint32_t fileSize;
} __attribute__((packed));

class SDCardWriter {
private:
  FAT32BootSector bootSector;
  uint32_t dataStart;
  uint32_t rootDirStart;
  byte buffer[512];

  bool readBootSector() {
    if (!SDCore::read(0, buffer)) {
      return false;
    }
    memcpy(&bootSector, buffer, sizeof(FAT32BootSector));
    dataStart = bootSector.reservedSectors + (bootSector.numberOfFATs * bootSector.sectorsPerFAT32);
    rootDirStart = dataStart + ((bootSector.rootCluster - 2) * bootSector.sectorsPerCluster);
    return true;
  }

public:
  bool begin() {
    if (!SDCore::begin()) {
      Serial.println("Failed to initialize SD card");
      return false;
    }
    if (!readBootSector()) {
      Serial.println("Failed to read boot sector");
      return false;
    }
    return true;
  }

  void writeRandomFile() {
    char fileName[13];
    sprintf(fileName, "FILE%d.TXT", random(1000, 9999));

    Serial.print("Writing file: ");
    Serial.println(fileName);

    // Fill buffer with random data
    for (int i = 0; i < 512; i++) {
      buffer[i] = random(0, 256);
    }

    // Writing random data to SD card at the first available sector
    if (!SDCore::write(rootDirStart, buffer)) {
      Serial.println("Error writing file");
      return;
    }

    Serial.println("File written successfully.");
  }

  void end() {
    SDCore::end();
  }
};

void setup() {
  Serial.begin(250000);
  while (!Serial) {
    ; // Wait for Serial port to connect
  }
  
  Serial.println("Initializing SD card writer...");
  

}

void loop() {
  SDCardWriter writer;
  if (writer.begin()) {
    writer.writeRandomFile();
    writer.end();
  }
  
  Serial.println("\nDone.");
}

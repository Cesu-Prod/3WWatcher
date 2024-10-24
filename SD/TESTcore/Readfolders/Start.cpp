#include "SDCore.h"
#include "Arduino.h"

extern "C" void __attribute__((weak)) yield(void) {}

struct FAT32BootSector {
  byte jmp[3];
  char oem[8];
  uint16_t bytesPerSector;
  uint8_t sectorsPerCluster;
  uint16_t reservedSectors;
  uint8_t numberOfFATs;
  uint16_t rootEntries;          // Always 0 for FAT32
  uint16_t totalSectors16;       // Always 0 for FAT32
  uint8_t mediaDescriptor;
  uint16_t sectorsPerFAT16;      // Always 0 for FAT32
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

class SDCardReader {
private:
  FAT32BootSector bootSector;
  uint32_t fatStart;
  uint32_t dataStart;
  uint32_t rootDirStart;
  byte buffer[512];

  bool readBootSector() {
    if (!SDCore::read(0, buffer)) {
      return false;
    }
    memcpy(&bootSector, buffer, sizeof(FAT32BootSector));
    
    // Calculate important addresses
    fatStart = bootSector.reservedSectors;
    dataStart = fatStart + (bootSector.numberOfFATs * bootSector.sectorsPerFAT32);
    rootDirStart = dataStart + ((bootSector.rootCluster - 2) * bootSector.sectorsPerCluster);
    
    return true;
  }

  void printFileName(DirectoryEntry* entry) {
    char filename[13];
    int j = 0;
    
    // Copy name (removing spaces)
    for (int i = 0; i < 8; i++) {
      if (entry->name[i] != ' ') {
        filename[j++] = entry->name[i];
      }
    }
    
    // Add extension if it exists
    if (entry->ext[0] != ' ') {
      filename[j++] = '.';
      for (int i = 0; i < 3; i++) {
        if (entry->ext[i] != ' ') {
          filename[j++] = entry->ext[i];
        }
      }
    }
    
    filename[j] = '\0';
    
    // Print file details
    Serial.print(filename);
    if (entry->attributes & 0x10) {  // Directory
      Serial.println("/");
    } else {
      Serial.print("\t\t");
      Serial.print(entry->fileSize);
      Serial.println(" bytes");
    }
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

  void listFiles() {
    Serial.println("\nFiles on SD card:");
    Serial.println("----------------");
    
    // Read root directory
    uint32_t currentSector = rootDirStart;
    for (int sector = 0; sector < bootSector.sectorsPerCluster; sector++) {
      if (!SDCore::read(currentSector + sector, buffer)) {
        Serial.println("Error reading directory");
        return;
      }
      
      DirectoryEntry* entry = (DirectoryEntry*)buffer;
      for (int i = 0; i < 16; i++) {  // 16 entries per sector
        // Check if entry is valid
        if (entry[i].name[0] == 0x00) {  // End of directory
          return;
        }
        if (entry[i].name[0] != 0xE5 && entry[i].attributes != 0x0F) {  // Skip deleted and LFN entries
          printFileName(&entry[i]);
        }
      }
    }
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
  
  Serial.println("Initializing SD card reader...");
  
  SDCardReader reader;
  if (reader.begin()) {
    reader.listFiles();
    reader.end();
  }
  
  Serial.println("\nDone.");
}

void loop() {
  // Empty loop
}
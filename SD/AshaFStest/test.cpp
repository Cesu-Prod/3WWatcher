#include "Arduino.h"
#include "SPI.h"


extern "C" void __attribute__((weak)) yield(void) {} 

// Pin definitions
const int SD_CS_PIN = 4;  // Chip Select pin

// Status codes
#define SUCCESS 0
#define ERROR 1

// SD card commands
#define CMD0    0x40  // GO_IDLE_STATE - reset card to idle state
#define CMD1    0x41  // SEND_OP_COND - initialize card
#define CMD8    0x48  // SEND_IF_COND - verify SD Memory Card interface operating condition
#define CMD13   0x4D  // SEND_STATUS - read card status register
#define CMD16   0x50  // SET_BLOCKLEN
#define CMD17   0x51  // READ_SINGLE_BLOCK - read one block
#define CMD24   0x58  // WRITE_BLOCK - write one block
#define CMD55   0x77  // APP_CMD - prefix for ACMD commands
#define CMD58   0x7A  // READ_OCR - read OCR register
#define ACMD41  0x69  // SD_SEND_OP_COND - send host capacity support

#define BLOCK_SIZE 512

// MBR and partition structures
struct __attribute__((packed)) PartitionEntry {
    uint8_t  boot_flag;          // Boot indicator (0x80 = bootable, 0x00 = not bootable)
    uint8_t  start_chs[3];       // Starting CHS address
    uint8_t  partition_type;     // Partition type (0x0B or 0x0C for FAT32)
    uint8_t  end_chs[3];         // Ending CHS address
    uint32_t start_sector;       // Starting sector (LBA)
    uint32_t total_sectors;      // Total sectors in partition
};

struct __attribute__((packed)) MBR {
    uint8_t         bootstrap[446];  // Boot code
    PartitionEntry  partitions[4];   // Partition entries
    uint16_t        signature;       // Boot signature (0xAA55)
};

// FAT32 structures
struct __attribute__((packed)) Fat32BootSector {
    uint8_t  jump_boot[3];       // Jump instruction to boot code
    char     oem_name[8];        // OEM Name (padded with spaces)
    uint16_t bytes_per_sector;   // Bytes per sector (usually 512)
    uint8_t  sectors_per_cluster; // Sectors per cluster
    uint16_t reserved_sectors;   // Reserved sector count
    uint8_t  fat_count;         // Number of FATs (usually 2)
    uint16_t root_entries;      // Number of root directory entries (FAT32 = 0)
    uint16_t total_sectors_16;  // Total sectors (FAT32 = 0)
    uint8_t  media_type;        // Media type
    uint16_t sectors_per_fat_16; // Sectors per FAT (FAT32 = 0)
    uint16_t sectors_per_track; // Sectors per track
    uint16_t head_count;        // Number of heads
    uint32_t hidden_sectors;    // Hidden sectors
    uint32_t total_sectors_32;  // Total sectors (if total_sectors_16 = 0)
    uint32_t sectors_per_fat_32; // Sectors per FAT
    uint16_t ext_flags;        // FAT flags
    uint16_t fs_version;       // Filesystem version
    uint32_t root_cluster;     // First cluster of root directory
    uint16_t fs_info;         // FSInfo sector number
    uint16_t backup_boot;     // Backup boot sector
    uint8_t  reserved[12];    // Reserved
    uint8_t  drive_number;    // Drive number
    uint8_t  reserved1;       // Reserved
    uint8_t  boot_signature;  // Extended boot signature
    uint32_t volume_id;       // Volume serial number
    char     volume_label[11]; // Volume label
    char     fs_type[8];      // Filesystem type ("FAT32   ")
    uint8_t  boot_code[420];  // Boot code
    uint16_t boot_signature2; // Boot signature (0xAA55)
};

struct __attribute__((packed)) DirEntry {
    char     name[8];         // File name
    char     ext[3];          // File extension
    uint8_t  attributes;      // File attributes
    uint8_t  reserved;       // Reserved
    uint8_t  create_time_ms; // Creation time (milliseconds)
    uint16_t create_time;    // Creation time
    uint16_t create_date;    // Creation date
    uint16_t access_date;    // Last access date
    uint16_t cluster_high;   // High word of first cluster
    uint16_t modify_time;    // Last modify time
    uint16_t modify_date;    // Last modify date
    uint16_t cluster_low;    // Low word of first cluster
    uint32_t size;           // File size
};

// Global variables
uint32_t partitionStart = 0;
uint32_t fatStart = 0;
uint32_t dataStart = 0;
uint32_t rootCluster = 0;
uint32_t clusterSize = 0;

// Basic SD card functions
bool waitReady(uint16_t timeoutMillis) {
    uint8_t response;
    uint32_t startMillis = millis();
    do {
        response = SPI.transfer(0xFF);
        if (response == 0xFF) return true;
    } while ((millis() - startMillis) < timeoutMillis);
    return false;
}

uint8_t sendCommand(uint8_t cmd, uint32_t arg) {
    waitReady(300);
    
    digitalWrite(SD_CS_PIN, LOW);
    
    // Send command
    SPI.transfer(cmd);
    SPI.transfer(arg >> 24);
    SPI.transfer(arg >> 16);
    SPI.transfer(arg >> 8);
    SPI.transfer(arg);
    
    // Send CRC
    uint8_t crc = 0xFF;
    if (cmd == CMD0) crc = 0x95;  // Valid CRC for CMD0
    if (cmd == CMD8) crc = 0x87;  // Valid CRC for CMD8
    SPI.transfer(crc);
    
    // Wait for response
    uint8_t response;
    for (int i = 0; i < 10; i++) {
        response = SPI.transfer(0xFF);
        if (response != 0xFF) break;
    }
    
    return response;
}

// Initialize SD card
uint8_t initSDCard() {
    Serial.println(F("Initializing SD card..."));
    
    // Set CS high and send dummy bytes
    digitalWrite(SD_CS_PIN, HIGH);
    for(int i = 0; i < 10; i++) {
        SPI.transfer(0xFF);
    }
    
    // Enter SPI mode
    int attempts = 0;
    uint8_t response;
    do {
        response = sendCommand(CMD0, 0);
        attempts++;
        if(attempts > 10) {
            Serial.println(F("Failed to enter SPI mode"));
            return ERROR;
        }
    } while(response != 0x01);
    
    // Check SD version
    response = sendCommand(CMD8, 0x1AA);
    if(response == 0x01) {
        // SDv2 card
        uint32_t r7 = SPI.transfer(0xFF) << 24;
        r7 |= SPI.transfer(0xFF) << 16;
        r7 |= SPI.transfer(0xFF) << 8;
        r7 |= SPI.transfer(0xFF);
        
        if((r7 & 0xFFF) != 0x1AA) {
            Serial.println(F("Invalid voltage range"));
            return ERROR;
        }
        
        // Initialize card
        attempts = 0;
        do {
            sendCommand(CMD55, 0);
            response = sendCommand(ACMD41, 0x40000000);
            attempts++;
            if(attempts > 100) {
                Serial.println(F("Card initialization failed"));
                return ERROR;
            }
        } while(response != 0x00);
    }
    
    // Set block size to 512
    if(sendCommand(CMD16, 512) != 0x00) {
        Serial.println(F("Failed to set block size"));
        return ERROR;
    }
    
    // Switch to full speed
    SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
    
    Serial.println(F("SD card initialized successfully"));
    return SUCCESS;
}

// Read a block from SD card
uint8_t readBlock(uint32_t blockNumber, uint8_t *buffer) {
    if(sendCommand(CMD17, blockNumber) != 0x00) {
        return ERROR;
    }
    
    // Wait for data token
    uint8_t token;
    uint16_t timeout = 0;
    do {
        token = SPI.transfer(0xFF);
        timeout++;
    } while(token == 0xFF && timeout < 10000);
    
    if(token != 0xFE) {
        return ERROR;
    }
    
    // Read data
    for(uint16_t i = 0; i < 512; i++) {
        buffer[i] = SPI.transfer(0xFF);
    }
    
    // Read and discard CRC
    SPI.transfer(0xFF);
    SPI.transfer(0xFF);
    
    digitalWrite(SD_CS_PIN, HIGH);
    return SUCCESS;
}

// Write a block to SD card
uint8_t writeBlock(uint32_t blockNumber, const uint8_t *buffer) {
    if(sendCommand(CMD24, blockNumber) != 0x00) {
        return ERROR;
    }
    
    // Send data token
    SPI.transfer(0xFE);
    
    // Write data
    for(uint16_t i = 0; i < 512; i++) {
        SPI.transfer(buffer[i]);
    }
    
    // Send dummy CRC
    SPI.transfer(0xFF);
    SPI.transfer(0xFF);
    
    // Check response
    uint8_t response = SPI.transfer(0xFF);
    if((response & 0x1F) != 0x05) {
        return ERROR;
    }
    
    // Wait for write to complete
    if(!waitReady(500)) {
        return ERROR;
    }
    
    digitalWrite(SD_CS_PIN, HIGH);
    return SUCCESS;
}

// Initialize FAT32 filesystem
bool initializeFAT32() {
    uint8_t buffer[BLOCK_SIZE];
    
    // Read MBR
    if(readBlock(0, buffer) != SUCCESS) {
        Serial.println(F("Failed to read MBR"));
        return false;
    }
    
    MBR *mbr = (MBR*)buffer;
    if(mbr->signature != 0xAA55) {
        Serial.println(F("Invalid MBR signature"));
        return false;
    }
    
    // Find FAT32 partition
    bool found = false;
    for(int i = 0; i < 4; i++) {
        if(mbr->partitions[i].partition_type == 0x0B || 
           mbr->partitions[i].partition_type == 0x0C) {
            partitionStart = mbr->partitions[i].start_sector;
            found = true;
            break;
        }
    }
    
    if(!found) {
        Serial.println(F("No FAT32 partition found"));
        return false;
    }
    
    // Read boot sector
    if(readBlock(partitionStart, buffer) != SUCCESS) {
        Serial.println(F("Failed to read boot sector"));
        return false;
    }
    
    Fat32BootSector *boot = (Fat32BootSector*)buffer;
    if(boot->boot_signature2 != 0xAA55) {
        Serial.println(F("Invalid boot sector signature"));
        return false;
    }
    
    // Calculate filesystem parameters
    fatStart = partitionStart + boot->reserved_sectors;
    rootCluster = boot->root_cluster;
    clusterSize = boot->sectors_per_cluster * boot->bytes_per_sector;
    dataStart = fatStart + (boot->fat_count * boot->sectors_per_fat_32);
    
    Serial.println(F("FAT32 filesystem initialized"));
    Serial.print(F("Partition start: ")); Serial.println(partitionStart);
    Serial.print(F("FAT start: ")); Serial.println(fatStart);
    Serial.print(F("Data start: ")); Serial.println(dataStart);
    Serial.print(F("Root cluster: ")); Serial.println(rootCluster);
    Serial.print(F("Cluster size: ")); Serial.println(clusterSize);
    
    return true;
}

// Convert cluster number to sector number
uint32_t clusterToSector(uint32_t cluster) {
    return dataStart + ((cluster - 2) * clusterSize / BLOCK_SIZE);
}

// Add these functions to the previous code:

// Find first free cluster
uint32_t findFreeCluster() {
    uint8_t buffer[BLOCK_SIZE];
    uint32_t cluster = 2;  // First usable cluster
    
    // Read through FAT
    for(uint32_t fatSector = 0; fatSector < 32; fatSector++) {  // Read first 32 FAT sectors
        if(readBlock(fatStart + fatSector, buffer) != SUCCESS) {
            return 0;
        }
        
        // Check each entry in this sector
        for(uint16_t i = 0; i < BLOCK_SIZE; i += 4) {
            uint32_t value = buffer[i] | (buffer[i + 1] << 8) | 
                            (buffer[i + 2] << 16) | (buffer[i + 3] << 24);
            
            if(value == 0) {
                return cluster;
            }
            cluster++;
        }
    }
    
    return 0;  // No free clusters found
}

// Find a free directory entry in root directory
uint32_t findFreeDirEntry() {
    uint8_t buffer[BLOCK_SIZE];
    uint32_t sector = clusterToSector(rootCluster);
    
    // Read root directory sector
    if(readBlock(sector, buffer) != SUCCESS) {
        return 0;
    }
    
    // Look for free entry
    for(uint16_t i = 0; i < BLOCK_SIZE; i += 32) {
        if(buffer[i] == 0x00 || buffer[i] == 0xE5) {
            return i;  // Return offset within sector
        }
    }
    
    return 0xFFFFFFFF;  // No free entry found
}

// Create a new file
bool createFile(const char* filename) {
    uint8_t buffer[BLOCK_SIZE];
    
    // Find free cluster for file data
    uint32_t cluster = findFreeCluster();
    if(cluster == 0) {
        Serial.println(F("No free clusters available"));
        return false;
    }
    
    // Find free directory entry
    uint32_t dirEntry = findFreeDirEntry();
    if(dirEntry == 0xFFFFFFFF) {
        Serial.println(F("No free directory entries"));
        return false;
    }
    
    // Read root directory sector
    uint32_t rootSector = clusterToSector(rootCluster);
    if(readBlock(rootSector, buffer) != SUCCESS) {
        Serial.println(F("Failed to read root directory"));
        return false;
    }
    
    // Create directory entry
    DirEntry* entry = (DirEntry*)(buffer + dirEntry);
    memset(entry, 0, sizeof(DirEntry));
    
    // Set filename (convert to 8.3 format)
    memset(entry->name, ' ', 8);
    memset(entry->ext, ' ', 3);
    
    // Copy name
    int nameLen = 0;
    for(; nameLen < 8 && filename[nameLen] && filename[nameLen] != '.'; nameLen++) {
        entry->name[nameLen] = toupper(filename[nameLen]);
    }
    
    // Copy extension if exists
    const char* dot = strchr(filename, '.');
    if(dot) {
        for(int i = 0; i < 3 && dot[i+1]; i++) {
            entry->ext[i] = toupper(dot[i+1]);
        }
    }
    
    // Set other directory entry fields
    entry->attributes = 0x20;  // Archive bit
    entry->cluster_high = cluster >> 16;
    entry->cluster_low = cluster & 0xFFFF;
    entry->size = 0;  // Empty file initially
    
    // Current date and time
    entry->create_time = 0;
    entry->create_date = 0;
    entry->modify_time = 0;
    entry->modify_date = 0;
    
    // Write directory entry back
    if(writeBlock(rootSector, buffer) != SUCCESS) {
        Serial.println(F("Failed to write directory entry"));
        return false;
    }
    
    // Mark cluster as end of chain in FAT
    uint32_t fatSector = fatStart + ((cluster * 4) / BLOCK_SIZE);
    uint32_t fatOffset = (cluster * 4) % BLOCK_SIZE;
    
    if(readBlock(fatSector, buffer) != SUCCESS) {
        Serial.println(F("Failed to read FAT"));
        return false;
    }
    
    // Mark as end of chain (0x0FFFFFFF)
    buffer[fatOffset] = 0xFF;
    buffer[fatOffset + 1] = 0xFF;
    buffer[fatOffset + 2] = 0xFF;
    buffer[fatOffset + 3] = 0x0F;
    
    if(writeBlock(fatSector, buffer) != SUCCESS) {
        Serial.println(F("Failed to write FAT"));
        return false;
    }
    
    Serial.print(F("File created successfully: "));
    Serial.println(filename);
    return true;
}

bool appendToFile(const char* filename, const char* data) {
    uint8_t dirBuffer[BLOCK_SIZE];
    uint8_t fileBuffer[BLOCK_SIZE];
    uint32_t rootSector = clusterToSector(rootCluster);
    
    Serial.print(F("\nLooking for file: "));
    Serial.println(filename);
    
    // Read root directory
    if(readBlock(rootSector, dirBuffer) != SUCCESS) {
        Serial.println(F("Failed to read root directory"));
        return false;
    }
    
    // Prepare 8.3 format name for search
    char fname[9] = "        ";  // 8 spaces
    char fext[4] = "   ";      // 3 spaces
    
    // Split filename into name and extension
    const char* dot = strchr(filename, '.');
    size_t nameLen = dot ? (dot - filename) : strlen(filename);
    
    // Copy name part (up to 8 chars)
    for(size_t i = 0; i < 8 && i < nameLen; i++) {
        fname[i] = toupper(filename[i]);
    }
    
    // Copy extension if present (up to 3 chars)
    if(dot && dot[1]) {
        for(size_t i = 0; i < 3 && dot[i + 1]; i++) {
            fext[i] = toupper(dot[i + 1]);
        }
    }
    
    Serial.println(F("Directory entries:"));
    
    // Find matching file
    DirEntry* entry = nullptr;
    uint16_t entryOffset = 0;
    
    for(uint16_t i = 0; i < BLOCK_SIZE; i += sizeof(DirEntry)) {
        DirEntry* current = (DirEntry*)(dirBuffer + i);
        
        // Skip empty or deleted entries
        if(current->name[0] == 0x00 || current->name[0] == 0xE5) {
            continue;
        }
        
        // Debug print each entry
        Serial.print(F("Entry found - Name: '"));
        for(int j = 0; j < 8; j++) {
            Serial.print((char)current->name[j]);
        }
        Serial.print(F("' Ext: '"));
        for(int j = 0; j < 3; j++) {
            Serial.print((char)current->ext[j]);
        }
        Serial.println(F("'"));
        
        // Compare name and extension
        if(memcmp(current->name, fname, 8) == 0 && 
           memcmp(current->ext, fext, 3) == 0) {
            entry = current;
            entryOffset = i;
            Serial.println(F("Match found!"));
            break;
        }
    }
    
    if(!entry) {
        Serial.println(F("File not found in directory"));
        Serial.print(F("Was looking for name: '"));
        Serial.print(fname);
        Serial.print(F("' ext: '"));
        Serial.print(fext);
        Serial.println(F("'"));
        return false;
    }
    
    // Get file's cluster and read data
    uint32_t cluster = (entry->cluster_high << 16) | entry->cluster_low;
    uint32_t sector = clusterToSector(cluster);
    
    if(readBlock(sector, fileBuffer) != SUCCESS) {
        Serial.println(F("Failed to read file content"));
        return false;
    }
    
    // Append new data
    uint32_t currentSize = entry->size;
    uint32_t dataLen = strlen(data);
    uint32_t newSize = currentSize + dataLen;
    
    if(newSize > BLOCK_SIZE) {
        Serial.println(F("File would exceed maximum size"));
        return false;
    }
    
    // Copy new data to file buffer
    memcpy(fileBuffer + currentSize, data, dataLen);
    
    // Write updated file content
    if(writeBlock(sector, fileBuffer) != SUCCESS) {
        Serial.println(F("Failed to write updated file content"));
        return false;
    }
    
    // Update directory entry
    entry->size = newSize;
    
    // Update modification time
    entry->modify_time = 0;
    entry->modify_date = 0;
    
    // Write updated directory entry
    if(writeBlock(rootSector, dirBuffer) != SUCCESS) {
        Serial.println(F("Failed to update directory entry"));
        return false;
    }
    
    Serial.println(F("File updated successfully!"));
    return true;
}

uint32_t getFileSize(const char* filename) {
    uint8_t dirBuffer[BLOCK_SIZE];
    uint32_t rootSector = clusterToSector(rootCluster);
    
    Serial.print(F("\nChecking size of file: "));
    Serial.println(filename);
    
    // Read root directory
    if(readBlock(rootSector, dirBuffer) != SUCCESS) {
        Serial.println(F("Failed to read root directory"));
        return 0xFFFFFFFF;  // Return max value to indicate error
    }
    
    // Prepare 8.3 format name for search
    char fname[9] = "        ";  // 8 spaces
    char fext[4] = "   ";      // 3 spaces
    
    // Split filename into name and extension
    const char* dot = strchr(filename, '.');
    size_t nameLen = dot ? (dot - filename) : strlen(filename);
    
    // Copy name part (up to 8 chars)
    for(size_t i = 0; i < 8 && i < nameLen; i++) {
        fname[i] = toupper(filename[i]);
    }
    
    // Copy extension if present (up to 3 chars)
    if(dot && dot[1]) {
        for(size_t i = 0; i < 3 && dot[i + 1]; i++) {
            fext[i] = toupper(dot[i + 1]);
        }
    }
    
    // Find matching file
    for(uint16_t i = 0; i < BLOCK_SIZE; i += sizeof(DirEntry)) {
        DirEntry* current = (DirEntry*)(dirBuffer + i);
        
        // Skip empty or deleted entries
        if(current->name[0] == 0x00 || current->name[0] == 0xE5) {
            continue;
        }
        
        // Compare name and extension
        if(memcmp(current->name, fname, 8) == 0 && 
           memcmp(current->ext, fext, 3) == 0) {
            
            // Print file details
            Serial.print(F("File found: "));
            // Print name
            for(int j = 0; j < 8 && current->name[j] != ' '; j++) {
                Serial.print((char)current->name[j]);
            }
            Serial.print('.');
            // Print extension
            for(int j = 0; j < 3 && current->ext[j] != ' '; j++) {
                Serial.print((char)current->ext[j]);
            }
            Serial.print(F(" - Size: "));
            Serial.print(current->size);
            Serial.println(F(" bytes"));
            
            // Return the file size
            return current->size;
        }
    }
    
    Serial.println(F("File not found"));
    return 0xFFFFFFFF;  // Return max value to indicate error
}

bool isSDCardFull() {
    uint8_t buffer[BLOCK_SIZE];
    
    // Read the boot sector
    if(readBlock(partitionStart, buffer) != SUCCESS) {
        Serial.println(F("Failed to read boot sector"));
        return true; // Return true (full) on error to be safe
    }
    
    Fat32BootSector* boot = (Fat32BootSector*)buffer;
    
    // Read first sector of FAT
    uint32_t fatStartSector = partitionStart + boot->reserved_sectors;
    if(readBlock(fatStartSector, buffer) != SUCCESS) {
        Serial.println(F("Failed to read FAT"));
        return true;
    }
    
    // Quick scan for any free cluster
    for(uint32_t i = 0; i < BLOCK_SIZE; i += 4) {
        uint32_t fatEntry = *((uint32_t*)&buffer[i]) & 0x0FFFFFFF;
        if(fatEntry == 0) {
            // Found a free cluster
            return false;
        }
    }
    
    return true; // No free clusters found in first sector
}

// Example usage in setup():
void setup() {
    Serial.begin(9600);
    while(!Serial);
    
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    
    SPI.begin();
    SPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
    
    Serial.println(F("\nFAT32 SD Card Test"));
    Serial.println(F("================="));
    
    if(initSDCard() == SUCCESS && initializeFAT32()) {
        // Create a test file
        if(createFile("TEST.TXT")) {
            delay(1000);  // Small delay between operations
            
            // Append some data
            appendToFile("TEST.TXT", "Hello, World!\r\n");
            delay(1000);
            
            // Append more data
            appendToFile("TEST.TXT", "This is a test file.\r\n");

            // Get file size
            getFileSize("TEST.TXT");

            Serial.print(isSDCardFull());
        }
    }
}

void loop(){}
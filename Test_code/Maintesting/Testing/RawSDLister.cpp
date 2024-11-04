#include "RawSDLister.h"

RawSDLister::RawSDLister(uint8_t chipSelect) {
    _chipSelect = chipSelect;
    _initialized = false;
    _blocks = 0;
    _type = 0;
}

uint8_t RawSDLister::spiTransfer(uint8_t data) {
    return SPI.transfer(data);
}

void RawSDLister::select() {
    digitalWrite(_chipSelect, LOW);
}

void RawSDLister::deselect() {
    digitalWrite(_chipSelect, HIGH);
}

uint8_t RawSDLister::sendCommand(uint8_t cmd, uint32_t arg) {
    select();
    
    spiTransfer(cmd | 0x40);
    spiTransfer(arg >> 24);
    spiTransfer(arg >> 16);
    spiTransfer(arg >> 8);
    spiTransfer(arg);
    
    uint8_t crc = 0xFF;
    if (cmd == CMD0) crc = 0x95;  // Valid CRC for CMD0
    if (cmd == CMD8) crc = 0x87;  // Valid CRC for CMD8
    spiTransfer(crc);
    
    uint8_t response;
    for (uint8_t i = 0; i < 10; i++) {
        response = spiTransfer(0xFF);
        if (!(response & 0x80)) break;
    }
    
    return response;
}

bool RawSDLister::begin() {
    pinMode(_chipSelect, OUTPUT);
    deselect();
    
    SPI.begin();
    SPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
    
    // Send dummy bytes
    for (uint8_t i = 0; i < 10; i++) {
        spiTransfer(0xFF);
    }
    
    // Send CMD0 to enter SPI mode
    if (sendCommand(CMD0, 0) != 0x01) {
        Serial.println(F("Error: Could not enter SPI mode"));
        return false;
    }
    
    // Send CMD8 to check voltage range
    if (sendCommand(CMD8, 0x1AA) == 0x01) {
        // SDHC/SDXC card
        uint32_t response = 0;
        for (uint8_t i = 0; i < 4; i++) {
            response = (response << 8) | spiTransfer(0xFF);
        }
        if ((response & 0xFFF) != 0x1AA) {
            Serial.println(F("Error: Invalid voltage range"));
            return false;
        }
        
        // Initialize card
        uint16_t retries = 100;
        do {
            sendCommand(CMD55, 0);
            if (sendCommand(ACMD41, 0x40000000) == 0) break;
            retries--;
        } while (retries > 0);
        
        if (retries == 0) {
            Serial.println(F("Error: Card initialization failed"));
            return false;
        }
        
        _type = 2; // SDHC/SDXC
    }
    
    SPI.endTransaction();
    _initialized = true;
    return true;
}

bool RawSDLister::readBlock(uint32_t block, uint8_t* buffer) {
    if (!_initialized) return false;
    
    SPI.beginTransaction(SPISettings(25000000, MSBFIRST, SPI_MODE0));
    
    if (sendCommand(CMD17, block) != 0x00) {
        SPI.endTransaction();
        return false;
    }
    
    // Wait for data token
    uint16_t retries = 10000;
    uint8_t token;
    do {
        token = spiTransfer(0xFF);
        retries--;
    } while (token == 0xFF && retries > 0);
    
    if (token != 0xFE) {
        SPI.endTransaction();
        return false;
    }
    
    // Read block data
    for (uint16_t i = 0; i < BLOCK_SIZE; i++) {
        buffer[i] = spiTransfer(0xFF);
    }
    
    // Read CRC (discard)
    spiTransfer(0xFF);
    spiTransfer(0xFF);
    
    deselect();
    SPI.endTransaction();
    return true;
}

void RawSDLister::listRootDirectory() {
    if (!_initialized) return;
    
    uint8_t buffer[BLOCK_SIZE];
    
    // Read first sector of root directory (usually at block 2)
    if (!readBlock(2, buffer)) {
        Serial.println(F("Error reading root directory"));
        return;
    }
    
    Serial.println(F("\nFiles in root directory:"));
    Serial.println(F("======================="));
    
    // Parse directory entries
    for (uint16_t offset = 0; offset < BLOCK_SIZE; offset += 32) {
        // Check if entry is used
        if (buffer[offset] == 0x00) break;  // End of directory
        if (buffer[offset] == 0xE5) continue;  // Deleted entry
        
        // Get filename
        char filename[13];
        uint8_t j = 0;
        
        // Get name (first 8 bytes)
        for (uint8_t i = 0; i < 8; i++) {
            char c = buffer[offset + i];
            if (c == ' ') break;
            filename[j++] = c;
        }
        
        // Get extension (last 3 bytes)
        if (buffer[offset + 8] != ' ') {
            filename[j++] = '.';
            for (uint8_t i = 8; i < 11; i++) {
                char c = buffer[offset + i];
                if (c == ' ') break;
                filename[j++] = c;
            }
        }
        
        filename[j] = 0;  // Null terminate
        
        // Get file attributes
        uint8_t attr = buffer[offset + 11];
        
        // Get file size (4 bytes starting at offset + 28)
        uint32_t fileSize = 
            ((uint32_t)buffer[offset + 31] << 24) |
            ((uint32_t)buffer[offset + 30] << 16) |
            ((uint32_t)buffer[offset + 29] << 8) |
            buffer[offset + 28];
        
        // Print file information
        if (!(attr & 0x08)) {  // Skip volume label
            Serial.print(filename);
            if (attr & 0x10) {
                Serial.println("/");  // Directory
            } else {
                Serial.print("\t\t");
                Serial.print(fileSize);
                Serial.println(" bytes");
            }
        }
    }
    
    Serial.println(F("======================="));
}

void RawSDLister::printHex(uint8_t data) {
    if (data < 0x10) Serial.print('0');
    Serial.print(data, HEX);
    Serial.print(' ');
}

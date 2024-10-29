// SimplePFF.cpp
#include "SimplePFF.h"

uint8_t SimplePFF::spiTransfer(uint8_t data) {
    return SPI.transfer(data);
}

void SimplePFF::debugCommand(uint8_t cmd, uint8_t response) {
    Serial.print("CMD");
    Serial.print(cmd & 0x3F);
    Serial.print(" Response: 0x");
    Serial.println(response, HEX);
}



bool SimplePFF::writeBlock(uint32_t block, const uint8_t* buffer) {
    Serial.print("Writing block ");
    Serial.println(block);
    
    // Power stabilization delay
    delay(50);
    
    select();
    
    // Send write command at lower speed
    SPI.beginTransaction(SPISettings(250000, MSBFIRST, SPI_MODE0));
    
    if (!sendCommand(CMD24, block * BLOCK_SIZE)) {
        Serial.println("Write command failed");
        deselect();
        return false;
    }
    
    Serial.println("CMD24 accepted, stabilizing...");
    delay(10);  // Allow power to stabilize
    
    // Send dummy bytes before data token
    for (int i = 0; i < 8; i++) {
        spiTransfer(0xFF);
    }
    
    Serial.println("Sending data token 0xFE...");
    spiTransfer(0xFE);
    
    Serial.println("Writing data...");
    
    // Write data in smaller chunks with delays
    for (uint16_t i = 0; i < BLOCK_SIZE; i += 32) {
        // Write 32 bytes at a time
        for (uint16_t j = 0; j < 32 && (i + j) < BLOCK_SIZE; j++) {
            spiTransfer(buffer[i + j]);
        }
        
        // Add a tiny delay every 32 bytes
        delayMicroseconds(100);
        
        if (i % 128 == 0) {
            Serial.print("Written ");
            Serial.print(i);
            Serial.println(" bytes");
        }
    }
    
    Serial.println("Data written, sending CRC...");
    
    // Send CRC
    spiTransfer(0xFF);
    spiTransfer(0xFF);
    
    Serial.println("Waiting for data response...");
    
    // Read data response with timeout
    uint8_t response;
    uint16_t timeout = 0;
    do {
        response = spiTransfer(0xFF);
        timeout++;
        if (timeout >= 1000) {
            Serial.println("No data response received");
            deselect();
            return false;
        }
        delayMicroseconds(10);
    } while (response == 0xFF);
    
    Serial.print("Data response received: 0x");
    Serial.println(response, HEX);
    
    if ((response & 0x1F) != 0x05) {
        Serial.print("Bad data response: 0x");
        Serial.println(response, HEX);
        deselect();
        return false;
    }
    
    Serial.println("Data accepted, waiting for write to complete...");
    
    // Wait for write to complete with power-optimized approach
    timeout = 0;
    uint32_t startTime = millis();
    uint8_t lastResponse = 0xFF;
    bool writeComplete = false;
    
    while (millis() - startTime < 500) {  // 500ms timeout
        response = spiTransfer(0xFF);
        
        if (response != lastResponse) {
            Serial.print("Write status response: 0x");
            Serial.println(response, HEX);
            lastResponse = response;
        }
        
        if (response == 0xFF) {
            writeComplete = true;
            break;
        }
        
        // Add delay between checks
        delayMicroseconds(250);
        timeout++;
    }
    
    if (!writeComplete) {
        Serial.println("Write completion timeout");
        deselect();
        return false;
    }
    
    Serial.println("Write completed!");
    
    // Return to higher speed for next operations
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    
    // Send finalizing clock cycles
    for (int i = 0; i < 8; i++) {
        spiTransfer(0xFF);
    }
    
    deselect();
    delay(50);  // Final stabilization delay
    
    return true;
}

void SimplePFF::powerUpSequence() {
    // More conservative power-up sequence
    deselect();
    delay(100);
    
    // Send dummy clocks at very low speed
    SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE0));
    
    for (int i = 0; i < 100; i++) {
        spiTransfer(0xFF);
        delayMicroseconds(200);
    }
    
    delay(100);
}

bool SimplePFF::waitReady(uint16_t timeout) {
    uint32_t startTime = millis();
    uint8_t response;
    uint16_t count = 0;
    uint8_t lastResponse = 0xFF;
    
    while (millis() - startTime < timeout) {
        response = spiTransfer(0xFF);
        count++;
        
        if (response != lastResponse) {
            lastResponse = response;
        }
        
        if (response == 0xFF) {
            return true;
        }
        
        // More frequent but shorter delays
        if (count % 10 == 0) {
            delayMicroseconds(100);
        }
    }
    
    Serial.println("Wait timeout!");
    return false;
}

bool SimplePFF::sendCommand(uint8_t cmd, uint32_t arg) {
    // Send dummy bytes before command
    for (int i = 0; i < 8; i++) {
        spiTransfer(0xFF);
    }
    
    // Send command
    spiTransfer(0x40 | cmd);
    spiTransfer((arg >> 24) & 0xFF);
    spiTransfer((arg >> 16) & 0xFF);
    spiTransfer((arg >> 8) & 0xFF);
    spiTransfer(arg & 0xFF);
    
    uint8_t crc = 0xFF;    // Default CRC
    
    // Use proper CRC for specific commands
    if (cmd == CMD0) crc = 0x95;
    if (cmd == CMD8) crc = 0x87;
    
    spiTransfer(crc);
    
    // Skip stuff byte for stop read
    if (cmd == CMD12) {
        spiTransfer(0xFF);
    }
    
    // Wait for response with improved timing
    uint8_t response;
    uint16_t timeout = 0;
    
    do {
        response = spiTransfer(0xFF);
        timeout++;
        
        if (timeout % 100 == 0) {
            delayMicroseconds(100);
        }
        
        if (timeout > 1000) {
            Serial.println("Command timeout");
            return false;
        }
    } while ((response & 0x80) && timeout < 1000);
    
    debugCommand(cmd, response);
    return (response == 0x00);
}

bool SimplePFF::begin() {
    // Give power time to stabilize
    delay(250);
    
    Serial.println("Initializing SD card...");
    
    // Setup CS pin with pullup
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);  // Explicit HIGH
    deselect();
    
    // Also set other SPI pins with pullups
    pinMode(MISO, INPUT_PULLUP);
    pinMode(MOSI, OUTPUT);
    pinMode(SCK, OUTPUT);
    
    // Initialize SPI at very low speed first
    SPI.begin();
    SPI.beginTransaction(SPISettings(250000, MSBFIRST, SPI_MODE0));
    
    // More aggressive power stabilization sequence
    powerUpSequence();
    
    // Reset sequence
    deselect();
    for (int i = 0; i < 10; i++) {
        spiTransfer(0xFF);
    }
    delay(100);
    
    // Send GO_IDLE_STATE command (CMD0)
    uint8_t response = 0;
    uint8_t retry = 0;
    bool success = false;
    
    do {
        select();
        // Send CMD0 with correct CRC
        spiTransfer(0xFF);  // Extra clock cycle
        spiTransfer(0x40 | CMD0);
        spiTransfer(0x00);
        spiTransfer(0x00);
        spiTransfer(0x00);
        spiTransfer(0x00);
        spiTransfer(0x95);  // Valid CRC for CMD0
        
        // Wait for response with longer timeout
        for (int i = 0; i < 200; i++) {  // Increased timeout
            response = spiTransfer(0xFF);
            if (response != 0xFF) {
                success = (response == R1_IDLE_STATE);
                break;
            }
            delayMicroseconds(50);  // Shorter delay between tries
        }
        
        debugCommand(CMD0, response);
        
        if (!success) {
            deselect();
            delay(20);  // Shorter delay between retries
            retry++;
        }
        
        if (retry > 20) {  // Increased retry count
            Serial.println("Failed to enter idle state");
            deselect();
            return false;
        }
    } while (!success);
    
    Serial.println("Card entered idle state successfully");
    delay(50);  // Reduced wait after CMD0
    
    // CMD8 sequence with power management
    retry = 0;
    success = false;
    bool isSDHC = false;
    
    do {
        deselect();
        delayMicroseconds(100);
        select();
        
        spiTransfer(0xFF);
        spiTransfer(0x40 | CMD8);
        spiTransfer(0x00);
        spiTransfer(0x00);
        spiTransfer(0x01);
        spiTransfer(0xAA);
        spiTransfer(0x87);
        
        response = 0xFF;
        for (int i = 0; i < 100; i++) {
            response = spiTransfer(0xFF);
            if (response != 0xFF) break;
            delayMicroseconds(50);
        }
        
        debugCommand(CMD8, response);
        
        if (response == R1_IDLE_STATE) {
            uint8_t r7[4];
            for(int i = 0; i < 4; i++) {
                r7[i] = spiTransfer(0xFF);
            }
            
            Serial.print("R7 response: ");
            for(int i = 0; i < 4; i++) {
                Serial.print("0x");
                Serial.print(r7[i], HEX);
                Serial.print(" ");
            }
            Serial.println();
            
            if (r7[2] == 0x01 && r7[3] == 0xAA) {
                Serial.println("SDHC/SDXC card detected");
                isSDHC = true;
                success = true;
            }
        } else if (response & R1_ILLEGAL_COMMAND) {
            Serial.println("Legacy SD card detected");
            success = true;
            isSDHC = false;
        }
        
        retry++;
        if (retry > 10 && !success) {
            Serial.println("CMD8 failed");
            deselect();
            return false;
        }
        
        delay(10);  // Short delay between retries
    } while (!success);
    
    // ACMD41 initialization with power management
    retry = 0;
    success = false;
    
    do {
        deselect();
        delayMicroseconds(100);
        select();
        
        // CMD55
        spiTransfer(0xFF);
        spiTransfer(0x40 | CMD55);
        spiTransfer(0x00);
        spiTransfer(0x00);
        spiTransfer(0x00);
        spiTransfer(0x00);
        spiTransfer(0x65);
        
        response = 0xFF;
        for (int i = 0; i < 100; i++) {
            response = spiTransfer(0xFF);
            if (response != 0xFF) break;
            delayMicroseconds(50);
        }
        
        debugCommand(CMD55, response);
        
        if (response == R1_IDLE_STATE) {
            spiTransfer(0xFF);
            
            // ACMD41
            spiTransfer(0x40 | ACMD41);
            spiTransfer(isSDHC ? 0x40 : 0x00);
            spiTransfer(0x00);
            spiTransfer(0x00);
            spiTransfer(0x00);
            spiTransfer(0x77);
            
            response = 0xFF;
            for (int i = 0; i < 100; i++) {
                response = spiTransfer(0xFF);
                if (response != 0xFF) break;
                delayMicroseconds(50);
            }
            
            debugCommand(ACMD41, response);
            success = (response == 0x00);
        }
        
        retry++;
        if (retry > 100) {  // Reduced maximum retries
            Serial.println("Card initialization timeout");
            deselect();
            return false;
        }
        
        if (!success) {
            delay(20);  // Shorter delay between retries
        }
    } while (!success);
    
    // Check card status
    deselect();
    delay(20);
    select();
    
    spiTransfer(0xFF);
    spiTransfer(0x40 | CMD58);
    spiTransfer(0x00);
    spiTransfer(0x00);
    spiTransfer(0x00);
    spiTransfer(0x00);
    spiTransfer(0xFF);
    
    response = 0xFF;
    for (int i = 0; i < 100; i++) {
        response = spiTransfer(0xFF);
        if (response != 0xFF) break;
        delayMicroseconds(50);
    }
    
    debugCommand(CMD58, response);
    
    if (response == 0x00) {
        uint8_t ocr[4];
        for (int i = 0; i < 4; i++) {
            ocr[i] = spiTransfer(0xFF);
        }
        
        Serial.print("OCR: ");
        for(int i = 0; i < 4; i++) {
            Serial.print("0x");
            Serial.print(ocr[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        
        if (ocr[0] & 0x80) {
            Serial.println("Card power up completed");
            if (ocr[0] & 0x40) {
                Serial.println("Card is high capacity");
            }
        } else {
            Serial.println("Card power up not complete");
            deselect();
            return false;
        }
    }
    
    // Keep SPI speed low for reliability
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    
    deselect();
    delay(50);
    select();
    
    // Set block size
    Serial.println("Sending CMD16...");
    spiTransfer(0x40 | CMD16);
    spiTransfer(0x00);
    spiTransfer(0x00);
    spiTransfer(0x02);
    spiTransfer(0x00);
    spiTransfer(0x15);
    
    response = 0xFF;
    for (int i = 0; i < 100; i++) {
        response = spiTransfer(0xFF);
        if (response != 0xFF) break;
        delayMicroseconds(50);
    }
    
    debugCommand(CMD16, response);
    
    if (response == 0x00) {
        Serial.println("Block size set successfully");
    } else if (isSDHC) {
        Serial.println("Block size ignored (SDHC card)");
    } else {
        Serial.println("Failed to set block size");
        deselect();
        return false;
    }
    
    deselect();
    
    // Send dummy clocks to ensure card is ready
    for (int i = 0; i < 10; i++) {
        spiTransfer(0xFF);
    }
    
    Serial.println("Card initialized successfully");
    mounted = true;
    return true;
}



bool SimplePFF::readBlock(uint32_t block, uint8_t* buffer) {
    Serial.print("Reading block ");
    Serial.println(block);
    
    select();
    
    // Send read command
    if (!sendCommand(CMD17, block * BLOCK_SIZE)) {
        Serial.println("Read command failed");
        deselect();
        return false;
    }
    
    // Wait for data token
    uint8_t token;
    uint16_t timer = 0;
    do {
        token = spiTransfer(0xFF);
        timer++;
        if (timer > 5000) {
            Serial.println("Read timeout");
            deselect();
            return false;
        }
    } while (token == 0xFF);
    
    if (token != 0xFE) {
        Serial.print("Bad data token: 0x");
        Serial.println(token, HEX);
        deselect();
        return false;
    }
    
    // Read data
    for (uint16_t i = 0; i < BLOCK_SIZE; i++) {
        buffer[i] = spiTransfer(0xFF);
    }
    
    // Read CRC (ignored)
    spiTransfer(0xFF);
    spiTransfer(0xFF);
    
    deselect();
    return true;
}


bool SimplePFF::findFile(const char* filename, uint16_t* position) {
    uint8_t buffer[BLOCK_SIZE];
    if (!readBlock(ROOT_DIR_BLOCK, buffer)) return false;
    
    FileEntry* entries = (FileEntry*)buffer;
    for (uint16_t i = 0; i < BLOCK_SIZE / sizeof(FileEntry); i++) {
        if (strncmp(entries[i].filename, filename, MAX_FILENAME_LENGTH) == 0) {
            if (position) *position = i;
            return true;
        }
    }
    return false;
}

bool SimplePFF::createFile(const char* filename) {
    if (!mounted) return false;
    
    Serial.print("Creating file: ");
    Serial.println(filename);
    
    if (findFile(filename)) {
        Serial.println("File already exists");
        return false;
    }
    
    uint8_t buffer[BLOCK_SIZE];
    if (!readBlock(ROOT_DIR_BLOCK, buffer)) return false;
    
    FileEntry* entries = (FileEntry*)buffer;
    bool found = false;
    
    // Find empty slot
    for (uint16_t i = 0; i < BLOCK_SIZE / sizeof(FileEntry); i++) {
        if (entries[i].filename[0] == 0) {
            strncpy(entries[i].filename, filename, MAX_FILENAME_LENGTH);
            entries[i].startBlock = currentBlock++;
            entries[i].size = 0;
            found = true;
            break;
        }
    }
    
    if (!found) {
        Serial.println("No free directory entries");
        return false;
    }
    
    if (!writeBlock(ROOT_DIR_BLOCK, buffer)) {
        Serial.println("Failed to update directory");
        return false;
    }
    
    return true;
}

bool SimplePFF::writeToFile(const char* filename, const char* text) {
    if (!mounted) return false;
    
    Serial.print("Writing to file: ");
    Serial.println(filename);
    
    uint8_t buffer[BLOCK_SIZE];
    if (!readBlock(ROOT_DIR_BLOCK, buffer)) return false;
    
    FileEntry* entries = (FileEntry*)buffer;
    FileEntry* targetFile = nullptr;
    
    for (uint16_t i = 0; i < BLOCK_SIZE / sizeof(FileEntry); i++) {
        if (strncmp(entries[i].filename, filename, MAX_FILENAME_LENGTH) == 0) {
            targetFile = &entries[i];
            break;
        }
    }
    
    if (!targetFile) {
        Serial.println("File not found");
        return false;
    }
    
    // Prepare data block
    memset(buffer, 0, BLOCK_SIZE);
    uint32_t len = strlen(text);
    if (len > BLOCK_SIZE) len = BLOCK_SIZE;
    memcpy(buffer, text, len);
    
    if (!writeBlock(targetFile->startBlock, buffer)) {
        Serial.println("Failed to write data block");
        return false;
    }
    
    targetFile->size = len;
    
    if (!writeBlock(ROOT_DIR_BLOCK, (uint8_t*)entries)) {
        Serial.println("Failed to update directory");
        return false;
    }
    
    return true;
}

bool SimplePFF::rename(const char* oldname, const char* newname) {
    if (!mounted) return false;
    
    Serial.print("Renaming ");
    Serial.print(oldname);
    Serial.print(" to ");
    Serial.println(newname);
    
    uint8_t buffer[BLOCK_SIZE];
    if (!readBlock(ROOT_DIR_BLOCK, buffer)) return false;
    
    FileEntry* entries = (FileEntry*)buffer;
    bool found = false;
    
    for (uint16_t i = 0; i < BLOCK_SIZE / sizeof(FileEntry); i++) {
        if (strncmp(entries[i].filename, oldname, MAX_FILENAME_LENGTH) == 0) {
            strncpy(entries[i].filename, newname, MAX_FILENAME_LENGTH);
            found = true;
            break;
        }
    }
    
    if (!found) {
        Serial.println("Original file not found");
        return false;
    }
    
    if (!writeBlock(ROOT_DIR_BLOCK, buffer)) {
        Serial.println("Failed to update directory");
        return false;
    }
    
    return true;
}

uint32_t SimplePFF::getFileSize(const char* filename) {
    if (!mounted) return 0;
    
    uint8_t buffer[BLOCK_SIZE];
    if (!readBlock(ROOT_DIR_BLOCK, buffer)) return 0;
    
    FileEntry* entries = (FileEntry*)buffer;
    
    for (uint16_t i = 0; i < BLOCK_SIZE / sizeof(FileEntry); i++) {
        if (strncmp(entries[i].filename, filename, MAX_FILENAME_LENGTH) == 0) {
            return entries[i].size;
        }
    }
    
    return 0;
}

bool SimplePFF::end() {
    if (!mounted) return false;
    deselect();
    SPI.end();
    mounted = false;
    return true;
}

SimplePFF SPF;

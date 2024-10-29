
#include "Arduino.h"
#include "SPI.h"

extern "C" void __attribute__((weak)) yield(void) {}
// SD card initialization and file operations
// Uses SPI communication protocol

// Pin definitions
const int SD_CS_PIN = 4;  // Chip Select pin

// Status codes
#define SD_INIT_SUCCESS 0
#define SD_INIT_ERROR 1
#define FILE_CREATE_SUCCESS 0
#define FILE_CREATE_ERROR 1

// SD card commands
#define CMD0    0x40  // GO_IDLE_STATE
#define CMD1    0x41  // SEND_OP_COND
#define CMD8    0x48  // SEND_IF_COND
#define CMD55   0x77  // APP_CMD
#define ACMD41  0x69  // SD_SEND_OP_COND

// Send command to SD card
uint8_t sendCommand(uint8_t cmd, uint32_t arg) {
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

// SD card initialization function
uint8_t initSDCard() {
  Serial.println("Beginning SD card initialization...");
  
  // Configure SPI pins
  pinMode(SD_CS_PIN, OUTPUT);
  pinMode(11, OUTPUT);  // MOSI
  pinMode(12, INPUT);   // MISO
  pinMode(13, OUTPUT);  // SCK
  
  // Deselect SD card and all other SPI devices
  digitalWrite(SD_CS_PIN, HIGH);
  
  // Start SPI
  SPI.begin();
  SPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
  
  Serial.println("Sending initial clock pulses...");
  // Send 80 clock pulses to initialize
  for(int i = 0; i < 10; i++) {
    SPI.transfer(0xFF);
  }
  
  // Send CMD0 to enter SPI mode
  Serial.println("Sending CMD0...");
  uint8_t response = 0;
  int attempts = 0;
  do {
    response = sendCommand(CMD0, 0);
    attempts++;
    if(attempts > 10) {
      Serial.println("CMD0 failed after 10 attempts");
      return SD_INIT_ERROR;
    }
  } while(response != 0x01);
  Serial.println("CMD0 successful!");
  
  // Send CMD8 to check voltage range (required for SDHC cards)
  Serial.println("Sending CMD8...");
  response = sendCommand(CMD8, 0x1AA);
  if(response == 0x01) {  // SD v2.x
    // Read remaining 4 bytes of R7 response
    uint32_t r7 = SPI.transfer(0xFF) << 24;
    r7 |= SPI.transfer(0xFF) << 16;
    r7 |= SPI.transfer(0xFF) << 8;
    r7 |= SPI.transfer(0xFF);
    
    if((r7 & 0xFFF) != 0x1AA) {
      Serial.println("CMD8 voltage pattern mismatch");
      return SD_INIT_ERROR;
    }
    Serial.println("CMD8 successful!");
    
    // Initialize card with ACMD41
    Serial.println("Sending ACMD41...");
    attempts = 0;
    do {
      sendCommand(CMD55, 0);  // APP_CMD
      response = sendCommand(ACMD41, 0x40000000);  // HCS bit set
      attempts++;
      if(attempts > 100) {
        Serial.println("ACMD41 failed after 100 attempts");
        return SD_INIT_ERROR;
      }
    } while(response != 0x00);
    Serial.println("ACMD41 successful!");
  }
  else {  // SD v1.x or MMC
    Serial.println("Card is SD v1.x or MMC");
    // Initialize with CMD1
    Serial.println("Sending CMD1...");
    attempts = 0;
    do {
      response = sendCommand(CMD1, 0);
      attempts++;
      if(attempts > 100) {
        Serial.println("CMD1 failed after 100 attempts");
        return SD_INIT_ERROR;
      }
    } while(response != 0x00);
    Serial.println("CMD1 successful!");
  }
  
  // Set SPI to full speed
  SPI.endTransaction();
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
  
  digitalWrite(SD_CS_PIN, HIGH);  // Deselect card
  Serial.println("SD card initialization complete!");
  return SD_INIT_SUCCESS;
}

// Create file function remains the same
uint8_t createFile(const char* filename) {
    // ... (previous createFile implementation)
}

void setup() {
  Serial.begin(9600);
  while(!Serial) {
    ; // Wait for serial port to connect
  }
  
  Serial.println("\nSD Card Test Program");
  Serial.println("-------------------");
  
  if(initSDCard() == SD_INIT_SUCCESS) {
    Serial.println("SD card initialized successfully!");
    
    Serial.println("Creating test file...");
    if(createFile("TEST.TXT") == FILE_CREATE_SUCCESS) {
      Serial.println("File created successfully!");
    } else {
      Serial.println("Error creating file!");
    }
  } else {
    Serial.println("SD card initialization failed!");
  }
}

void loop() {
  // Main program loop
  delay(1000);
}

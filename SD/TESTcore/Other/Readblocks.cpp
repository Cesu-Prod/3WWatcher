#include "SDCore.h"
#include "Arduino.h"

extern "C" void __attribute__((weak)) yield(void) {}

// Function to check if a block is empty (all zeros)
bool isEmptyBlock(byte *buffer) {
  for (int i = 0; i < 512; i++) {
    if (buffer[i] != 0) {
      return false;
    }
  }
  return true;
}

// Function to check if block contains printable content
bool hasTextContent(byte *buffer) {
  int printableCount = 0;
  for (int i = 0; i < 512; i++) {
    if ((buffer[i] >= 32 && buffer[i] <= 126) || buffer[i] == '\n' || buffer[i] == '\r' || buffer[i] == '\t') {
      printableCount++;
    }
  }
  // Consider block as text if more than 30% is printable
  return (printableCount > 150);
}

void printBlockContents(byte *buffer, unsigned long blockNumber) {
  // Print block header
  Serial.print("\n=== Block ");
  Serial.print(blockNumber);
  Serial.println(" ===");

  // If block has mostly text content, print as text
  if (hasTextContent(buffer)) {
    for (int i = 0; i < 512; i++) {
      if (buffer[i] >= 32 && buffer[i] <= 126) {
        Serial.write(buffer[i]);
      } else if (buffer[i] == '\n' || buffer[i] == '\r' || buffer[i] == '\t') {
        Serial.write(buffer[i]);
      } else {
        Serial.write('.');
      }
    }
  } else {
    // Print as hex for non-text content
    for (int i = 0; i < 512; i++) {
      if (i % 16 == 0) {
        Serial.println();
        if (buffer[i] < 0x10) Serial.print("0");
        Serial.print(i, HEX);
        Serial.print(": ");
      }
      if (buffer[i] < 0x10) Serial.print("0");
      Serial.print(buffer[i], HEX);
      Serial.print(" ");
    }
  }
  Serial.println("\n");
}

void setup() {
  // Initialize Serial communication
  Serial.begin(250000);
  while (!Serial) {
    ; // Wait for Serial port to connect
  }
  
  Serial.println("Initializing SD card...");
  
  // Initialize SD Card
  if (!SDCore::begin()) {
    Serial.println("SD card initialization failed!");
    return;
  }
  
  Serial.println("SD card initialized successfully.");
  Serial.println("Reading entire card contents...\n");
  
  // Create buffer to store blocks
  byte buffer[512];
  unsigned long blockCount = 0;
  unsigned long emptyBlocksInRow = 0;
  bool skippingEmptyBlocks = false;
  
  // Read blocks until we can't anymore or find too many empty blocks in a row
  while (true) {
    if (SDCore::read(blockCount, buffer)) {
      if (isEmptyBlock(buffer)) {
        emptyBlocksInRow++;
        if (emptyBlocksInRow > 100) {  // After 100 empty blocks, we assume we've reached unused space
          if (!skippingEmptyBlocks) {
            Serial.println("Found large empty section, skipping...");
            skippingEmptyBlocks = true;
          }
        }
      } else {
        emptyBlocksInRow = 0;
        skippingEmptyBlocks = false;
        printBlockContents(buffer, blockCount);
      }
      
      // Print progress every 100 blocks
      if (blockCount % 100 == 0) {
        Serial.print("\nProcessed ");
        Serial.print(blockCount);
        Serial.println(" blocks...");
      }
      
      blockCount++;
      
      // Break if we've found too many empty blocks or reached a very high block number
      if (emptyBlocksInRow > 1000 || blockCount > 1000000) {  // Adjust these limits as needed
        break;
      }
    } else {
      Serial.print("\nCould not read beyond block ");
      Serial.println(blockCount);
      break;
    }
    
    // Small delay to prevent watchdog issues
    delay(1);
    yield();  // Allow other processes to run
  }
  
  // Close connection with SD
  SDCore::end();
  Serial.print("\nCompleted reading ");
  Serial.print(blockCount);
  Serial.println(" blocks.");
  Serial.println("SD card reading complete.");
}

void loop() {
  // Empty loop
}
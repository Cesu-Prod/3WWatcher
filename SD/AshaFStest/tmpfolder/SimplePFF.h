#ifndef SIMPLE_PFF_H
#define SIMPLE_PFF_H

#include "Arduino.h"
#include "SPI.h"

// Pin definitions
#define SD_CS_PIN 4

// Block and file system constants
#define BLOCK_SIZE 512
#define ROOT_DIR_BLOCK 2
#define MAX_FILENAME_LENGTH 8

// SD Card Commands
#define CMD0    0   // GO_IDLE_STATE - Reset card to idle state
#define CMD1    1   // SEND_OP_COND - Initialize card
#define CMD8    8   // SEND_IF_COND - Verify interface operating condition
#define CMD9    9   // SEND_CSD - Read CSD register
#define CMD10   10  // SEND_CID - Read CID register
#define CMD12   12  // STOP_TRANSMISSION - Stop reading data
#define CMD13   13  // SEND_STATUS - Read card status
#define CMD16   16  // SET_BLOCKLEN - Set the block length
#define CMD17   17  // READ_SINGLE_BLOCK - Read a single block
#define CMD18   18  // READ_MULTIPLE_BLOCK - Read multiple blocks
#define CMD24   24  // WRITE_BLOCK - Write a single block
#define CMD25   25  // WRITE_MULTIPLE_BLOCK - Write multiple blocks
#define CMD27   27  // PROGRAM_CSD - Program CSD register
#define CMD28   28  // SET_WRITE_PROT - Set write protection
#define CMD29   29  // CLR_WRITE_PROT - Clear write protection
#define CMD30   30  // SEND_WRITE_PROT - Read write protection
#define CMD32   32  // ERASE_WR_BLK_START - Set first erase block
#define CMD33   33  // ERASE_WR_BLK_END - Set last erase block
#define CMD38   38  // ERASE - Erase selected blocks
#define CMD55   55  // APP_CMD - Next command is an application command
#define CMD58   58  // READ_OCR - Read OCR register
#define CMD59   59  // CRC_ON_OFF - Turn CRC on/off
#define ACMD41  41  // APP_SEND_OP_COND - Send operating condition

// Response codes
#define R1_IDLE_STATE           0x01
#define R1_ERASE_RESET         0x02
#define R1_ILLEGAL_COMMAND     0x04
#define R1_COM_CRC_ERROR       0x08
#define R1_ERASE_SEQUENCE_ERROR 0x10
#define R1_ADDRESS_ERROR       0x20
#define R1_PARAMETER_ERROR     0x40

// File entry structure
struct FileEntry {
    char filename[MAX_FILENAME_LENGTH];
    uint32_t startBlock;
    uint32_t size;
};

class SimplePFF {
public:
    SimplePFF() : mounted(false), currentBlock(3) {}
    
    bool begin();
    uint32_t getFileSize(const char* filename);
    bool rename(const char* oldname, const char* newname);
    bool createFile(const char* filename);
    bool writeToFile(const char* filename, const char* text);
    bool end();
    
    // Made public for testing
    bool readBlock(uint32_t block, uint8_t* buffer);
    bool writeBlock(uint32_t block, const uint8_t* buffer);
    
private:
    bool mounted;
    uint32_t currentBlock;
    
    inline void select() { digitalWrite(SD_CS_PIN, LOW); delay(1); }
    inline void deselect() { digitalWrite(SD_CS_PIN, HIGH); delay(1); }
    uint8_t spiTransfer(uint8_t data);
    bool sendCommand(uint8_t cmd, uint32_t arg);
    void debugCommand(uint8_t cmd, uint8_t response);
    bool waitReady(uint16_t timeout = 500);
    void powerUpSequence();
    bool initCard();
    bool findFile(const char* filename, uint16_t* position = nullptr);
};

extern SimplePFF SPF;

#endif

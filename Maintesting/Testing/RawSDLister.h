#ifndef RawSDLister_h
#define RawSDLister_h

#include "Arduino.h"
#include "SPI.h"

// SD Card commands
#define CMD0    0x40    // GO_IDLE_STATE
#define CMD8    0x48    // SEND_IF_COND
#define CMD9    0x49    // SEND_CSD
#define CMD10   0x4A    // SEND_CID
#define CMD12   0x4C    // STOP_TRANSMISSION
#define CMD16   0x50    // SET_BLOCKLEN
#define CMD17   0x51    // READ_SINGLE_BLOCK
#define CMD18   0x52    // READ_MULTIPLE_BLOCK
#define CMD24   0x58    // WRITE_BLOCK
#define CMD25   0x59    // WRITE_MULTIPLE_BLOCK
#define CMD55   0x77    // APP_CMD
#define CMD58   0x7A    // READ_OCR
#define ACMD41  0x69    // SD_SEND_OP_COND

// File system constants
#define BLOCK_SIZE 512
#define FAT_ENTRY_SIZE 32

class RawSDLister {
private:
    uint8_t _chipSelect;
    bool _initialized;
    uint32_t _blocks;
    uint8_t _type;
    
    uint8_t spiTransfer(uint8_t data);
    void select();
    void deselect();
    uint8_t sendCommand(uint8_t cmd, uint32_t arg);
    bool readBlock(uint32_t block, uint8_t* buffer);
    uint32_t readCluster(uint32_t cluster);
    bool readFATEntry(uint32_t cluster, uint32_t &nextCluster);
    void printHex(uint8_t data);

public:
    RawSDLister(uint8_t chipSelect = 10);
    bool begin();
    void listRootDirectory();
    bool isInitialized() { return _initialized; }
};

#endif

#ifndef RTC_H
#define RTC_H

#include <stdint.h>  // Inclusion pour uint8_t et uint16_t

#define DS1307_ADDR 0x68

class DS1307
{
public:
    bool begin();
    bool isRunning();
    void startClock();
    void stopClock();

    void setTime(uint8_t hours, uint8_t minutes, uint8_t seconds);
    void getTime(uint8_t &hours, uint8_t &minutes, uint8_t &seconds);

    void setDate(uint8_t day, uint8_t month, uint8_t year);
    void getDate(uint8_t &day, uint8_t &month, uint8_t &year);

private:
    void i2cStart();
    void i2cStop();
    void i2cWrite(uint8_t data);
    uint8_t i2cRead(bool ack);
    uint8_t bcd2bin(uint8_t val);
    uint8_t bin2bcd(uint8_t val);
};

#endif /* RTC_H */
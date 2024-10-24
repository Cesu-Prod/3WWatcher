#ifndef RTC_H
#define RTC_H

#define DS1307_ADDR 0x68

class DS1307
{
public:
    bool begin();
    void setSeconds(uint8_t seconds);
    uint8_t getSeconds();
    void setMinutes(uint8_t minutes);
    uint8_t getMinutes();
    void setHours(uint8_t hours);
    uint8_t getHours();
    void setDay(uint8_t day);
    uint8_t getDay();
    void setMonth(uint8_t month);
    uint8_t getMonth();
    void setYear(uint16_t year);
    uint16_t getYear();
    void setDateTime(uint8_t hour, uint8_t minute, uint8_t second, uint8_t day, uint8_t month, uint16_t year);
    
private:
    void i2c_start();
    void i2c_stop();
    void i2c_write_bit(bool bit);
    bool i2c_read_bit();
    void i2c_write_byte(uint8_t byte);
    uint8_t i2c_read_byte(bool ack);
    void rtc_write(uint8_t addr, uint8_t data);
    uint8_t rtc_read(uint8_t addr);
    uint8_t bcd2bin(uint8_t val);
    uint8_t bin2bcd(uint8_t val);
};

#endif /* RTC_H */

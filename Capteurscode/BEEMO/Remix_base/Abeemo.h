#ifndef Abeemo_H
#define Abeemo_H

#include <stdint.h>

class BME280_Simple {
public:
    BME280_Simple(uint8_t sda_pin, uint8_t scl_pin);
    
    // Core functions
    bool begin();
    void sleep();
    void wake();
    float readTemperature();
    float readPressure();
    uint8_t readHumidity();

private:
    uint8_t _sda_pin;
    uint8_t _scl_pin;
    uint8_t _address = 0x76; // Default BME280 address
    
    // Calibration data structure
    struct {
        uint16_t T1;
        int16_t T2, T3;
        uint16_t P1;
        int16_t P2, P3, P4, P5, P6, P7, P8, P9;
        uint8_t H1, H3;
        int16_t H2, H4, H5;
        int8_t H6;
    } _calib;

    // I2C communication functions
    void i2c_start();
    void i2c_stop();
    void i2c_write(uint8_t data);
    uint8_t i2c_read(bool ack);
    bool i2c_writeReg(uint8_t reg, uint8_t data);
    uint8_t i2c_readReg(uint8_t reg);
    void i2c_readRegs(uint8_t reg, uint8_t* buffer, uint8_t len);

    // Helper functions
    int32_t compensateTemp(int32_t adc_T);
    float compensatePressure(int32_t adc_P, int32_t t_fine);
    uint8_t compensateHumidity(int32_t adc_H, int32_t t_fine);
};

#endif
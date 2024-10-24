#include "Abeemo.h"
#include "Arduino.h"

BME280_Simple::BME280_Simple(uint8_t sda_pin, uint8_t scl_pin) : _sda_pin(sda_pin), _scl_pin(scl_pin) {}

bool BME280_Simple::begin() {
    // Configure pins
    pinMode(_sda_pin, INPUT_PULLUP);
    pinMode(_scl_pin, INPUT_PULLUP);
    
    // Check device ID
    uint8_t chip_id = i2c_readReg(0xD0);
    if (chip_id != 0x60) return false;

    // Read calibration data
    uint8_t cal_data[26];
    i2c_readRegs(0x88, cal_data, 24);  // Temperature and Pressure calibration
    i2c_readRegs(0xA1, &cal_data[24], 1);  // Humidity calibration part 1
    i2c_readRegs(0xE1, &cal_data[25], 1);  // Humidity calibration part 2
    
    // Parse calibration data
    _calib.T1 = (cal_data[1] << 8) | cal_data[0];
    _calib.T2 = (cal_data[3] << 8) | cal_data[2];
    _calib.T3 = (cal_data[5] << 8) | cal_data[4];
    
    _calib.P1 = (cal_data[7] << 8) | cal_data[6];
    _calib.P2 = (cal_data[9] << 8) | cal_data[8];
    _calib.P3 = (cal_data[11] << 8) | cal_data[10];
    _calib.P4 = (cal_data[13] << 8) | cal_data[12];
    _calib.P5 = (cal_data[15] << 8) | cal_data[14];
    _calib.P6 = (cal_data[17] << 8) | cal_data[16];
    _calib.P7 = (cal_data[19] << 8) | cal_data[18];
    _calib.P8 = (cal_data[21] << 8) | cal_data[20];
    _calib.P9 = (cal_data[23] << 8) | cal_data[22];
    
    _calib.H1 = cal_data[24];
    _calib.H2 = (cal_data[25] << 8);
    
    // Set default configuration (normal mode, x1 oversampling)
    i2c_writeReg(0xF2, 0x01); // ctrl_hum
    i2c_writeReg(0xF4, 0x27); // ctrl_meas
    i2c_writeReg(0xF5, 0x00); // config
    
    return true;
}

void BME280_Simple::sleep() {
    uint8_t ctrl = i2c_readReg(0xF4);
    i2c_writeReg(0xF4, ctrl & 0xFC); // Clear mode bits to enter sleep mode
}

void BME280_Simple::wake() {
    uint8_t ctrl = i2c_readReg(0xF4);
    i2c_writeReg(0xF4, (ctrl & 0xFC) | 0x03); // Set mode to normal
}

float BME280_Simple::readTemperature() {
    uint8_t data[3];
    i2c_readRegs(0xFA, data, 3);
    int32_t adc_T = ((data[0] << 16) | (data[1] << 8) | data[2]) >> 4;
    int32_t t_fine = compensateTemp(adc_T);
    return (float)((t_fine * 5 + 128) >> 8) / 100.0f;
}

float BME280_Simple::readPressure() {
    uint8_t data[3];
    i2c_readRegs(0xF7, data, 3);
    int32_t adc_P = ((data[0] << 16) | (data[1] << 8) | data[2]) >> 4;
    // First read temperature to get t_fine
    uint8_t temp_data[3];
    i2c_readRegs(0xFA, temp_data, 3);
    int32_t adc_T = ((temp_data[0] << 16) | (temp_data[1] << 8) | temp_data[2]) >> 4;
    int32_t t_fine = compensateTemp(adc_T);
    return compensatePressure(adc_P, t_fine);
}

uint8_t BME280_Simple::readHumidity() {
    uint8_t data[2];
    i2c_readRegs(0xFD, data, 2);
    int32_t adc_H = (data[0] << 8) | data[1];
    // First read temperature to get t_fine
    uint8_t temp_data[3];
    i2c_readRegs(0xFA, temp_data, 3);
    int32_t adc_T = ((temp_data[0] << 16) | (temp_data[1] << 8) | temp_data[2]) >> 4;
    int32_t t_fine = compensateTemp(adc_T);
    return compensateHumidity(adc_H, t_fine);
}

// Private I2C implementation
void BME280_Simple::i2c_start() {
    digitalWrite(_sda_pin, LOW);
    pinMode(_sda_pin, OUTPUT);
    delayMicroseconds(4);
    digitalWrite(_scl_pin, LOW);
    pinMode(_scl_pin, OUTPUT);
}

void BME280_Simple::i2c_stop() {
    pinMode(_sda_pin, OUTPUT);
    digitalWrite(_sda_pin, LOW);
    pinMode(_scl_pin, INPUT_PULLUP);
    delayMicroseconds(4);
    pinMode(_sda_pin, INPUT_PULLUP);
}

void BME280_Simple::i2c_write(uint8_t data) {
    for (int i = 7; i >= 0; i--) {
        digitalWrite(_sda_pin, (data >> i) & 0x01);
        delayMicroseconds(4);
        digitalWrite(_scl_pin, HIGH);
        delayMicroseconds(4);
        digitalWrite(_scl_pin, LOW);
    }
    pinMode(_sda_pin, INPUT_PULLUP);
    digitalWrite(_scl_pin, HIGH);
    delayMicroseconds(4);
    uint8_t ack = digitalRead(_sda_pin);
    digitalWrite(_scl_pin, LOW);
    pinMode(_sda_pin, OUTPUT);
}

uint8_t BME280_Simple::i2c_read(bool ack) {
    uint8_t data = 0;
    pinMode(_sda_pin, INPUT_PULLUP);
    for (int i = 7; i >= 0; i--) {
        digitalWrite(_scl_pin, HIGH);
        delayMicroseconds(4);
        data = (data << 1) | digitalRead(_sda_pin);
        digitalWrite(_scl_pin, LOW);
        delayMicroseconds(4);
    }
    pinMode(_sda_pin, OUTPUT);
    digitalWrite(_sda_pin, !ack);
    digitalWrite(_scl_pin, HIGH);
    delayMicroseconds(4);
    digitalWrite(_scl_pin, LOW);
    return data;
}

bool BME280_Simple::i2c_writeReg(uint8_t reg, uint8_t data) {
    i2c_start();
    i2c_write(_address << 1);
    i2c_write(reg);
    i2c_write(data);
    i2c_stop();
    return true;
}

uint8_t BME280_Simple::i2c_readReg(uint8_t reg) {
    i2c_start();
    i2c_write(_address << 1);
    i2c_write(reg);
    i2c_start();
    i2c_write((_address << 1) | 1);
    uint8_t data = i2c_read(false);
    i2c_stop();
    return data;
}

void BME280_Simple::i2c_readRegs(uint8_t reg, uint8_t* buffer, uint8_t len) {
    i2c_start();
    i2c_write(_address << 1);
    i2c_write(reg);
    i2c_start();
    i2c_write((_address << 1) | 1);
    for (uint8_t i = 0; i < len - 1; i++) {
        buffer[i] = i2c_read(true);
    }
    buffer[len - 1] = i2c_read(false);
    i2c_stop();
}

// Compensation formulas from BME280 datasheet
int32_t BME280_Simple::compensateTemp(int32_t adc_T) {
    int32_t var1, var2;
    var1 = ((((adc_T >> 3) - ((int32_t)_calib.T1 << 1))) * ((int32_t)_calib.T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)_calib.T1)) * ((adc_T >> 4) - ((int32_t)_calib.T1))) >> 12) * ((int32_t)_calib.T3)) >> 14;
    return var1 + var2;
}

float BME280_Simple::compensatePressure(int32_t adc_P, int32_t t_fine) {
    int64_t var1, var2, p;
    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)_calib.P6;
    var2 = var2 + ((var1 * (int64_t)_calib.P5) << 17);
    var2 = var2 + (((int64_t)_calib.P4) << 35);
    var1 = ((var1 * var1 * (int64_t)_calib.P3) >> 8) + ((var1 * (int64_t)_calib.P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)_calib.P1) >> 33;
    if (var1 == 0) return 0;
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)_calib.P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)_calib.P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)_calib.P7) << 4);
    return (float)p / 256.0f;
}

uint8_t BME280_Simple::compensateHumidity(int32_t adc_H, int32_t t_fine) {
    int32_t v_x1_u32r = (t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)_calib.H4) << 20) - (((int32_t)_calib.H5) * v_x1_u32r)) +
                   ((int32_t)16384)) >> 15) *
                 (((((((v_x1_u32r * ((int32_t)_calib.H6)) >> 10) *
                     (((v_x1_u32r * ((int32_t)_calib.H3)) >> 11) + ((int32_t)32768))) >> 10) +
                    ((int32_t)2097152)) * ((int32_t)_calib.H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)_calib.H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    return (uint8_t)((v_x1_u32r >> 12) / 1024);
}
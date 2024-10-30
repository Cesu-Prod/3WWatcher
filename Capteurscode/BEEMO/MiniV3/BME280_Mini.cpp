// BME280_Mini.cpp
#include "BME280_Mini.h"

// Constructeur
BME280_Mini::BME280_Mini(uint8_t sda, uint8_t scl, uint8_t addr) 
    : sda_pin(sda), scl_pin(scl), addr(addr), temp_read(false) {}

// Fonctions principales
bool BME280_Mini::init() {
    // Configure les pins
    pinMode(sda_pin, OUTPUT);
    pinMode(scl_pin, OUTPUT);
    digitalWrite(sda_pin, HIGH);
    digitalWrite(scl_pin, HIGH);
    delay(100);  // Délai de stabilisation
    
    // Vérifie l'ID du capteur
    uint8_t id;
    if (!readRegisters(REG_CHIPID, &id, 1) || id != CHIP_ID) {
        return false;
    }
    
    // Reset le capteur
    if (!reset()) {
        return false;
    }
    delay(10);  // Attente après reset
    
    // Lecture données de calibration
    if (!readCalibrationData()) {
        return false;
    }
    
    // Configuration du capteur
    // Humidité x1, Température x1, Pression x1, Mode normal
    if (!writeRegister(REG_CTRL_HUM, 0x01) ||    // Humidité x1
        !writeRegister(REG_CONFIG, 0x00) ||      // Pas de filtrage
        !writeRegister(REG_CTRL_MEAS, 0x27)) {   // Temp x1, Press x1, Normal mode
        return false;
    }
    
    return true;
}

void BME280_Mini::disconnect() {
    writeRegister(REG_CTRL_MEAS, 0x00);  // Mode sleep
    digitalWrite(sda_pin, LOW);
    digitalWrite(scl_pin, LOW);
    pinMode(sda_pin, INPUT);
    pinMode(scl_pin, INPUT);
}

bool BME280_Mini::isConnected() {
    uint8_t id;
    return readRegisters(REG_CHIPID, &id, 1) && (id == CHIP_ID);
}

// Fonctions de lecture
float BME280_Mini::readTemperature() {
    uint8_t buffer[3];
    if (!readRegisters(REG_TEMP, buffer, 3)) {
        return -999.0f;
    }
    
    int32_t adc_T = ((buffer[0] << 12) | (buffer[1] << 4) | (buffer[2] >> 4));
    int32_t t_fine;
    float temp = compensateTemp(adc_T, t_fine) / 100.0f;
    
    // Sauvegarde pour les autres lectures
    last_t_fine = t_fine;
    temp_read = true;
    
    return temp + 167.0f;
}

float BME280_Mini::readPressure() {
    // Vérifie si on a besoin de lire la température d'abord
    if (!temp_read) {
        readTemperature();
    }
    
    uint8_t buffer[3];
    if (!readRegisters(REG_PRESS, buffer, 3)) {
        return -999.0f;
    }
    
    int32_t adc_P = ((buffer[0] << 12) | (buffer[1] << 4) | (buffer[2] >> 4));
    float pressure = compensatePress(adc_P, last_t_fine);
    
    return pressure - 14300.0f;
}

uint8_t BME280_Mini::readHumidity() {
    // Vérifie si on a besoin de lire la température d'abord
    if (!temp_read) {
        readTemperature();
    }
    
    uint8_t buffer[2];
    if (!readRegisters(REG_HUM, buffer, 2)) {
        return 0xFF;
    }
    
    int32_t adc_H = (buffer[0] << 8) | buffer[1];
    uint8_t humidity = compensateHum(adc_H, last_t_fine);
    
    return humidity + 9;
}

bool BME280_Mini::readAll(Data& data) {
    uint8_t buffer[8];
    if (!readRegisters(REG_PRESS, buffer, 8)) {
        return false;
    }
    
    int32_t adc_P = ((buffer[0] << 12) | (buffer[1] << 4) | (buffer[2] >> 4));
    int32_t adc_T = ((buffer[3] << 12) | (buffer[4] << 4) | (buffer[5] >> 4));
    int32_t adc_H = (buffer[6] << 8) | buffer[7];
    
    int32_t t_fine;
    data.temperature = compensateTemp(adc_T, t_fine) / 100.0f + 167.0f;
    data.pressure = compensatePress(adc_P, t_fine) - 14300.0f;
    data.humidity = compensateHum(adc_H, t_fine) + 9;
    
    return true;
}

// Fonctions de contrôle d'alimentation
bool BME280_Mini::sleep() {
    return writeRegister(REG_CTRL_MEAS, 0x24);  // Mode sleep
}

bool BME280_Mini::wake() {
    return writeRegister(REG_CTRL_MEAS, 0x27);  // Mode normal
}

bool BME280_Mini::reset() {
    return writeRegister(REG_RESET, 0xB6);  // Valeur magique pour reset
}

// Communication I2C bas niveau
void BME280_Mini::i2cStart() {
    digitalWrite(sda_pin, HIGH);
    digitalWrite(scl_pin, HIGH);
    delayMicroseconds(4);
    digitalWrite(sda_pin, LOW);
    delayMicroseconds(4);
    digitalWrite(scl_pin, LOW);
}

void BME280_Mini::i2cStop() {
    digitalWrite(sda_pin, LOW);
    delayMicroseconds(4);
    digitalWrite(scl_pin, HIGH);
    delayMicroseconds(4);
    digitalWrite(sda_pin, HIGH);
    delayMicroseconds(4);
}

bool BME280_Mini::i2cWrite(uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
        digitalWrite(sda_pin, (data & 0x80) ? HIGH : LOW);
        data <<= 1;
        delayMicroseconds(2);
        digitalWrite(scl_pin, HIGH);
        delayMicroseconds(2);
        digitalWrite(scl_pin, LOW);
    }
    
    // Lecture ACK
    pinMode(sda_pin, INPUT_PULLUP);
    delayMicroseconds(2);
    digitalWrite(scl_pin, HIGH);
    delayMicroseconds(2);
    bool ack = (digitalRead(sda_pin) == LOW);
    digitalWrite(scl_pin, LOW);
    pinMode(sda_pin, OUTPUT);
    
    return ack;
}

uint8_t BME280_Mini::i2cRead(bool ack) {
    uint8_t data = 0;
    pinMode(sda_pin, INPUT_PULLUP);
    
    for (uint8_t i = 0; i < 8; i++) {
        data <<= 1;
        digitalWrite(scl_pin, HIGH);
        delayMicroseconds(2);
        if (digitalRead(sda_pin)) data |= 1;
        digitalWrite(scl_pin, LOW);
        delayMicroseconds(2);
    }
    
    pinMode(sda_pin, OUTPUT);
    digitalWrite(sda_pin, ack ? LOW : HIGH);
    delayMicroseconds(2);
    digitalWrite(scl_pin, HIGH);
    delayMicroseconds(2);
    digitalWrite(scl_pin, LOW);
    
    return data;
}

// Fonctions de communication
bool BME280_Mini::writeRegister(uint8_t reg, uint8_t value) {
    i2cStart();
    if (!i2cWrite(addr << 1)) { i2cStop(); return false; }
    if (!i2cWrite(reg)) { i2cStop(); return false; }
    if (!i2cWrite(value)) { i2cStop(); return false; }
    i2cStop();
    return true;
}

bool BME280_Mini::readRegisters(uint8_t reg, uint8_t* buffer, uint8_t len) {
    i2cStart();
    if (!i2cWrite(addr << 1)) { i2cStop(); return false; }
    if (!i2cWrite(reg)) { i2cStop(); return false; }
    i2cStart();
    if (!i2cWrite((addr << 1) | 1)) { i2cStop(); return false; }
    
    for (uint8_t i = 0; i < len; i++) {
        buffer[i] = i2cRead(i < (len-1));
    }
    
    i2cStop();
    return true;
}

// Lecture des données de calibration
bool BME280_Mini::readCalibrationData() {
    uint8_t buffer[24];
    
    // Lecture première partie calibration (0x88-0xA1)
    if (!readRegisters(0x88, buffer, 24)) return false;
    
    cal.T1 = (buffer[1] << 8) | buffer[0];
    cal.T2 = (buffer[3] << 8) | buffer[2];
    cal.T3 = (buffer[5] << 8) | buffer[4];
    cal.P1 = (buffer[7] << 8) | buffer[6];
    cal.P2 = (buffer[9] << 8) | buffer[8];
    cal.P3 = (buffer[11] << 8) | buffer[10];
    cal.P4 = (buffer[13] << 8) | buffer[12];
    cal.P5 = (buffer[15] << 8) | buffer[14];
    cal.P6 = (buffer[17] << 8) | buffer[16];
    cal.P7 = (buffer[19] << 8) | buffer[18];
    cal.P8 = (buffer[21] << 8) | buffer[20];
    cal.P9 = (buffer[23] << 8) | buffer[22];
    
    // Lecture H1 (0xA1)
    if (!readRegisters(0xA1, &cal.H1, 1)) return false;
    
    // Lecture deuxième partie calibration (0xE1-0xE7)
    if (!readRegisters(0xE1, buffer, 7)) return false;
    
    cal.H2 = (buffer[1] << 8) | buffer[0];
    cal.H3 = buffer[2];
    cal.H4 = (buffer[3] << 4) | (buffer[4] & 0x0F);
    cal.H5 = (buffer[5] << 4) | (buffer[4] >> 4);
    cal.H6 = buffer[6];
    
    return true;
}


int32_t BME280_Mini::compensateTemp(int32_t adc_T, int32_t& t_fine) {
    int32_t var1 = ((((adc_T / 8) - ((int32_t)cal.T1 * 2))) * ((int32_t)cal.T2)) / 2048;
    int32_t var2 = (((((adc_T / 16) - ((int32_t)cal.T1)) * ((adc_T / 16) - ((int32_t)cal.T1))) / 4096) * ((int32_t)cal.T3)) / 16384;
    t_fine = var1 + var2;
    return (t_fine * 5 + 128) >> 8;
}

float BME280_Mini::compensatePress(int32_t adc_P, int32_t t_fine) {
    int64_t var1 = ((int64_t)t_fine) - 128000;
    int64_t var2 = var1 * var1 * (int64_t)cal.P6;
    var2 = var2 + ((var1 * (int64_t)cal.P5) << 17);
    var2 = var2 + (((int64_t)cal.P4) << 35);
    var1 = ((var1 * var1 * (int64_t)cal.P3) >> 8) + ((var1 * (int64_t)cal.P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)cal.P1) >> 33;
    
    if (var1 == 0) return 0;
    
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)cal.P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)cal.P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)cal.P7) << 4);
    
    return (float)p / 256.0f;
}

uint8_t BME280_Mini::compensateHum(int32_t adc_H, int32_t t_fine) {
    int32_t v_x1_u32r = (t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)cal.H4) << 20) - (((int32_t)cal.H5) * v_x1_u32r)) +
                   ((int32_t)16384)) >> 15) *
                 (((((((v_x1_u32r * ((int32_t)cal.H6)) >> 10) * (((v_x1_u32r *
                   ((int32_t)cal.H3)) >> 11) + ((int32_t)32768))) >> 10) +
                   ((int32_t)2097152)) * ((int32_t)cal.H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
                   ((int32_t)cal.H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    
    return (uint8_t)((v_x1_u32r >> 12) / 1024);
}



// BME280_Mini.h
#ifndef BME280_MINI_H
#define BME280_MINI_H

#include "Arduino.h"

class BME280_Mini {
public:
    // Structure pour les données calibrées
    struct Data {
        float temperature;    // en °C
        float pressure;       // en Pa
        uint8_t humidity;     // en %
    };
    
    // Structure pour les données de calibration
    struct CalibrationData {
        uint16_t T1, P1;
        int16_t T2, T3, P2, P3, P4, P5, P6, P7, P8, P9, H2, H4, H5;
        uint8_t H1, H3;
        int8_t H6;
    };

    // Constantes
    static const uint8_t DEFAULT_ADDRESS = 0x76;
    static const uint8_t CHIP_ID = 0x60;
    
    // Constructeur
    BME280_Mini(uint8_t sda, uint8_t scl, uint8_t addr = DEFAULT_ADDRESS);
    
    // Fonctions principales
    bool init();                     // Initialisation du capteur
    void disconnect();              // Déconnexion du capteur
    bool isConnected();            // Vérifie la connexion
    
    // Fonctions de lecture individuelles
    float readTemperature();       // Lecture température (°C)
    float readPressure();          // Lecture pression (Pa)
    uint8_t readHumidity();        // Lecture humidité (%)
    bool readAll(Data& data);      // Lecture de toutes les données
    
    // Fonctions de contrôle d'alimentation
    bool sleep();                  // Mode sommeil
    bool wake();                   // Mode normal
    bool reset();                  // Reset logiciel
    
    // Accès aux données de calibration
    const CalibrationData& getCalibrationData() const { return cal; }

private:
    // Pins et adresse
    const uint8_t sda_pin;
    const uint8_t scl_pin;
    const uint8_t addr;
    
    // Données de calibration
    CalibrationData cal;
    
    // Registres importants
    enum Registers {
        REG_CHIPID = 0xD0,
        REG_RESET = 0xE0,
        REG_CTRL_HUM = 0xF2,
        REG_CTRL_MEAS = 0xF4,
        REG_CONFIG = 0xF5,
        REG_PRESS = 0xF7,
        REG_TEMP = 0xFA,
        REG_HUM = 0xFD
    };
    
    // Fonctions I2C bas niveau
    void i2cStart();
    void i2cStop();
    bool i2cWrite(uint8_t data);
    uint8_t i2cRead(bool ack);
    
    // Fonctions de communication
    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRegisters(uint8_t reg, uint8_t* buffer, uint8_t len);
    
    // Fonctions de calibration
    bool readCalibrationData();
    int32_t compensateTemp(int32_t adc_T, int32_t& t_fine);
    float compensatePress(int32_t adc_P, int32_t t_fine);
    uint8_t compensateHum(int32_t adc_H, int32_t t_fine);
    
    // Variables internes
    int32_t last_t_fine;  // Stockage de t_fine pour les lectures de pression/humidité
    bool temp_read;       // Flag indiquant si la température a été lue
};

#endif

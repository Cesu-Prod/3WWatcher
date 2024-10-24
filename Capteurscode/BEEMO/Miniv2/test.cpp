#include <Arduino.h>
#include "BME280_Mini.h"

extern "C" void __attribute__((weak)) yield(void) {} // Spécifique au compil, peux être retiré.

// Définir les pins I2C
const uint8_t SDA_PIN = A4; // Pin SDA pour Arduino UNO
const uint8_t SCL_PIN = A3; // Pin SCL pour Arduino UNO

BME280_Mini bme(SDA_PIN, SCL_PIN); // Utilise l'adresse par défaut (0x76)
// Pour utiliser l'adresse alternative:
// BME280_Mini bme(SDA_PIN, SCL_PIN, 0x77);

void setup() {
    Serial.begin(9600);
    
    // Attendre que le port série soit prêt
    while (!Serial && millis() < 5000);
    
    Serial.println(F("Test BME280 Mini (Sans Wire)"));
    
    // Initialiser le capteur
    if (!bme.begin()) {
        Serial.println(F("Erreur: BME280 non trouvé!"));
    }
    
    Serial.println(F("BME280 initialisé"));
    Serial.println(F("Temp(°C) Press(hPa) Hum(%)"));
}

void loop() {
    BME280_Mini::Data data;
    
    // Lecture des données
    if (bme.read(data)) {
        Serial.print(data.temperature, 1);
        Serial.print(F("°C "));
        
        Serial.print(data.pressure / 100.0f, 1);
        Serial.print(F("hPa "));
        
        Serial.print(data.humidity);
        Serial.println(F("%"));
    } else {
        Serial.println(F("Erreur de lecture"));
    }
    
    delay(2000);
}

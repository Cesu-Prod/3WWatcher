#include "Arduino.h"
#include "BME280_Mini.h"

extern "C" void __attribute__((weak)) yield(void) {}

const uint8_t SDA_PIN = A4;
const uint8_t SCL_PIN = A3;

BME280_Mini bme(SDA_PIN, SCL_PIN);

void printMeasurement() {
    BME280_Mini::Data data;
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
}

void setup() {
    Serial.begin(9600);
    
    while (!Serial && millis() < 5000);
    
    Serial.println(F("Test BME280 Mini avec Sleep/Wake"));
    
    if (!bme.begin()) {
        Serial.println(F("Erreur: BME280 non trouvé!"));
        while (1);
    }
    
    Serial.println(F("BME280 initialisé"));
    Serial.println(F("Temp(°C) Press(hPa) Hum(%)"));
}

void loop() {
    // Prendre une mesure en mode normal
    Serial.println(F("\nMode normal:"));
    printMeasurement();
    
    // Mettre en mode sleep
    Serial.println(F("Passage en mode sleep..."));
    if (!bme.sleep()) {
        Serial.println(F("Erreur passage en sleep"));
    }
    delay(2000);
    
    // Essayer de prendre une mesure en mode sleep
    Serial.println(F("Tentative mesure en mode sleep:"));
    printMeasurement();
    
    // Réveiller le capteur
    Serial.println(F("Réveil du capteur..."));
    if (!bme.wake()) {
        Serial.println(F("Erreur réveil"));
    }
    delay(500); // Laisser le temps au capteur de se stabiliser
    
    // Prendre une nouvelle mesure après réveil
    Serial.println(F("Mesure après réveil:"));
    printMeasurement();
    
    delay(5000); // Attendre 5 secondes avant de recommencer le cycle
}

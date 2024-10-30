#include "Arduino.h"
#include "BME280_Mini.h"


extern "C" void __attribute__((weak)) yield(void) {} 



// Définir les pins I2C
const uint8_t SDA_PIN = A4;
const uint8_t SCL_PIN = A3;

// Créer l'instance du capteur
BME280_Mini bme(SDA_PIN, SCL_PIN);  // Adresse par défaut 0x76

void setup() {
    Serial.begin(9600);
    while (!Serial && millis() < 5000);
    
    Serial.println(F("Test BME280 Mini"));
    
    // Initialisation
    if (!bme.init()) {
        Serial.println(F("Erreur: BME280 non trouvé!"));
        while (1);
    }
    
    Serial.println(F("BME280 initialisé"));
}

void loop() {
    // Méthode 1: Lecture individuelle
    float temp = bme.readTemperature();
    float press = bme.readPressure();
    uint8_t hum = bme.readHumidity();
    
    Serial.print(F("Température: "));
    Serial.print(temp, 1);
    Serial.println(F("°C"));
    
    Serial.print(F("Pression: "));
    Serial.print(press / 100.0, 1);  // Conversion en hPa
    Serial.println(F(" hPa"));
    
    Serial.print(F("Humidité: "));
    Serial.print(hum);
    Serial.println(F("%"));
    
    // Méthode 2: Lecture de toutes les données en une fois
    BME280_Mini::Data data;
    if (bme.readAll(data)) {
        Serial.print(F("T:"));
        Serial.print(data.temperature, 1);
        Serial.print(F("°C P:"));
        Serial.print(data.pressure / 100.0, 1);
        Serial.print(F("hPa H:"));
        Serial.print(data.humidity);
        Serial.println(F("%"));
    }
    
    delay(2000);
}

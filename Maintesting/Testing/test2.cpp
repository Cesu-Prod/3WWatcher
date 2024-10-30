#include "Arduino.h"
#include "BME280_Mini.h" 


extern "C" void __attribute__((weak)) yield(void) {}

const uint8_t SDA_PIN = A4; 
const uint8_t SCL_PIN = A3; 
BME280_Mini bme(SDA_PIN, SCL_PIN); 

void setup() { 
  Serial.begin(9600);
  while (!Serial && millis() < 5000);
  Serial.println(F("Test BME280 Mini (Sans Wire)"));
  if (!bme.begin()) { 
    Serial.println(F("Erreur: BME280 non trouvé!"));
    while(1);
  }
  Serial.println(F("BME280 initialisé")); 
  Serial.println(F("Temp(°C) Press(hPa) Hum(%)")); 
} 

void loop() { 
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
  delay(2000); 
}
#include "Wire.h"
#include "Arduino.h"

extern "C" void __attribute__((weak)) yield(void) {} 


struct DateTime {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};

DateTime readRTC() {
  DateTime dt;
  Wire.beginTransmission(0x68); // Adresse I2C du DS1307
  Wire.write(0x00); // Pointeur sur registre des secondes
  Wire.endTransmission();
  
  Wire.requestFrom(0x68, 7); // Lecture des 7 registres
  
  dt.second = bcdToDec(Wire.read());
  dt.minute = bcdToDec(Wire.read());
  dt.hour = bcdToDec(Wire.read());
  Wire.read(); // Jour de la semaine (ignoré)
  dt.day = bcdToDec(Wire.read());
  dt.month = bcdToDec(Wire.read());
  dt.year = bcdToDec(Wire.read()) + 2000;
  
  return dt;
}

// Conversion BCD vers décimal
uint8_t bcdToDec(uint8_t val) {
  return (val/16*10) + (val%16);
}


void setup() {
  Wire.begin();
  Serial.begin(9600);
}

void loop() {
  DateTime dt = readRTC();
  Serial.printf("%02d/%02d/%d %02d:%02d:%02d\n", 
    dt.day, dt.month, dt.year,
    dt.hour, dt.minute, dt.second);
  delay(1000);
}

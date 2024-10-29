#include <Wire.h>
#include "RTClib.h"

RTC_DS1307 rtc;

const char jours[] PROGMEM = "SUNMONTUEWEDTHUFRISAM"; // Stocké en mémoire programme

void setup() {
    Serial.begin(9600);
    
    if (!rtc.begin()) {
        Serial.println(F("RTC error")); // F() stocke la chaîne en mémoire flash
        while (1);
    }
}

void loop() {
    static DateTime now = rtc.now(); // Variable statique pour réduire la pile
    
    // Affichage optimisé de l'heure
    printDigits(now.hour());
    Serial.print(F(":"));
    printDigits(now.minute());
    Serial.print(F(":"));
    printDigits(now.second());
    Serial.print(F("  "));
    
    // Affichage optimisé de la date
    printDigits(now.day());
    Serial.print(F("/"));
    printDigits(now.month());
    Serial.print(F("/"));
    Serial.print(now.year());
    Serial.print(F(" "));
    
    // Affichage optimisé du jour
    uint8_t jour = now.dayOfTheWeek() * 3;
    for(uint8_t i = 0; i < 3; i++) {
        Serial.print((char)pgm_read_byte(&jours[jour + i]));
    }
    
    Serial.println();
    delay(10000);
}

// Fonction optimisée pour afficher les nombres < 10 avec un zéro devant
void printDigits(uint8_t digits) {
    if(digits < 10) Serial.print(F("0"));
    Serial.print(digits, DEC);
}
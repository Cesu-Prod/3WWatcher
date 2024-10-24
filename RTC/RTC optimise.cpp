#include <Arduino.h>
#include "RTC.h"

#define SDA_PIN 4  // Changez ceci selon votre configuration
#define SCL_PIN 5  // Changez ceci selon votre configuration

bool DS1307::begin() {
    pinMode(SDA_PIN, OUTPUT);
    pinMode(SCL_PIN, OUTPUT);
    digitalWrite(SDA_PIN, HIGH);
    digitalWrite(SCL_PIN, HIGH);
    return true;
}

void DS1307::i2cStart() {
    digitalWrite(SDA_PIN, LOW);
    digitalWrite(SCL_PIN, LOW);
}

void DS1307::i2cStop() {
    digitalWrite(SDA_PIN, LOW);
    digitalWrite(SCL_PIN, HIGH);
    digitalWrite(SDA_PIN, HIGH);
}

void DS1307::i2cWrite(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        digitalWrite(SDA_PIN, (data & 0x80) ? HIGH : LOW);
        digitalWrite(SCL_PIN, HIGH);
        digitalWrite(SCL_PIN, LOW);
        data <<= 1;
    }
    pinMode(SDA_PIN, INPUT); // Libérer SDA pour lire l'ACK
    digitalWrite(SCL_PIN, HIGH);
    digitalWrite(SCL_PIN, LOW);
    pinMode(SDA_PIN, OUTPUT); // Remettre SDA en mode sortie
}

uint8_t DS1307::i2cRead(bool ack) {
    uint8_t data = 0;
    pinMode(SDA_PIN, INPUT); // Mettre SDA en mode entrée
    for (int i = 0; i < 8; i++) {
        digitalWrite(SCL_PIN, HIGH);
        data = (data << 1) | digitalRead(SDA_PIN);
        digitalWrite(SCL_PIN, LOW);
    }
    pinMode(SDA_PIN, OUTPUT); // Remettre SDA en mode sortie
    digitalWrite(SDA_PIN, ack ? LOW : HIGH); // Envoyer ACK ou NACK
    digitalWrite(SCL_PIN, HIGH);
    digitalWrite(SCL_PIN, LOW);
    return data;
}

bool DS1307::isRunning() {
    i2cStart();
    i2cWrite(DS1307_ADDR << 1); // Écriture
    i2cWrite(0x00); // Adresse des secondes
    i2cStart(); // Redémarrer pour lire
    i2cWrite((DS1307_ADDR << 1) | 1); // Lecture
    uint8_t seconds = i2cRead(false);
    i2cStop();
    return !(seconds & 0x80); // Vérifie le bit CH (Clock Halt)
}

void DS1307::startClock() {
    i2cStart();
    i2cWrite(DS1307_ADDR << 1);
    i2cWrite(0x00); // Adresse des secondes
    uint8_t seconds = i2cRead(false);
    seconds &= 0x7F; // Clear CH bit
    i2cWrite(seconds);
    i2cStop();
}

void DS1307::stopClock() {
    i2cStart();
    i2cWrite(DS1307_ADDR << 1);
    i2cWrite(0x00); // Adresse des secondes
    uint8_t seconds = i2cRead(false);
    seconds |= 0x80; // Set CH bit
    i2cWrite(seconds);
    i2cStop();
}

void DS1307:: setTime(uint8_t hours, uint8_t minutes, uint8_t seconds) {
    i2cStart();
    i2cWrite(DS1307_ADDR << 1);
    i2cWrite(0x00); // Adresse des secondes
    i2cWrite(bin2bcd(seconds));
    i2cWrite(bin2bcd(minutes));
    i2cWrite(bin2bcd(hours));
    i2cStop();
}

void DS1307::getTime(uint8_t &hours, uint8_t &minutes, uint8_t &seconds) {
    i2cStart();
    i2cWrite(DS1307_ADDR << 1);
    i2cWrite(0x00); // Adresse des secondes
    i2cStart(); // Redémarrer pour lire
    i2cWrite((DS1307_ADDR << 1) | 1); // Lecture
    seconds = bcd2bin(i2cRead(true));
    minutes = bcd2bin(i2cRead(true));
    hours = bcd2bin(i2cRead(false));
    i2cStop();

    // Debugging
    Serial.print("Valeurs brutes lues : ");
    Serial.print(hours);
    Serial.print(", ");
    Serial.print(minutes);
    Serial.print(", ");
    Serial.println(seconds);
}

void DS1307::setDate(uint8_t day, uint8_t month, uint8_t year) {
    i2cStart();
    i2cWrite(DS1307_ADDR << 1);
    i2cWrite(0x04); // Adresse du jour
    i2cWrite(bin2bcd(day));
    i2cWrite(bin2bcd(month));
    i2cWrite(bin2bcd(year));
    i2cStop();
}

void DS1307::getDate(uint8_t &day, uint8_t &month, uint8_t &year) {
    i2cStart();
    i2cWrite(DS1307_ADDR << 1);
    i2cWrite(0x04); // Adresse du jour
    i2cStart(); // Redémarrer pour lire
    i2cWrite((DS1307_ADDR << 1) | 1); // Lecture
    day = bcd2bin(i2cRead(true));
    month = bcd2bin(i2cRead(true));
    year = bcd2bin(i2cRead(false));
    i2cStop();

    // Debugging
    Serial.print("Valeurs brutes lues : ");
    Serial.print(day);
    Serial.print(", ");
    Serial.print(month);
    Serial.print(", ");
    Serial.println(year);
}

uint8_t DS1307::bcd2bin(uint8_t val) {
    return val - 6 * (val >> 4);
}

uint8_t DS1307::bin2bcd(uint8_t val) {
    return val + 6 * (val / 10);
}
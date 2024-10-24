#include "RTC.h"
#include <Arduino.h>

#define SDA 2  // Remplacez avec la broche appropriée pour SDA
#define SCL 3  // Remplacez avec la broche appropriée pour SCL

bool DS1307::begin() {
    pinMode(SDA, OUTPUT);
    pinMode(SCL, OUTPUT);
    return true;
}

void DS1307::setSeconds(uint8_t seconds) {
    rtc_write(0x00, bin2bcd(seconds));
}

uint8_t DS1307::getSeconds() {
    return bcd2bin(rtc_read(0x00));
}

void DS1307::setMinutes(uint8_t minutes) {
    rtc_write(0x01, bin2bcd(minutes));
}

uint8_t DS1307::getMinutes() {
    return bcd2bin(rtc_read(0x01));
}

void DS1307::setHours(uint8_t hours) {
    rtc_write(0x02, bin2bcd(hours));
}

uint8_t DS1307::getHours() {
    return bcd2bin(rtc_read(0x02));
}

void DS1307::setDay(uint8_t day) {
    rtc_write(0x03, bin2bcd(day));
}

uint8_t DS1307::getDay() {
    return bcd2bin(rtc_read(0x03));
}

void DS1307::setMonth(uint8_t month) {
    rtc_write(0x05, bin2bcd(month));
}

uint8_t DS1307::getMonth() {
    return bcd2bin(rtc_read(0x05));
}

void DS1307::setYear(uint16_t year) {
    rtc_write(0x06, bin2bcd(year - 2000));  // Supposons que l'année soit après 2000
}

uint16_t DS1307::getYear() {
    return bcd2bin(rtc_read(0x06)) + 2000;  // Supposons que l'année soit après 2000
}

void DS1307::setDateTime(uint8_t hour, uint8_t minute, uint8_t second, uint8_t day, uint8_t month, uint16_t year) {
    setSeconds(second);
    setMinutes(minute);
    setHours(hour);
    setDay(day);
    setMonth(month);
    setYear(year);
}

void DS1307::i2c_start() {
    digitalWrite(SDA, HIGH);
    digitalWrite(SCL, HIGH);
    delayMicroseconds(5);
    digitalWrite(SDA, LOW);
    delayMicroseconds(5);
    digitalWrite(SCL, LOW);
}

void DS1307::i2c_stop() {
    digitalWrite(SDA, LOW);
    digitalWrite(SCL, HIGH);
    delayMicroseconds(5);
    digitalWrite(SDA, HIGH);
    delayMicroseconds(5);
}

void DS1307::i2c_write_bit(bool bit) {
    digitalWrite(SDA, bit);
    delayMicroseconds(5);
    digitalWrite(SCL, HIGH);
    delayMicroseconds(5);
    digitalWrite(SCL, LOW);
}

bool DS1307::i2c_read_bit() {
    pinMode(SDA, INPUT);
    delayMicroseconds(5);
    digitalWrite(SCL, HIGH);
    delayMicroseconds(5);
    bool bit = digitalRead(SDA);
    digitalWrite(SCL, LOW);
    pinMode(SDA, OUTPUT);
    return bit;
}

void DS1307::i2c_write_byte(uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        i2c_write_bit(byte & 0x80);
        byte <<= 1;
    }
    i2c_read_bit();  // Attente de l'ACK
}

uint8_t DS1307::i2c_read_byte(bool ack) {
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        byte <<= 1;
        if (i2c_read_bit()) byte |= 0x01;
    }
    i2c_write_bit(ack ? 0 : 1);  // Envoie l'ACK ou NACK
    return byte;
}

void DS1307::rtc_write(uint8_t addr, uint8_t data) {
    i2c_start();
    i2c_write_byte(DS1307_ADDR << 1);  // Adresse du périphérique + bit d'écriture
    i2c_write_byte(addr);  // Adresse du registre à écrire
    i2c_write_byte(data);  // Donnée à écrire
    i2c_stop();
}

uint8_t DS1307::rtc_read(uint8_t addr) {
    uint8_t data;
    i2c_start();
    i2c_write_byte(DS1307_ADDR << 1);  // Adresse du périphérique + bit d'écriture
    i2c_write_byte(addr);  // Adresse du registre à lire
    i2c_start();  // Restart
    i2c_write_byte((DS1307_ADDR << 1) | 1);  // Adresse du périphérique + bit de lecture
    data = i2c_read_byte(false);  // Lire le registre
    i2c_stop();
    return data;
}

uint8_t DS1307::bcd2bin(uint8_t val) {
    return val - 6 * (val >> 4);
}

uint8_t DS1307::bin2bcd(uint8_t val) {
    return val + 6 * (val / 10);
}
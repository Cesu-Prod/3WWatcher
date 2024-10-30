#include "Arduino.h"


extern "C" void __attribute__((weak)) yield(void) {}  // Acompil specific line, allows the compiler to be more optimized, but not necessary on unoptimized versions of Acompil or on better compilers.

struct DateTime {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t date;
    uint8_t month;
    uint8_t year;
};


// DS1307 I2C address
#define DS1307_ADDRESS 0x68

// Register addresses
#define SECONDS_REG 0x00
#define MINUTES_REG 0x01
#define HOURS_REG 0x02
#define DAY_REG 0x03
#define DATE_REG 0x04
#define MONTH_REG 0x05
#define YEAR_REG 0x06
#define CONTROL_REG 0x07

// Convert BCD to decimal
uint8_t bcdToDec(uint8_t bcd) {
    return ((bcd / 16) * 10) + (bcd % 16);
}

// Initialize I2C
void initI2C() {
    TWSR = 0;
    TWBR = ((F_CPU/100000)-16)/2;
    TWCR = (1 << TWEN);
}

uint8_t readRTCRegister(uint8_t reg) {
    // Start condition
    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Send device address for write
    TWDR = DS1307_ADDRESS << 1;
    TWCR = (1<<TWINT) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Send register address
    TWDR = reg;
    TWCR = (1<<TWINT) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Repeated start
    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Send device address for read
    TWDR = (DS1307_ADDRESS << 1) | 0x01;
    TWCR = (1<<TWINT) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Read data with NACK
    TWCR = (1<<TWINT) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    uint8_t data = TWDR;

    // Stop condition
    TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);

    return data;
}


// Convert decimal to BCD
uint8_t decToBcd(uint8_t dec) {
    return ((dec / 10) << 4) + (dec % 10);
}

DateTime readDateTime() {
    DateTime dt;
    dt.second = bcdToDec(readRTCRegister(SECONDS_REG) & 0x7F);
    dt.minute = bcdToDec(readRTCRegister(MINUTES_REG));
    dt.hour = bcdToDec(readRTCRegister(HOURS_REG) & 0x3F);
    dt.day = bcdToDec(readRTCRegister(DAY_REG));
    dt.date = bcdToDec(readRTCRegister(DATE_REG));
    dt.month = bcdToDec(readRTCRegister(MONTH_REG));
    dt.year = bcdToDec(readRTCRegister(YEAR_REG));
    return dt;
}
void writeRTCRegister(uint8_t reg, uint8_t value) {
    // Start condition
    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Send device address for write
    TWDR = DS1307_ADDRESS << 1;
    TWCR = (1<<TWINT) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Send register address
    TWDR = reg;
    TWCR = (1<<TWINT) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Send data
    TWDR = value;
    TWCR = (1<<TWINT) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Stop condition
    TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
}
void deinitI2C() {
    // Disable TWI
    TWCR = 0;
    // Set SDA and SCL pins to input mode
    DDRC &= ~((1 << PINC4) | (1 << PINC5));
    PORTC &= ~((1 << PINC4) | (1 << PINC5));
}

void setTime(uint8_t hour, uint8_t minute, uint8_t second) {
    writeRTCRegister(SECONDS_REG, decToBcd(second));
    writeRTCRegister(MINUTES_REG, decToBcd(minute));
    writeRTCRegister(HOURS_REG, decToBcd(hour));
}

void setDate(uint8_t date, uint8_t month, uint8_t year) {
    writeRTCRegister(DATE_REG, decToBcd(date));
    writeRTCRegister(MONTH_REG, decToBcd(month));
    writeRTCRegister(YEAR_REG, decToBcd(year));
}

void setDay(uint8_t day) {
    writeRTCRegister(DAY_REG, decToBcd(day));
}

const char* const week_days[7] PROGMEM = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};

void setup() {
    Serial.begin(9600);
    initI2C();
/*  setDate(30,10,24);
    setTime(17,26,4);
    setDay(3); */

}

void loop() {
    DateTime now = readDateTime();
    Serial.print(week_days[1]);
    Serial.print(" ");
    Serial.print(now.date);
    Serial.print("/");
    Serial.print(now.month);
    Serial.print("/");
    Serial.print(now.year);
    Serial.print(" ");
    Serial.print(now.hour);
    Serial.print(":");
    Serial.print(now.minute);
    Serial.print(":");
    Serial.println(now.second);
    
    delay(1000);
}
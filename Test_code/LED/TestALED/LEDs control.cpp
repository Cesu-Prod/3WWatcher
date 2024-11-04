#include <Arduino.h>

extern "C" void __attribute__((weak)) yield(void) {}

// Constants
#define _CLK_PULSE_DELAY 20

// Global variables
byte _clk_pin;
byte _data_pin;
byte _current_red;
byte _current_green;
byte _current_blue;

// Function declarations
void clk(void);
void sendByte(byte b);
void sendColor(byte red, byte green, byte blue);
void setColorRGB(byte red, byte green, byte blue);
void Init_LED(byte clk_pin, byte data_pin);

// Function implementations
void clk(void) {
    digitalWrite(_clk_pin, LOW);
    delayMicroseconds(_CLK_PULSE_DELAY);
    digitalWrite(_clk_pin, HIGH);
    delayMicroseconds(_CLK_PULSE_DELAY);
}

void sendByte(byte b) {
    for (byte i = 0; i < 8; i++) {
        digitalWrite(_data_pin, (b & 0x80) ? HIGH : LOW);
        clk();
        b <<= 1;
    }
}

void sendColor(byte red, byte green, byte blue) {
    byte prefix = B11000000;
    if ((blue & 0x80) == 0) prefix |= B00100000;
    if ((blue & 0x40) == 0) prefix |= B00010000;
    if ((green & 0x80) == 0) prefix |= B00001000;
    if ((green & 0x40) == 0) prefix |= B00000100;
    if ((red & 0x80) == 0) prefix |= B00000010;
    if ((red & 0x40) == 0) prefix |= B00000001;
    
    sendByte(prefix);
    sendByte(blue);
    sendByte(green);
    sendByte(red);
}

void Init_LED(byte clk_pin, byte data_pin) {
    _clk_pin = clk_pin;
    _data_pin = data_pin;
    _current_red = 0;
    _current_green = 0;
    _current_blue = 0;
    
    pinMode(_clk_pin, OUTPUT);
    pinMode(_data_pin, OUTPUT);
    setColorRGB(0, 0, 0);
}

void setColorRGB(byte red, byte green, byte blue) {
    _current_red = red;
    _current_green = green;
    _current_blue = blue;
    
    // Send data frame prefix
    for (byte i = 0; i < 4; i++) {
        sendByte(0x00);
    }
    
    sendColor(red, green, blue);
}

void setup() {
    Init_LED(7, 8);
}

void loop() {
    setColorRGB(0, 20, 255);  // White
    delay(500);
    setColorRGB(255, 20, 0);     // Red-Orange
    delay(500);
}
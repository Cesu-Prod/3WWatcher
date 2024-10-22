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
int err_code;
bool mode;

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

void ColorerLED(int couleur1[3], int couleur2[3], bool is_second_longer) {
    setColorRGB(couleur1[0], couleur1[1], couleur1[2]);
    delay(1000);
    setColorRGB(couleur2[0], couleur2[1], couleur2[2]);
    if (is_second_longer) {
        delay(2000);
    } else {
        delay(1000);
    }
}

void toggleLED() {
    if (err_code > 0) {
        unsigned char color1[3] = {255, 0, 0};
        unsigned char color2[3];
        bool is_second_longer;

        switch (err_code) {
            case 1:
                color2[0] = 0;
                color2[1] = 0;
                color2[2] = 255;
                is_second_longer = false;
                break;
            case 2:
                color2[0] = 255;
                color2[1] = 125;
                color2[2] = 0;
                is_second_longer = false;
                break;
            case 3:
                color2[0] = 0;
                color2[1] = 255;
                color2[2] = 0;
                is_second_longer = false;
                break;
            case 4:
                color2[0] = 0;
                color2[1] = 255;
                color2[2] = 0;
                is_second_longer = true;
                break;
            case 5:
                color2[0] = 255;
                color2[1] = 255;
                color2[2] = 255;
                is_second_longer = false;
                break;
            case 6:
                color2[0] = 255;
                color2[1] = 255;
                color2[2] = 255;
                is_second_longer = true;
                break;
        }
        ColorerLED(color1, color2, is_second_longer);
    } else {
        if (mode) {
            setColorRGB(0, 255, 0);
        } else {
            setColorRGB(0, 0, 255);
        }
    }
}

void setup() {
    Init_LED(7, 8);
}

void loop() {
    err_code = 1;
    toggleLED()

    err_code = 2;
    toggleLED()

    err_code = 3;
    toggleLED()

    err_code = 4;
    toggleLED()

    err_code = 5;
    toggleLED()

    err_code = 6;
    toggleLED()
}
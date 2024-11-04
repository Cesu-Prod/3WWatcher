
#include "ALED.h"

void Init_LED(byte clk_pin, byte data_pin) :
    _clk_pin(clk_pin), _data_pin(data_pin),
    _current_red(0), _current_green(0), _current_blue(0) {
    pinMode(_clk_pin, OUTPUT);
    pinMode(_data_pin, OUTPUT);
    setColorRGB(0, 0, 0);
}

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
    if ((blue & 0x80) == 0)  prefix |= B00100000;
    if ((blue & 0x40) == 0)  prefix |= B00010000;
    if ((green & 0x80) == 0) prefix |= B00001000;
    if ((green & 0x40) == 0) prefix |= B00000100;
    if ((red & 0x80) == 0)   prefix |= B00000010;
    if ((red & 0x40) == 0)   prefix |= B00000001;
    
    sendByte(prefix);
    sendByte(blue);
    sendByte(green);
    sendByte(red);
}

void setColorRGB(byte red, byte green, byte blue) {
    _current_red = red;
    _current_green = green;
    _current_blue = blue;
    for (byte i = 0; i < 4; i++) {
        sendByte(0x00);
    }
    sendColor(red, green, blue);
    for (byte i = 0; i < 4; i++) {
        sendByte(0x00);
    }
}

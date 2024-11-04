#ifndef __ALED_h__
#define __ALED_h__

#include "Arduino.h"

#define _CLK_PULSE_DELAY 20

class SingleLED {
public:
    SingleLED(byte clk_pin, byte data_pin);
    void setColorRGB(byte red, byte green, byte blue);

private:
    byte _clk_pin;
    byte _data_pin;
    byte _current_red;
    byte _current_green;
    byte _current_blue;
    
    void clk(void);
    void sendByte(byte b);
    void sendColor(byte red, byte green, byte blue);
};

#endif
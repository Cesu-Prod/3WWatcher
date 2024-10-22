
/* 
 * Example of using the ChainableRGB library for controlling a Grove RGB.
 * This code cycles through all the colors in an uniform way. This is accomplished using a HSB color space. 
 * comments add By: http://www.seeedstudio.com/
 * Suiable for Grove - Chainable LED 
 */


#include "ChainableLED.h"
//Defines the num of LEDs used, The undefined 
//will be lost control.

extern "C" void __attribute__((weak)) yield(void) {}

ChainableLED leds(0, 7, 8);//defines the pin used on arduino.

void setup()
{
  //nothing
}

float hue = 0.0;
boolean up = true;

void loop()
{

  leds.setColorRGB(0, 255, 50, 0);
    
  delay(100);
    
  leds.setColorRGB(0, 0, 0, 0);

  delay(100);
}


#include <Wire.h>
#include <I2C_RTC.h>

static DS1307 RTC;

int hours,minutes,seconds,day,week,month,year;

void setup()
{
    Serial.begin(9600);
     while (!Serial) {
    ;
  }
    
    if (RTC.isRunning())
    {
        Serial.println("Yes");
        switch (RTC.getWeek())
        {
            case 1: week = "SUN";
            break;
            case 2: week = "MON";
            break;
            case 3: week = "TUE";
            break;
            case 4: week = "WED";
            break;
            case 5: week = "THU";
            break;
            case 6: week = "FRI";
            break;
            case 7: week = "SAT";
            break;
        }
		
		day=RTC.getDay();
		month=RTC.getMonth();
		year=RTC.getYear();
		
		hours = RTC.getHours();
		minutes = RTC.getMinutes(); 
		seconds = RTC.getSeconds();
    }
}

void loop()
{
}
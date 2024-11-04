#include "Wire.h"
#include "RTCLib.h"
RTC_DS1307 RTC;

extern "C" void __attribute__((weak)) yield(void) {}

void setup () {
Serial.begin(9600);
Wire.begin();
RTC.begin();
//Réglage de l'heure
RTC.adjust(DateTime(2023,10,22,21,50,00));
}
void loop () {
//Affiche la date et l'heure en suivant la norme ISO 8601
DateTime now = RTC.now();
Serial.print(now.year(), DEC);
Serial.print('-');
Serial.print(now.month(), DEC);
Serial.print('-');
Serial.print(now.day(), DEC);
Serial.print(' ');
Serial.print(now.hour(), DEC);
Serial.print(':');
Serial.print(now.minute(), DEC);
Serial.print(':');
Serial.print(now.second(), DEC);
Serial.println();
//Delais d'une seconde entre chaque incrémentation
delay(1000);
}

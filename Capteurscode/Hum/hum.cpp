#include <Adafruit_BME280.h>

Adafruit_BME280 bme;

act_mes = 3;

unsigned short int hum = bme.readHumidity();
if (hum < ssr_hum.min && hum > ssr_hum.max) {
    ssr_hum.Update(hum);
} else {
    ssr_hum.Update(NULL);
}
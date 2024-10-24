#include <Adafruit_BME280.h>
Adafruit_BME280 bme;

mesure_actuelle = 3

int humiditee = bme.readHumidity();
if (humiditee < capteur_hum.min && humiditee > capteur_hum.max) {
    capteur_hum.Mettre_à_jour(humiditee)
    } else {
    capteur_hum.Mettre_à_jour(NULL);
}
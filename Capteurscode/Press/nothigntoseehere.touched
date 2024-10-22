mesure_actuelle = 4

bme.setSampling(Adafruit_BME280::MODE_FORCED);
int pression = bme.readPressure() / 100; //Pour avoir la pression en Hpa
if (pression < capteur_press.min && pression > capteur_press.max) {
    capteur_press.Mettre_à_jour(pression)
    } else {
    errorcode = 4
}
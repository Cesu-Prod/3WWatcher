mesure_actuelle = 5

bme.setSampling(Adafruit_BME280::MODE_FORCED);
int temperature = bme.readTemperature();
if (temperature < capteur_temp.min && temperature > capteur_temp.max) {
    capteur_temp.Mettre_à_jour(temperature)
    } else {
    errorcode = 4
}
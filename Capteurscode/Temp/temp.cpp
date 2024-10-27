#include <Adafruit_BME280.h>

Adafruit_BME280 bme;

bme.setSampling(Adafruit_BME280::MODE_FORCED);
short int tmp = 0;
tmp = bme.readTemperature();
// Vérification si la température est dans la plage autorisée
if (tmp < param[7].min && tmp > param[7].max) {
    // Mise à jour de la valeur de température
    ssr_tmp.Update(tmp);
} else {
    // Erreur si la température est hors plage
    err_code = 4;
}
// Vérification si le capteur de température a été mis à jour dans les 30 secondes
if (crnt_time - tmp_time >= 30000 && tmp == 0) {
    if (ssr_tmp.error == 0) {
        ssr_tmp.error = 1; // 30 secondes sans mise à jour
    } else {
        err_code = 3;
        ssr_tmp.error = 0;
    }
}
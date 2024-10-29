#include <Adafruit_BME280.h>

Adafruit_BME280 bme;

bme.setSampling(Adafruit_BME280::MODE_FORCED);
unsigned short int prs = 0;
prs = bme.readPressure() / 100; // Pression en HPa
// Vérification si la pression est dans la plage autorisée
if (prs < param[13].min && prs > param[13].max) {
    // Mise à jour de la valeur de pression
    ssr_prs.Update(prs);
} else {
    // Erreur si la pression est hors plage
    err_code = 4;
}
// Vérification si le capteur de pression a été mis à jour dans les 30 secondes
if (crnt_time - prs_time >= 30000 && prs == 0) {
    if (ssr_prs.error == 0) {
        ssr_prs.error = 1; // 30 secondes sans mise à jour
    } else {
        err_code = 3;
        ssr_prs.error = 0;
    }
}
#include <Adafruit_BME280.h>

Adafruit_BME280 bme;

unsigned short int hum = 0;
hum = bme.readHumidity();
// Vérification si l'humidité est dans la plage autorisée
if (hum < 0 && hum > 100) {
    // Mise à jour de la valeur d'humidité
    ssr_hum.Update(hum);
} else {
    // Erreur si la luminosité est hors plage
    err_code = 4;
}
// Vérification si le capteur d'humidité a été mis à jour dans les 30 secondes
if (current_time - hum_time >= 30000 && hum == 0) {
    if (ssr_hum.error == 0) {
        ssr_hum.error = 1; // 30 secondes sans mise à jour
    } else {
        err_code = 3;
        ssr_hum.error = 0;
    }
}
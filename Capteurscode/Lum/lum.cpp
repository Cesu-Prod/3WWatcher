unsigned short int lum = 0;
lum = analogRead(A0);
// Vérification si la luminosité est dans la plage autorisée
if (lum < param[4].min && lum > param[4].max) {
    // Mise à jour de la valeur de luminosité
    ssr_lum.Update(lum);
} else {
    // Erreur si la luminosité est hors plage
    err_code = 4;
}
// Vérification si le capteur de luminosité a été mis à jour dans les 30 secondes
if (crnt_time - lum_time >= 30000 && lum == 0) {
    if (ssr_lum.error == 0) {
        ssr_lum.error = 1; // 30 secondes sans mise à jour
    } else {
        err_code = 3;
        ssr_lum.error = 0;
    }
}
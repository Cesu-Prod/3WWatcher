///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
////////////////////////Code principal/////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

// Variables pour gérer l'état des boutons et le timing
volatile unsigned long bouton_rouge_start = 0; // Temps de début pour le bouton rouge
volatile unsigned long bouton_vert_start = 0;  // Temps de début pour le bouton vert
volatile bool config = false; // État de la configuration

void loop() {

    if (digitalRead(pin_bouton_rouge) == LOW) { // Si le bouton rouge est appuyé
        configuration();                        // Mode configuration
    }
    standard(); // Mode standard


    unsigned long temps_actuel = millis();

    if (config) {
        // POUIC
    } else {

        // Gestion du bouton rouge
        if (digitalRead(pin_bouton_rouge) == LOW && bouton_rouge_start == 0) {
            bouton_rouge_start = temps_actuel; // Enregistrer le temps de début
        } else if (digitalRead(pin_bouton_rouge) == HIGH) {
            if (temps_actuel - bouton_rouge_start > 5000) { // Si plus de 5000 ms passées
                bouton_rouge_start = 0;
                ToggleMode(false); // Appel de ToggleMode
            }
        }

        // Gestion du bouton vert
        if (digitalRead(pin_bouton_vert) == LOW && bouton_vert_start == 0) {
            bouton_vert_start = temps_actuel; // Enregistrer le temps de début
        } else if (digitalRead(pin_bouton_vert) == HIGH) {
            if (temps_actuel - bouton_vert_start > 5000) { // Si plus de 5000 ms passées
                bouton_vert_start = 0;
                ToggleMode(true); // Appel de ToggleMode
            }
        }
    }
}
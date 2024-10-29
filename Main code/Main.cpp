  ////////////////////
 // Code principal //
////////////////////

// Variables pour gérer l'état des boutons et le timing
volatile unsigned long red_btn_time; // Temps de début pour le bouton rouge
volatile unsigned long grn_btn_time; // Temps de début pour le bouton vert
volatile bool config_mode = false; // État de la configuration

void red_btn_fall() {
    // Gestion du bouton rouge
    red_btn_time = crnt_time; // Enregistrer le temps de début
}

void red_btn_rise() {
    // Gestion du bouton rouge
    if (crnt_time - red_btn_time > 5000) { // Si plus de 5000 ms passées
        ToggleMode(false); // Appel de ToggleMode
    }
}

void grn_btn_fall() {
    // Gestion du bouton vert
    grn_btn_time = crnt_time; // Enregistrer le temps de début
}

void grn_btn_rise() {
    // Gestion du bouton vert
    if (crnt_time - grn_btn_time > 5000) { // Si plus de 5000 ms passées
        ToggleMode(true); // Appel de ToggleMode
    }
}

void setup() {
    attachInterrupt(digitalPinToInterrupt(red_btn_pin), red_btn_fall, FALLING);
    attachInterrupt(digitalPinToInterrupt(red_btn_pin), red_btn_rise, RISING);
    attachInterrupt(digitalPinToInterrupt(grn_btn_pin), grn_btn_fall, FALLING);
    attachInterrupt(digitalPinToInterrupt(grn_btn_pin), grn_btn_rise, RISING);
}

void loop() {
    crnt_time = millis();

    if (digitalRead(red_btn_pin) == LOW) { // Si le bouton rouge est appuyé
        config_mode = true;
        Configuration(); // Mode configuration
    }

    Standard(); // Mode standard
}
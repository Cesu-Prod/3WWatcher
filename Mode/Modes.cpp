///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
////////////////////Modes de fonctionnement////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

void Configuration() {
    Serial.println("Configuration :");
    while (true) {
        if (Serial.available() > 0) {
            // Lecture de la commande en entrée série
            String fct = Serial.readStringUntil('\n');
            // Traitement de la commande
            Command_set(fct);
        } else {
            inact_time = crnt_time;
        }
        while (Serial.available() == 0) {
            if (crnt_time - inact_time > 1800000) {
                return;
            }
        }
    }
}

void Standard() {
    if (mode = false) {
        param[0].val = param[0].val / 2;
    }
    mode = true;
    Mesures(false);
    toggleLED();
    mesure_save();
    Serial.flush();
}

void Economique() {
    mode = false;
    param[0].val = param[0].val * 2;
    Mesures(true);
    toggleLED();
    mesure_save();
    Serial.flush();
}

void Maintenance() {
    setColorRGB(255, 20, 0);

    while (true) {
        Mesures(false);
        Send_Serial();
    }
    Serial.println("1");
}
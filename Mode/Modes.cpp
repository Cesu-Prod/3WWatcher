///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
////////////////////Modes de fonctionnement////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

void Configuration() {
    Serial.println("Configuration :");
    if (Serial.available() > 0) {
        // Lecture de la commande en entrée série
        String fct = Serial.readStringUntil('\n');
        // Traitement de la commande
        Command_set(fct);
    }
    interrupt 30 min;
}

void Standard() {
    if (mode = false) {
        param[0].val = param[0].val / 2;
    }
    mode = true;
    interrupt;
    Mesures(false);
    toggleLED();
    mesure_save();
    Serial.flush();
}


void Economique() {
    mode = false;
    param[0].val = param[0].val * 2;
    interrupt;
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
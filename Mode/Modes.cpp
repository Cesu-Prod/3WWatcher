///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
////////////////////Modes de fonctionnement////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////


void standard() {
    if (mode = false) {
    log_interval = log_interval / 2;
    }
    mode = true;
    toggleLED();
    mesure_save();
    Serial.flush();
}


void economique() {
    mode = false;
    log_interval = log_interval * 2;
    toggleLED();
    mesure_save();
    Serial.flush();
}


void maintenance() {
    leds.setColorRGB(i, 255, 20, 0);

    while (true) {
        mesures();
        Send_Serial();
    }
    Serial.println("1");
}
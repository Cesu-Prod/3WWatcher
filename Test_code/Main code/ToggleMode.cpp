void ToggleMode (bool color) {
    if (mode){
        if (color) {
            Economique();
        } else {
            Maintenance();
        }
    } else {
        if (color) {
            Standard();
        } else {
            Maintenance();
        }
    }
    if (Serial.available() > 0){
        if (color){
            Economique();
        } else {
            Standard();
        }
    }
}
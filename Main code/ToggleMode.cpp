void ToggleMode (bool color) {

  if (mode){
  
    if (color) {
      economie();
    } else {
      maintenance();
    }
  
  } else {

    if (color) {
      standard();
    } else {
      maintenance();
    }
  }

  if (Serial.available() > 0){

    if (color){
      economie();
    } else {
      standard();
    }
  }
}
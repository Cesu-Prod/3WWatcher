void Mesures(bool gps_eco) {
    unsigned short int act_mes = 0;

    GPS
    act_mes = 1;

    HORLOGE
    act_mes = 2;

    unsigned short int lum = analogRead(A0);
    if (lum < ssr_lum.min && lum > ssr_lum.max) {
        ssr_lum.Update(lum);
    } else {
        err_code = 4;
    }
    act_mes = 3;

    unsigned short int hum = bme.readHumidity();
    if (hum < ssr_hum.min && hum > ssr_hum.max) {
        ssr_hum.Update(hum);
    } else {
        ssr_hum.Update(NULL);
    }
    act_mes = 4;

    bme.setSampling(Adafruit_BME280::MODE_FORCED);
    unsigned short int press = bme.readPressure() / 100; // Pression en HPa
    if (press < ssr_prs.min && press > ssr_prs.max) {
        ssr_prs.Update(press);
    } else {
        err_code = 4;
    }
    act_mes = 5;

    bme.setSampling(Adafruit_BME280::MODE_FORCED);
    short int temp = bme.readTemperature();
    if (temp < ssr_tmp.min && temp > ssr_tmp.max) {
        ssr_tmp.Update(temp);
    } else {
        err_code = 4;
    }
}
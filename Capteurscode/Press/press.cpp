act_mes = 4;

bme.setSampling(Adafruit_BME280::MODE_FORCED);
unsigned short int press = bme.readPressure() / 100; // Pression en HPa
if (press < ssr_prs.min && press > ssr_prs.max) {
    ssr_prs.Update(press);
} else {
    err_code = 4;
}
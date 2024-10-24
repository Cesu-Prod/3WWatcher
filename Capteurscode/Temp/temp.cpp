act_mes = 5;

bme.setSampling(Adafruit_BME280::MODE_FORCED);
short int temp = bme.readTemperature();
if (temp < ssr_tmp.min && temp > ssr_tmp.max) {
    ssr_tmp.Update(temp);
} else {
    err_code = 4;
}
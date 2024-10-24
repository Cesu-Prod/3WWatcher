act_mes = 2;

int lum = analogRead(A0);
if (lum < ssr_lum.min && lum > ssr_lum.max) {
    ssr_lum.Update(lum);
} else {
    err_code = 4;
}
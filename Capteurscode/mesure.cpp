bool dataValid;
unsigned long int gps_time = 0;
unsigned long int rtc_time = 0;
unsigned long int lum_time = 0;
unsigned long int hum_time = 0;
unsigned long int prs_time = 0;
unsigned long int tmp_time = 0;
bool gps_mes = true;
float latitude = 0.0;
float longitude = 0.0;

void Mesures(bool gps_eco) {
    // Variable pour suivre l'état des mesures
    unsigned long current_time = millis(); // Obtenir le temps actuel
    dataValid = false;

    // GPS //
    if (gps_eco) {
        if (gps_mes) {
            // Appel de la fonction pour prendre la mesure GPS
            GPS_mes(current_time); 
        }
        // Inverser la prise de mesure pour la prochaine fois
        gps_mes = !gps_mes;
    } else {
        GPS_mes(current_time);
    }
    // Vérification si le GPS a été mis à jour dans les 30 secondes
    if (current_time - gps_time >= 30000 && !dataValid) {
        if (ssr_gps.error == 0) {
            ssr_gps.error = 1; // 30 secondes sans mise à jour GPS
            latitude = 0;
            longitude = 0;
        } else {
            err_code = 2;
            ssr_gps.error = 0;
        }
    }

    // HORLOGE //
    // Tableau des jours de la semaine stocké en mémoire programme
    const char week_days[] PROGMEM = "SUNMONTUEWEDTHUFRISAM";

    // Récupération de la date et de l'heure actuelle
    static DateTime now = rtc.now(); // Variable statique pour réduire la pile
    rtc_time = current_time; // Mettre à jour le temps de la dernière lecture RTC

    // Récupération des composantes de l'heure
    uint8_t hour = now.hour(); // Heure
    uint8_t minute = now.minute(); // Minute
    uint8_t second = now.second(); // Seconde

    // Récupération des composantes de la date
    uint8_t day = now.day(); // Jour du mois
    uint8_t month = now.month(); // Mois
    unsigned short int year = now.year(); // Année

    // Vérification des valeurs récupérées
    if (hour < 24 && minute < 60 && second < 60 && day > 0 && day <= 31 && month > 0 && month <= 12) {
        dataValid = true; // Les données sont valides
    } else {
        dataValid = false; // Les données ne sont pas valides
    }

    // Récupération du jour de la semaine
    uint8_t wd_index = now.dayOfTheWeek();
    char week_day;

    // Lecture du jour de la semaine
    for (uint8_t i = 0; i < 3; i++) {
        week_day = (char)pgm_read_byte(&week_days[wd_index + i]);
    }
    // Vérification si la RTC a été mise à jour dans les 30 secondes
    if (current_time - rtc_time >= 30000 && !dataValid) {
        if (ssr_rtc.error == 0) {
            ssr_rtc.error = 1; // 30 secondes sans mise à jour RTC
            hour = 0;
            minute = 0;
            second = 0;
            day = 0;
            month = 0;
            year = 0;
            week_day = '\0';
        } else {
            err_code = 1;
            ssr_rtc.error = 0;
        }
    }

    // LUMINOSITÉ //
    unsigned short int lum = 0;
    lum = analogRead(A0);
    // Vérification si la luminosité est dans la plage autorisée
    if (lum < param[4].min && lum > param[4].max) {
        // Mise à jour de la valeur de luminosité
        ssr_lum.Update(lum);
    } else {
        // Erreur si la luminosité est hors plage
        err_code = 4;
    }
    // Vérification si le capteur de luminosité a été mis à jour dans les 30 secondes
    if (current_time - lum_time >= 30000 && lum == 0) {
        if (ssr_lum.error == 0) {
            ssr_lum.error = 1; // 30 secondes sans mise à jour
        } else {
            err_code = 3;
            ssr_lum.error = 0;
        }
    }

    // HUMIDITÉ //
    unsigned short int hum = 0;
    hum = bme.readHumidity();
    // Vérification si l'humidité est dans la plage autorisée
    if (hum < 0 && hum > 100) {
        // Mise à jour de la valeur d'humidité
        ssr_hum.Update(hum);
    } else {
        // Erreur si la luminosité est hors plage
        err_code = 4;
    }
    // Vérification si le capteur d'humidité a été mis à jour dans les 30 secondes
    if (current_time - hum_time >= 30000 && hum == 0) {
        if (ssr_hum.error == 0) {
            ssr_hum.error = 1; // 30 secondes sans mise à jour
        } else {
            err_code = 3;
            ssr_hum.error = 0;
        }
    }
    
    // PRESSION //
    // Configuration du capteur de pression pour une lecture forcée
    bme.setSampling(Adafruit_BME280::MODE_FORCED);
    unsigned short int prs = 0;
    prs = bme.readPressure() / 100; // Pression en HPa
    // Vérification si la pression est dans la plage autorisée
    if (prs < param[13].min && prs > param[13].max) {
        // Mise à jour de la valeur de pression
        ssr_prs.Update(prs);
    } else {
        // Erreur si la pression est hors plage
        err_code = 4;
    }
    // Vérification si le capteur de pression a été mis à jour dans les 30 secondes
    if (current_time - prs_time >= 30000 && prs == 0) {
        if (ssr_prs.error == 0) {
            ssr_prs.error = 1; // 30 secondes sans mise à jour
        } else {
            err_code = 3;
            ssr_prs.error = 0;
        }
    }

    // TEMPÉRATURE //
    // Configuration du capteur de température pour une lecture forcée
    bme.setSampling(Adafruit_BME280::MODE_FORCED);
    short int tmp = 0;
    tmp = bme.readTemperature();
    // Vérification si la température est dans la plage autorisée
    if (tmp < param[7].min && tmp > param[7].max) {
        // Mise à jour de la valeur de température
        ssr_tmp.Update(tmp);
    } else {
        // Erreur si la température est hors plage
        err_code = 4;
    }
    // Vérification si le capteur de température a été mis à jour dans les 30 secondes
    if (current_time - tmp_time >= 30000 && tmp == 0) {
        if (ssr_tmp.error == 0) {
            ssr_tmp.error = 1; // 30 secondes sans mise à jour
        } else {
            err_code = 3;
            ssr_tmp.error = 0;
        }
    }
}
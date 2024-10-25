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
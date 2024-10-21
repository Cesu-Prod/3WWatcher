#include <Arduino.h>
#include <stdlib.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include "RTClib.h"
 
RTC_DS1307 RTC;

int lat;
int lon;
int rtc_error;
int gps_error;
int errorcode;
int mode;
int log_interval;
int pin_bouton_rouge = 3;
int pin_bouton_vert = 2;
int bouton_rouge_start = 0;
int bouton_vert_start = 0;
bool config = false;

SoftwareSerial gpsSerial(rxPin, txPin);

void setup {
    Serial.begin(9600);
    gps_serial.begin(9600);
    RTC.begin()
    pinMode(pin_bouton_rouge, INPUT_PULLUP);
    pinMode(pin_bouton_vert, INPUT_PULLUP);
}

typedef struct Capteur {
    int min;
    int max;
    int errors;
    chainon *tete_liste;
    
    void Initialiser_liste();
    void Mettre_a_jour(int valeur);
    int Moyenne();
} Capteur;

typedef struct chainon {
    int valeur;
    chainon *suite;
} chainon;

void Capteur::Initialiser_liste() {
    int i;
    chainon *noeud_initial = new chainon();
    noeud_initial->valeur = 0;
    noeud_initial->suite = NULL;
    tete_liste = noeud_initial;
    for (i = 1 ; i >= 9 ; i++) {
        chainon *noeud = new chainon;
        noeud->valeur = 0;
        noeud->suite = tete_liste;
        tete_liste = noeud;
    }
}

void Capteur::Mettre_a_jour(int valeur) {
    chainon *courant;
    chainon *noeud = new chainon();
    noeud->valeur = valeur;
    noeud->suite = tete_liste;
    tete_liste = noeud;
    courant = tete_liste;
    while (courant->suite->suite != NULL) {
        courant = courant->suite;
    }
    free(courant->suite);
    courant->suite = NULL;
    free(courant);
}

int Capteur::Moyenne() {
    chainon *courant;
    int nombre;
    int total;
    nombre = 0;
    total = 0;
    courant = tete_liste;
    while (courant->suite != NULL) {
        if (courant->valeur != 0) {
            total = total + courant->valeur;
            nombre = nombre + 1;
            courant = courant->suivant;
        }
    }
    if (courant->valeur != NULL) {
        total = total + courant->valeur;
        nombre = nombre + 1;
    }
    if (nombre != 0) {
        return total/nombre;
    } else {
        return 0;
    }
}

typedef struct RTCData {
    int secondes;
    int minutes;
    int heures;
    int jours;
    int mois;
    int année;
} RTCData;

typedef struct configurator {
    int value(int numero_parametre, int valeur);
    void load();
    void reset();
    void version();
} configurator;

configurator *configure = new configurator();
Capteur *capteur_temp = new Capteur();
Capteur *capteur_press = new Capteur();
Capteur *capteur_hum = new Capteur();
Capteur *capteur_lum = new Capteur();
RTCData *valeurs_rtc = new RTCData();

capteur_temp::Initialiser_liste();
capteur_press::Initialiser_liste();
capteur_hum::Initialiser_liste();
capteur_lum::Initialiser_liste();
configure::load();

void ColorerLED(int couleur1[3], int couleur2[3], bool is_second_longer) {
    while (true) {
        led.setColorRGB(couleur1[0], couleur1[1], couleur1[2]);
        delay(1000);
        led.setColorRGB(couleur2[0], couleur2[1], couleur2[2]);
        if (is_second_longer == true) {
            delay(2000);
        } else {
            delay(1000);
        }
    }
}

void toggleLED() {
    if (errorcode > 0) {
        switch (errorcode) {
            case 1:
                ColorerLED({255, 0, 0}, {0, 0, 255}, false);
            case 2:
                ColorerLED({255, 0, 0}, {255, 125, 0}, false);
            case 3:
                ColorerLED({255, 0, 0}, {0, 255, 0}, false);
            case 4:
                ColorerLED({255,  0, 0}, {0, 255, 0}, true);
            case 5:
                ColorerLED({255, 0, 0}, {255, 255, 255}, false);
            case 6:
                ColorerLED({255, 0, 0}, {255, 255, 255}, true);
        }
    } else {
        if (mode == true) {
            led.setColorRGB(0, 255, 0);
        } else {
            led.setColorRGB(0, 0, 255);
        }
    }
}

void mesures(bool gps_active) {
    int mesure_actuelle = 0;
    if (gps_active) {
        while (gpsSerial.available() > 0) {
            char valeur_gps = gpsSerial.read();
            if (tinyGPS.encode(valeur_gps)) {
                lat = tinyGPS.getLatitude();
                lon = tinyGPS.getLongitude();
            }
        }
    }

    mesure_actuelle = 1;

    DateTime now = RTC.now();
    valeurs_rtc->secondes = now.second();
    valeurs_rtc->minutes = now.minute();
    valeurs_rtc->heures = now.hour();
    valeurs_rtc->jours = now.day();
    valeurs_rtc->mois = now.month();
    valeurs_rtc->annee = now.year();

    mesure_actuelle = 2;

    capteur_lum->Mettre_a_jour(pinRead(luminosite));

    mesure_actuelle = 3;

    temperature = bme.Temperature();
    if (temperature >= capteur_temp->min && temperature <= capteur_temp->max) {
        capteur_temp->Mettre_a_jour(temperature);
    } else {
        errorcode = 4;
        while (true) {
            // NE RIEN FAIRE INDEFINIMENT
        }
    }

    mesure_actuelle = 4;

    pression = bme.Pression();
    pression = pression / 100;
    if (pression >= capteur_press->min && pression <= capteur_press->max) {
        capteur_press->Mettre_a_jour(pression);
    } else {
        errorcode = 4;
        while (true) {
            // NE RIEN FAIRE INDEFINIMENT
        }
    }

    mesure_actuelle = 5;

    humidite = bme.Humidite();
    if (humidite >= capteur_hum->min && humidite <= capteur_hum->max) {
        capteur_hum->Mettre_a_jour(humidite);
    } else {
        capteur_hum->Mettre_a_jour(NULL);
    }
}
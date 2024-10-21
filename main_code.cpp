#include <Arduino.h>
#include <stdlib.h>

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

void setup {
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
    int value(numero_parametre, valeur);
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

void ColorerLED(couleur1, couleur2, is_second_longer) {
    while (true) {
        
        delay(1000)
    }
}
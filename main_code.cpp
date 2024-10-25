Finish : Capteurscode, LED, Mode, RTC,
Reste : Capteurscode/BEEMO, Main code, SD, Test_2, WIRE

  //////////////
 // Includes //
//////////////

#include "Arduino.h"
#include "RTClib.h"
#include <Adafruit_BME280.h>
#include <EEPROM.h>
#include <Wire.h>

extern "C" void __attribute__((weak)) yield(void) {}

  //////////////////////////////////////
 // Variables globales et constantes //
//////////////////////////////////////

Adafruit_BME280 bme;
RTC_DS1307 rtc;

// Constants
#define _CLK_PULSE_DELAY 20

uint8_t err_code;
bool mode;
const uint8_t param_num = 15;

// MESURES //
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

// LED //
byte _clk_pin;
byte _data_pin;
byte _current_red;
byte _current_green;
byte _current_blue;

// CONFIGURATION //
uint8_t vers = 0.8;
uint8_t lot_num = 2;

volatile unsigned long bouton_rouge_start = 0; // Temps de début pour le bouton rouge
volatile unsigned long bouton_vert_start = 0;  // Temps de début pour le bouton vert
volatile bool config = false; // État de la configuration

  ////////////////
 // Structures //
////////////////

typedef struct Node {
    short int value;
    Node *next;
} Node;

typedef struct Sensor {
    uint8_t error;
    Node *head_list;

    Sensor () {
        Init_list();
    }
    
    void Init_list();
    void Update(short int value);
    short int Average();
} Sensor;

void Sensor::Init_list() {
    Node *initial_node = new Node();
    Node *node = new Node;
    initial_node->value = 0;
    initial_node->next = nullptr;
    head_list = initial_node;
    for (uint8_t i = 1; i <= 3; i++) {
        node->value = 0;
        node->next = head_list;
        head_list = node;
    }
}

void Sensor::Update(short int value) {
    Node *current;
    Node *node = new Node();
    node->value = value;
    node->next = head_list;
    head_list = node;
    current = head_list;
    while (current->next->next != nullptr) {
        current = current->next;
    }
    delete current->next;
    current->next = nullptr;
}

short int Sensor::Average() {
    Node *current;
    uint8_t count = 0;
    short int total = 0;
    current = head_list;
    while (current != nullptr) {
        if (current->value != 0) {
            total += current->value;
            count++;
        }
        current = current->next;
    }
    if (count != 0) {
        return total / count;
    } else {
        return 0;
    }
}

// Définition de la structure Param
typedef struct Param {
    uint8_t addr;      // Adresse de stockage
    short int def_val;            // Valeur par défaut
    short int min;                // Valeur minimale
    unsigned short int max;       // Valeur maximale
    short int val;                // Valeur actuelle
} Param;

// Définir les paramètres
Param params[] = {
    {18, 600, 0, 0, 600}, // LOG_INTERVAL ; 0
    {20, 2048, 0, 4096, 2048}, // FILE_MAX_SIZE ; 1
    {22, 30, 0, 0, 30}, // TIMEOUT ; 2
    {24, 1, 0, 1, 1}, // LUMIN ; 3
    {26, 255, 0, 1023, 255}, // LUMIN_LOW ; 4
    {28, 768, 0, 1023, 768}, // LUMIN_HIGH ; 5
    {30, 1, 0, 1, 1}, // TEMP_AIR ; 6
    {32, -10, -40, 85, -10}, // MIN_TEMP_AIR ; 7
    {34, 60, -40, 85, 60}, // MAX_TEMP_AIR ; 8
    {36, 1, 0, 1, 1}, // HYGR ; 9
    {38, 0, -40, 85, 0}, // HYGR_MINT ; 10
    {40, 50, -40, 85, 50}, // HYGR_MAXT ; 11
    {42, 1, 0, 1, 1}, // PRESSURE ; 12
    {44, 850, 300, 1100, 850}, // PRESSURE_MIN ; 13
    {46, 1080, 300, 1100, 1080}, // PRESSURE_MAX ; 14
};
Param *param = params;

// Définition de la structure Setting
typedef struct Setting {

    // Constructeur de la structure Setting
    Setting() {
        // Chargement des paramètres par défaut
        load();
    }

    // Fonction pour charger les paramètres
    void load();
    // Fonction pour définir une valeur
    void value(uint8_t num, short int valeur);
    // Fonction pour définir l'heure
    void clock(uint8_t hour, uint8_t min, uint8_t sec);
    // Fonction pour définir la date
    void date(uint8_t month, uint8_t day, unsigned short int year);
    // Fonction pour définir le jour
    void day(String week_day);
    // Fonction pour réinitialiser les paramètres
    void reset();
    // Fonction pour afficher la version
    void version();
} Setting;

void Setting::load() {
    // Vérifie si aucune configuration n'a été chargée
    if (EEPROM.read(2) == 0) {
        // Chargement des paramètres par défaut
        for (uint8_t i = 0; i < param_num; i++) {
            // Enregistrement de la version et du numéro de lot dans l'EEPROM
            EEPROM.put(0, vers);
            EEPROM.put(2, lot_num);
            // Écriture de la valeur par défaut dans l'EEPROM
            EEPROM.put(param[i].addr, param[i].def_val);
            // Mise à jour de la valeur actuelle du paramètre
            param[i].val = param[i].def_val;
        }
        // Indication que la configuration a été chargée
        EEPROM.write(2, 1);
    } else {
        // Chargement des paramètres depuis l'EEPROM
        for (uint8_t i = 0; i < param_num; i++) {
            // Lecture de la valeur actuelle du paramètre depuis l'EEPROM
            EEPROM.get(param[i].addr, param[i].val);
        }
    }
}

void Setting::value(uint8_t num, short int val_par) {
    // Vérifie si le numéro de paramètre est valide
    if (num >= 0 && num < param_num) {
        // Vérifie si la valeur est dans la plage autorisée ou si c'est un paramètre spécial
        if (val_par >= param[num].min && val_par <= param[num].max || num == 1 || num == 3) {
            // Met à jour la valeur du paramètre
            param[num].val = val_par;
            // Enregistre la valeur dans l'EEPROM
            EEPROM.put(param[num].addr, val_par);
            // Affiche un message de confirmation
            Serial.print("Parametre ");
            Serial.print(num);
            Serial.print(" mis a jour a ");
            Serial.println(val_par);
        } else {
            // Affiche un message d'erreur si la valeur est hors limites
            Serial.print("Valeur hors limites pour le parametre ");
            Serial.println(num);
        }
    } else {
        // Affiche un message d'erreur si le numéro de paramètre est invalide
        Serial.println("Numero de parametre invalide");
    }
}

void Setting::clock(uint8_t hour, uint8_t min, uint8_t sec) {
    // Vérifie si l'heure, la minute et la seconde sont dans la plage autorisée
    if (hour >= 0 && hour < 24 && min >= 0 && min < 60 && sec >= 0 && sec < 60) {
        // Enregistre l'heure, la minute et la seconde dans l'EEPROM
        EEPROM.put(4, hour);
        EEPROM.put(6, min);
        EEPROM.put(8, sec);
        // Affiche un message de confirmation
        Serial.print("Heure mise a jour a ");
        Serial.print(hour);
        Serial.print(":");
        Serial.print(min);
        Serial.print(":");
        Serial.println(sec);
    } else {
        // Affiche un message d'erreur si l'heure, la minute ou la seconde est invalide
        Serial.println("Valeurs d'heure, de minute ou de seconde invalides");
    }
}

void Setting::date(uint8_t month, uint8_t day, unsigned short int year) {
    // Vérifie si le mois, le jour et l'année sont dans la plage autorisée
    uint8_t max_days;
    if (month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) {
        max_days = 31;
    } else if (month == 4 || month == 6 || month == 9 || month == 11) {
        max_days = 30;
    } else {
        if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) {
            max_days = 29;
        } else {
            max_days = 28;
        }
    }
    if (month >= 1 && month <= 12 && day >= 1 && day <= max_days && year >= 2000) {
        // Enregistre le mois, le jour et l'année dans l'EEPROM
        EEPROM.put(10, month);
        EEPROM.put(12, day);
        EEPROM.put(14, year);
        // Affiche un message de confirmation
        Serial.print("Date mise a jour a ");
        Serial.print(month);
        Serial.print("/");
        Serial.print(day);
        Serial.print("/");
        Serial.println(year);
    } else {
        // Affiche un message d'erreur si le mois, le jour ou l'année est invalide
        Serial.println("Valeurs de mois, de jour ou d'annee invalides.");
    }
}

void Setting::day(String week_day) {
    // Vérifie si le jour de la semaine est valide
    if (week_day == "MON" || week_day == "TUE" || week_day == "WED" || week_day == "THU" || week_day == "FRI" || week_day == "SAT" || week_day == "SUN") {
        // Enregistre le jour de la semaine dans l'EEPROM
        EEPROM.put(16, week_day);
        // Affiche un message de confirmation
        Serial.print("Jour de la semaine mis a jour a ");
        Serial.println(week_day);
    } else {
        // Affiche un message d'erreur si le jour de la semaine est invalide
        Serial.println("Valeur de jour de la semaine invalide.");
    }
}

void Setting::reset() {
    // Réinitialise les paramètres à leurs valeurs par défaut
    for (uint8_t i = 0; i < param_num; i++) {
        // Écriture de la valeur par défaut dans l'EEPROM
        EEPROM.put(param[i].addr, param[i].def_val);
        // Mise à jour de la valeur actuelle du paramètre
        param[i].val = param[i].def_val;
    }
}

void Setting::version() {
    // Lire la version et le numéro de lot depuis l'EEPROM
    EEPROM.get(0, vers);
    EEPROM.get(2, lot_num);

    // Affichage de la version et du numéro de lot
    Serial.print("V : ");
    Serial.print(vers);
    Serial.print("\nN : ");
    Serial.println(lot_num);
}

// Création d'une instance de la classe Setting
Setting *configure = new Setting();

static Sensor ssr_gps;
static Sensor ssr_rtc;
static Sensor ssr_lum;
static Sensor ssr_hum;
static Sensor ssr_tmp;
static Sensor ssr_prs;

  ///////////////
 // Fonctions //
///////////////

// GPS //
bool GPS_mes(unsigned long current_time) {
    // Buffer pour stocker la trame NMEA
    static char buffer[100];
    static uint8_t position = 0;

    while (Serial.available()) {
        char c = Serial.read();
        
        // Si on détecte une nouvelle ligne
        if (c == '\n') {
            buffer[position] = '\0'; // Termine la chaîne
            
            // Vérifie si c'est une trame GNGGA
            if (strstr(buffer, "$GNGGA") != NULL) {
                // Parse la trame pour extraire latitude et longitude
                parseGGA(buffer, latitude, longitude);
                gps_time = current_time; // Mettre à jour le temps de la dernière lecture GPS
            }
            
            position = 0; // Reset la position pour la prochaine trame
        }
        // Si on n'a pas encore atteint la fin du buffer
        else if (position < 99) {
            buffer[position++] = c;
        }
        
        if (dataValid) {
            turnOffGPS();
            break;
        }
    }
    return dataValid;
}

float convertNMEAToDecimal(float val) {
    int degrees = (int)(val / 100);
    float minutes = val - (degrees * 100);
    return degrees + (minutes / 60.0);
}

bool parseGGA(char* trame, float &latitude, float &longitude) {
    char* ptr = strtok(trame, ",");
    uint8_t index = 0;
    char lat_dir, lon_dir;
    
    while (ptr != NULL) {
        switch(index) {
            case 2: // Latitude
                latitude = convertNMEAToDecimal(atof(ptr));
                break;
            case 3: // Direction N/S
                lat_dir = ptr[0];
                break;
            case 4: // Longitude
                longitude = convertNMEAToDecimal(atof(ptr));
                break;
            case 5: // Direction E/W
                lon_dir = ptr[0];
                break;
        }
        ptr = strtok(NULL, ",");
        index++;
    }
    // Vérification des directions et des coordonnées
    if ((lat_dir == 'N' || lat_dir == 'S') && (lon_dir == 'E' || lon_dir == 'W') && latitude != 0.0 && longitude != 0.0) {
        // Applique la direction à la latitude et à la longitude
        if (lon_dir == 'W') longitude = -longitude;
        if (lat_dir == 'S') latitude = -latitude;
        dataValid = true;
    }
    return dataValid;
}

void turnOffGPS() {
    // Envoie la commande pour éteindre le GPS
    Serial.println("$PMTK161,0*28");
}

// MESURES //
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

// LED //
void clk(void) {
    digitalWrite(_clk_pin, LOW);
    delayMicroseconds(_CLK_PULSE_DELAY);
    digitalWrite(_clk_pin, HIGH);
    delayMicroseconds(_CLK_PULSE_DELAY);
}

void sendByte(byte b) {
    for (byte i = 0; i < 8; i++) {
        digitalWrite(_data_pin, (b & 0x80) ? HIGH : LOW);
        clk();
        b <<= 1;
    }
}

void sendColor(byte red, byte green, byte blue) {
    byte prefix = B11000000;
    if ((blue & 0x80) == 0) prefix |= B00100000;
    if ((blue & 0x40) == 0) prefix |= B00010000;
    if ((green & 0x80) == 0) prefix |= B00001000;
    if ((green & 0x40) == 0) prefix |= B00000100;
    if ((red & 0x80) == 0) prefix |= B00000010;
    if ((red & 0x40) == 0) prefix |= B00000001;
    
    sendByte(prefix);
    sendByte(blue);
    sendByte(green);
    sendByte(red);
}

void Init_LED(byte clk_pin, byte data_pin) {
    _clk_pin = clk_pin;
    _data_pin = data_pin;
    _current_red = 0;
    _current_green = 0;
    _current_blue = 0;
    
    pinMode(_clk_pin, OUTPUT);
    pinMode(_data_pin, OUTPUT);
    setColorRGB(0, 0, 0);
}

void setColorRGB(byte red, byte green, byte blue) {
    _current_red = red;
    _current_green = green;
    _current_blue = blue;
    
    // Send data frame prefix
    for (byte i = 0; i < 4; i++) {
        sendByte(0x00);
    }
    
    sendColor(red, green, blue);
}

void ColorerLED(uint8_t couleur1[3], uint8_t couleur2[3], bool is_second_longer) {
    setColorRGB(couleur1[0], couleur1[1], couleur1[2]);
    delay(1000);
    setColorRGB(couleur2[0], couleur2[1], couleur2[2]);
    if (is_second_longer) {
        delay(2000);
    } else {
        delay(1000);
    }
}

void toggleLED() {
    if (err_code > 0) {
        byte color1[3] = {255, 0, 0};
        byte color2[3];
        bool is_second_longer;

        switch (err_code) {
            case 1: // RTC error
                color2[0] = 0;
                color2[1] = 0;
                color2[2] = 255;
                is_second_longer = false;
                break;
            case 2: // GPS error
                color2[0] = 255;
                color2[1] = 125;
                color2[2] = 0;
                is_second_longer = false;
                break;
            case 3: // Sensor error
                color2[0] = 0;
                color2[1] = 255;
                color2[2] = 0;
                is_second_longer = false;
                break;
            case 4: // done ki pa konsistan
                color2[0] = 0;
                color2[1] = 255;
                color2[2] = 0;
                is_second_longer = true;
                break;
            case 5: // SD full error
                color2[0] = 255;
                color2[1] = 255;
                color2[2] = 255;
                is_second_longer = false;
                break;
            case 6: // SD access error
                color2[0] = 255;
                color2[1] = 255;
                color2[2] = 255;
                is_second_longer = true;
                break;
        }
        ColorerLED(color1, color2, is_second_longer);
        while (true) {}
    } else {
        if (mode) {
            setColorRGB(0, 255, 0);
        } else {
            setColorRGB(0, 0, 255);
        }
    }
}

// MODES //
void Command_set(String fct) {
    // Vérifie si la commande est pour définir une valeur
    if (fct.startsWith("set ")) {
        // Commande pour définir une valeur
        uint8_t num_par = fct.substring(4, fct.indexOf(' ', 4)).toInt();
        short int val_par = fct.substring(fct.indexOf(' ', 4) + 1).toInt();
        configure->value(num_par, val_par);
    } 
    // Vérifie si la commande est pour définir l'heure
    else if (fct.startsWith("clock ")) {
        // Commande pour définir l'heure
        uint8_t hour = fct.substring(6, fct.indexOf(':')).toInt(); 
        uint8_t min = fct.substring(fct.indexOf(':') + 1, fct.indexOf(':', fct.indexOf(':') + 1)).toInt(); 
        uint8_t sec = fct.substring(fct.lastIndexOf(':') + 1).toInt(); 
        configure->clock(hour, min, sec);
    } 
    // Vérifie si la commande est pour définir la date
    else if (fct.startsWith("date ")) {
        // Commande pour définir la date
        uint8_t month = fct.substring(5, fct.indexOf('/')).toInt(); 
        uint8_t day = fct.substring(fct.indexOf('/') + 1, fct.lastIndexOf('/')).toInt(); 
        unsigned short int year = fct.substring(fct.lastIndexOf('/') + 1).toInt(); 
        configure->date(month, day, year);
    } 
    // Vérifie si la commande est pour définir le jour
    else if (fct.startsWith("day ")) {
        // Commande pour définir le jour
        String week_day = fct.substring(4, fct.indexOf(' ', 4));
        configure->day(week_day);
    } 
    // Vérifie si la commande est pour réinitialiser les paramètres
    else if (fct.equals("reset")) {
        configure->reset();
        Serial.println("Reset ok");
    } 
    // Vérifie si la commande est pour afficher la version
    else if (fct.equals("version")) {
        configure->version();
    } 
    // Si aucune commande n'est reconnue
    else {
        Serial.println("Commande inconnue");
    }
}

void Configuration() {
    Serial.println("Configuration :");
    if (Serial.available() > 0) {
        // Lecture de la commande en entrée série
        String fct = Serial.readStringUntil('\n');
        // Traitement de la commande
        Command_set(fct);
    }
    interrupt 30 min
}

void Standard() {
    if (mode = false) {
        param[0].val = param[0].val / 2;
    }
    mode = true;
    interrupt
    Mesures(false);
    toggleLED();
    mesure_save(); manque + (moyenne)
    Serial.flush();
}

void Economique() {
    mode = false;
    param[0].val = param[0].val * 2;
    interrupt
    Mesures(true);
    toggleLED();
    mesure_save(); manque + (moyenne)
    Serial.flush();
}

void Maintenance() {
    setColorRGB(255, 20, 0);

    while (true) {
        Mesures(false);
        Send_Serial(); manque
    }
    Serial.println("1");
}

  ////////////////////
 // Initialisation //
////////////////////

void setup() {
    Serial.begin(9600);
    
    Init_LED(7, 8);
    
    if (!rtc.begin()) {
        Serial.println(F("RTC error")); // F() stocke la chaîne en mémoire flash
        while (1);
    }

    if (!bme.begin()) {
        Serial.println(F("BME error")); // F() stocke la chaîne en mémoire flash
        while (1);
    }
}

  ////////////////////
 // Code principal //
////////////////////

void loop() {
    
}
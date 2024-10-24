#include <Arduino.h>
#include <EEPROM.h>

// Nombre de paramètres
const int param_num = 15;

// Définition de la structure Param
typedef struct Param {
    int addr;      // Adresse de stockage
    int def_val;   // Valeur par défaut
    int min;       // Valeur minimale
    int max;       // Valeur maximale
    int val;       // Valeur actuelle
} Param;

// Définir les paramètres
Param params[] = {
    {10, 600, 0, 0, 600}, // LOG_INTERVALL
    {11, 2048, 0, 4096, 2048}, // FILE_MAX_SIZE
    {12, 30, 0, 0, 30}, // TIMEOUT
    {13, 1, 0, 1, 1}, // LUMIN
    {14, 255, 0, 1023, 255}, // LUMIN_LOW
    {15, 768, 0, 1023, 768}, // LUMIN_HIGH
    {16, 1, 0, 1, 1}, // TEMP_AIR
    {17, -10, -40, 85, -10}, // MIN_TEMP_AIR
    {18, 60, -40, 85, 60}, // MAX_TEMP_AIR
    {19, 1, 0, 1, 1}, // HYGR
    {20, 0, -40, 85, 0}, // HYGR_MINT
    {21, 50, -40, 85, 50}, // HYGR_MAXT
    {22, 1, 0, 1, 1}, // PRESSURE
    {23, 850, 300, 1100, 850}, // PRESSURE_MIN
    {24, 1080, 300, 1100, 1080}, // PRESSURE_MAX
};

// Définition de la structure Configurator
typedef struct Configurator {
    // Pointeur vers le tableau de paramètres
    Param *param = params;

    // Constructeur de la structure Configurator
    Configurator() {
        // Chargement des paramètres par défaut
        load();
    }

    // Fonction pour charger les paramètres
    void load();

    // Fonction pour définir une valeur
    void value(int num, int valeur);

    // Fonction pour définir l'heure
    void clock(int hour, int min, int sec);

    // Fonction pour définir la date
    void date(int month, int day, int year);

    // Fonction pour définir le jour
    void day(String week_day);

    // Fonction pour réinitialiser les paramètres
    void reset();

    // Fonction pour afficher la version
    void version();
} Configurator;

void Configurator::load() {
    // Vérifie si aucune configuration n'a été chargée
    if (EEPROM.read(2) == 0) {
        // Chargement des paramètres par défaut
        for (int i = 0; i < param_num; i++) {
            // Écriture de la valeur par défaut dans l'EEPROM
            EEPROM.put(param[i].addr, param[i].def_val);
            // Mise à jour de la valeur actuelle du paramètre
            param[i].val = param[i].def_val;
        }
        // Indication que la configuration a été chargée
        EEPROM.write(2, 1);
    } else {
        // Chargement des paramètres depuis l'EEPROM
        for (int i = 0; i < param_num; i++) {
            // Lecture de la valeur actuelle du paramètre depuis l'EEPROM
            EEPROM.get(param[i].addr, param[i].val);
        }
    }
}

void Configurator::value(int num, int val_par) {
    // Vérifie si le numéro de paramètre est valide
    if (num >= 0 && num < param_num) {
        // Vérifie si la valeur est dans la plage autorisée ou si c'est un paramètre spécial
        if (val_par >= param[num].min && val_par <= param[num].max || num == 1 || num == 3) {
            // Met à jour la valeur du paramètre
            param[num].val = val_par;
            // Enregistre la valeur dans l'EEPROM
            EEPROM.put(param[num].addr, val_par);
            // Affiche un message de confirmation
            Serial.print("Paramètre ");
            Serial.print(num);
            Serial.print(" mis à jour à ");
            Serial.println(val_par);
            // Affiche les valeurs de chaque paramètre
            for (int i = 0; i < sizeof(params) / sizeof(params[0]); i++) {
                Serial.print("ID: ");
                Serial.print(param[i].addr);
                Serial.print(", Value: ");
                Serial.print(param[i].def_val);
                Serial.print(", Min: ");
                Serial.print(param[i].min);
                Serial.print(", Max: ");
                Serial.print(param[i].max);
                Serial.print(", Default: ");
                Serial.println(param[i].val);
            }
        } else {
            // Affiche un message d'erreur si la valeur est hors limites
            Serial.print("Valeur hors limites pour le paramètre ");
            Serial.println(num);
        }
    } else {
        // Affiche un message d'erreur si le numéro de paramètre est invalide
        Serial.println("Numéro de paramètre invalide.");
    }
}

void Configurator::clock(int hour, int min, int sec) {
    // Vérifie si l'heure, la minute et la seconde sont dans la plage autorisée
    if (hour >= 0 && hour < 24 && min >= 0 && min < 60 && sec >= 0 && sec < 60) {
        // Enregistre l'heure, la minute et la seconde dans l'EEPROM
        EEPROM.put(3, hour);
        EEPROM.put(4, min);
        EEPROM.put(5, sec);
        // Affiche un message de confirmation
        Serial.print("Heure mise à jour à ");
        Serial.print(hour);
        Serial.print(":");
        Serial.print(min);
        Serial.print(":");
        Serial.println(sec);
    } else {
        // Affiche un message d'erreur si l'heure, la minute ou la seconde est invalide
        Serial.println("Valeurs d'heure, de minute ou de seconde invalides.");
    }
}

void Configurator::date(int month, int day, int year) {
    // Vérifie si le mois, le jour et l'année sont dans la plage autorisée
    int max_days;
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
        EEPROM.put(6, month);
        EEPROM.put(7, day);
        EEPROM.put(8, year);
        // Affiche un message de confirmation
        Serial.print("Date mise à jour à ");
        Serial.print(month);
        Serial.print("/");
        Serial.print(day);
        Serial.print("/");
        Serial.println(year);
    } else {
        // Affiche un message d'erreur si le mois, le jour ou l'année est invalide
        Serial.println("Valeurs de mois, de jour ou d'année invalides.");
    }
}

void Configurator::day(String week_day) {
    // Vérifie si le jour de la semaine est valide
    if (week_day == "MON" || week_day == "TUE" || week_day == "WED" || week_day == "THU" || week_day == "FRI" || week_day == "SAT" || week_day == "SUN") {
        // Enregistre le jour de la semaine dans l'EEPROM
        EEPROM.put(9, week_day);
        // Affiche un message de confirmation
        Serial.print("Jour de la semaine mis à jour à ");
        Serial.println(week_day);
    } else {
        // Affiche un message d'erreur si le jour de la semaine est invalide
        Serial.println("Valeur de jour de la semaine invalide.");
    }
}

void Configurator::reset() {
    // Réinitialise les paramètres à leurs valeurs par défaut
    for (int i = 0; i < param_num; i++) {
        // Écriture de la valeur par défaut dans l'EEPROM
        EEPROM.put(param[i].addr, param[i].def_val);
        // Mise à jour de la valeur actuelle du paramètre
        param[i].val = param[i].def_val;
    }
}

void Configurator::version() {
    // Fonction pour afficher la version et le numéro de lot
    int version;
    int lot_num;

    // Lire la version et le numéro de lot depuis l'EEPROM
    EEPROM.get(0, version);
    EEPROM.get(1, lot_num);

    // Affichage de la version et du numéro de lot
    Serial.print("V : ");
    Serial.print(version);
    Serial.print("\nN : ");
    Serial.println(lot_num);
}

// Création d'une instance de la classe Configurator
Configurator *configure = new Configurator();

// Fonction pour traiter une commande
void processCommand(String fct) {
    // Vérifie si la commande est pour définir une valeur
    if (fct.startsWith("set ")) {
        // Commande pour définir une valeur
        int num_par = fct.substring(4, fct.indexOf(' ', 4)).toInt();
        int val_par = fct.substring(fct.indexOf(' ', 4) + 1).toInt();
        configure->value(num_par, val_par);
    } 
    // Vérifie si la commande est pour définir l'heure
    else if (fct.startsWith("clock ")) {
        // Commande pour définir l'heure
        int hour = fct.substring(6, fct.indexOf(':')).toInt(); 
        int min = fct.substring(fct.indexOf(':') + 1, fct.indexOf(':', fct.indexOf(':') + 1)).toInt(); 
        int sec = fct.substring(fct.lastIndexOf(':') + 1).toInt(); 
        configure->clock(hour, min, sec);
    } 
    // Vérifie si la commande est pour définir la date
    else if (fct.startsWith("date ")) {
        // Commande pour définir la date
        int month = fct.substring(5, fct.indexOf('/')).toInt(); 
        int day = fct.substring(fct.indexOf('/') + 1, fct.lastIndexOf('/')).toInt(); 
        int year = fct.substring(fct.lastIndexOf('/') + 1).toInt(); 
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

void setup() {
    // Initialisation de la communication série
    Serial.begin(9600);
    // Affichage du message de configuration
    Serial.println("Configuration :");
}

void loop() {
    // Enregistrement des valeurs par défaut dans l'EEPROM
    EEPROM.put(0, 0.8);
    EEPROM.put(1, 2);
    // Vérification de la disponibilité de données en entrée série
    if (Serial.available() > 0) {
        // Lecture de la commande en entrée série
        String fct = Serial.readStringUntil('\n');
        // Traitement de la commande
        processCommand(fct);
    }
}
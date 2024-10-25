#include "Arduino.h"

extern "C" void __attribute__((weak)) yield(void) {}

float convertNMEAToDecimal(float val) {
    int degrees = (int)(val / 100);
    float minutes = val - (degrees * 100);
    return degrees + (minutes / 60.0);
}

bool parseGGA(char* trame, float &latitude, float &longitude) {
    char* ptr = strtok(trame, ",");
    unsigned short int index = 0;
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
                if (lon_dir == 'W') longitude = -longitude;
                if (lat_dir == 'S') latitude = -latitude;
                
                // Affiche les coordonnées si valides
                if (latitude != 0.0 && longitude != 0.0) {
                    return true;
                }
                break;
        }
        ptr = strtok(NULL, ",");
        index++;
    }
    return false;
}

bool printCurrentCoordinates(float &latitude, float &longitude) {
    // Buffer pour stocker la trame NMEA
    static char buffer[100];
    static unsigned short int position = 0;
    
    while (Serial.available()) {
        char c = Serial.read();
        
        // Si on détecte une nouvelle ligne
        if (c == '\n') {
            buffer[position] = '\0'; // Termine la chaîne
            
            // Vérifie si c'est une trame GNGGA
            if (strstr(buffer, "$GNGGA") != NULL) {
                // Parse la trame pour extraire latitude et longitude
                if (parseGGA(buffer, latitude, longitude)) {
                    return true;
                }
            }
            
            position = 0; // Reset la position pour la prochaine trame
        }
        // Si on n'a pas encore atteint la fin du buffer
        else if (position < 99) {
            buffer[position++] = c;
        }
    }
    return false;
}

void turnOffGPS() {
    // Envoie la commande pour éteindre le GPS
    Serial.println("$PMTK161,0*28");
}

void setup() {
    // Initialisation du port série pour le moniteur
    Serial.begin(9600);

    Serial.println("Démarrage du GPS...");

    float latitude = 0.0;
    float longitude = 0.0;
    unsigned long startTime = millis();

    // Attendre jusqu'à ce que des coordonnées valides soient reçues
    while (!printCurrentCoordinates(latitude, longitude)) {
        // Attendre un peu pour éviter de surcharger le port série
        delay(100);
    }

    // Afficher les coordonnées
    Serial.print("Latitude: ");
    Serial.print(latitude, 6);
    Serial.print("° ");
    Serial.print("Longitude: ");
    Serial.print(longitude, 6);
    Serial.println("°");

    // Afficher le temps écoulé depuis le démarrage
    unsigned long elapsedTime = millis() - startTime;
    Serial.print("Temps écoulé: ");
    Serial.print(elapsedTime / 1000.0);
    Serial.println(" secondes");

    // Éteindre le GPS
    turnOffGPS();
}

void loop() {
    // Rien à faire dans la boucle principale
}

#include "Arduino.h"
#include "SoftwareSerial.h"

// Création de l'objet SoftwareSerial
SoftwareSerial gpsSerial(6, 7); // RX sur pin 6, TX sur pin 7

extern "C" void __attribute__((weak)) yield(void) {}

float longitude = 0 ;
float latitude = 0 ;
// Fonction pour vérifier l'état du GPS
bool isGPSAwake(SoftwareSerial &gpsSerial) {
    // Envoie la commande de requête d'état
    gpsSerial.println("$PCAS06*1B");
    
    // Attend la réponse
    unsigned long startTime = millis();
    char buffer[100];
    int pos = 0;
    
    // Attend jusqu'à 1 seconde pour la réponse
    while (millis() - startTime < 1000) {
        if (gpsSerial.available()) {
            char c = gpsSerial.read();
            if (c == '\n') {
                buffer[pos] = '\0';
                // Vérifie si c'est la réponse à notre commande
                if (strstr(buffer, "$PCAS66") != NULL) {
                    // Le quatrième champ contient l'état du GPS
                    char* ptr = strtok(buffer, ",");
                    for (int i = 0; i < 3 && ptr != NULL; i++) {
                        ptr = strtok(NULL, ",");
                    }
                    if (ptr != NULL) {
                        // Convertit en entier
                        int state = atoi(ptr);
                        return state == 1; // 1 = actif, 0 = en veille
                    }
                }
                pos = 0;
            } else if (pos < 99) {
                buffer[pos++] = c;
            }
        }
    }
    return false; // En cas de timeout, on suppose que le GPS est en veille
}

bool getGPSdata() {
    
    // Démarrage de la communication série
    gpsSerial.begin(9600);
    
    // Vérifie si le GPS est déjà réveillé
    if (!isGPSAwake(gpsSerial)) {
        // Réveil du GPS
        gpsSerial.println("$PCAS04,1*1D");
        delay(1000); // Attendre que le GPS se réveille
    }
    
    // Variables pour la lecture des données
    char buffer[100];
    unsigned short int position = 0;
    unsigned long startTime = millis();
    const unsigned long TIMEOUT = 60000; // Timeout de 60 secondes
    
    // Boucle de lecture jusqu'à obtenir des coordonnées valides
    while ((millis() - startTime) < TIMEOUT) {
        while (gpsSerial.available()) {
            char c = gpsSerial.read();
            
            if (c == '\n') {
                buffer[position] = '\0';
                
                // Vérifie si c'est une trame GNGGA
                if (strstr(buffer, "$GNGGA") != NULL) {
                    // Parse la trame
                    char* ptr = strtok(buffer, ",");
                    unsigned short int index = 0;
                    char lat_dir = 'N', lon_dir = 'E';
                    float lat = 0.0, lon = 0.0;
                    
                    while (ptr != NULL) {
                        switch(index) {
                            case 2: // Latitude
                                lat = atof(ptr);
                                if (lat != 0) {
                                    int degrees = (int)(lat / 100);
                                    float minutes = lat - (degrees * 100);
                                    lat = degrees + (minutes / 60.0);
                                }
                                break;
                            case 3: // Direction N/S
                                lat_dir = ptr[0];
                                break;
                            case 4: // Longitude
                                lon = atof(ptr);
                                if (lon != 0) {
                                    int degrees = (int)(lon / 100);
                                    float minutes = lon - (degrees * 100);
                                    lon = degrees + (minutes / 60.0);
                                }
                                break;
                            case 5: // Direction E/W
                                lon_dir = ptr[0];
                                break;
                        }
                        ptr = strtok(NULL, ",");
                        index++;
                    }
                    
                    // Si les coordonnées sont valides
                    if (lat != 0.0 && lon != 0.0) {
                        // Appliquer les directions
                        if (lon_dir == 'W') lon = -lon;
                        if (lat_dir == 'S') lat = -lat;
                        
                        // Sauvegarder les coordonnées
                        latitude = lat;
                        longitude = lon;
                        
                        // Mettre le GPS en veille profonde
                        gpsSerial.println("$PCAS04,3*1F");
                        
                        // Fermer la communication série
                        gpsSerial.end();
                        
                        return true;
                    }
                }
                
                position = 0;
            }
            else if (position < 99) {
                buffer[position++] = c;
            }
        }
        delay(10); // Petit délai pour ne pas surcharger le processeur
    }
    
    // En cas de timeout, mettre le GPS en veille et fermer la communication
    gpsSerial.println("$PCAS04,3*1F");
    gpsSerial.end();
    
    return false;
}

void setup(){
    Serial.begin(9600);
    Serial.println("Arduino up!");
    
}
void loop() {
    delay(5000);
    Serial.println("GETTING GPS DATA");
    getGPSdata();
    Serial.println("Data is :");
    Serial.print("Lat : ");
    Serial.println(latitude);
    Serial.print("Long : ");
    Serial.println(longitude);
    
}

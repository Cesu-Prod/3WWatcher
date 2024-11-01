#include "Arduino.h"
#include "SoftwareSerial.h"

// Create SoftwareSerial object
SoftwareSerial gpsSerial(6, 7); // RX on pin 5, TX on pin 6

extern "C" void __attribute__((weak)) yield(void) {}

float longitude = 0;
float latitude = 0;

// Debug function to print NMEA sentences
void printNMEA(const char* buffer) {
    Serial.print("NMEA: ");
    Serial.println(buffer);
}

bool isGPSAwake(SoftwareSerial &gpsSerial) {
    Serial.println("Checking GPS state...");
    gpsSerial.println("$PCAS06*1B");
    
    unsigned long startTime = millis();
    char buffer[100];
    int pos = 0;
    
    while (millis() - startTime < 1000) {
        if (gpsSerial.available()) {
            char c = gpsSerial.read();
            if (c == '\n') {
                buffer[pos] = '\0';
                Serial.print("Response: ");
                Serial.println(buffer);
                
                if (strstr(buffer, "$PCAS66") != NULL) {
                    char* ptr = strtok(buffer, ",");
                    for (int i = 0; i < 3 && ptr != NULL; i++) {
                        ptr = strtok(NULL, ",");
                    }
                    if (ptr != NULL) {
                        int state = atoi(ptr);
                        Serial.print("GPS State: ");
                        Serial.println(state);
                        return state == 1;
                    }
                }
                pos = 0;
            } else if (pos < 99) {
                buffer[pos++] = c;
            }
        }
    }
    Serial.println("GPS state check timed out");
    return false;
}

bool getGPSdata() {
    Serial.println("Starting GPS communication...");
    gpsSerial.begin(9600);
    delay(100); // Allow time for serial to initialize
    
    // Clear any existing data in the buffer
    while(gpsSerial.available()) {
        gpsSerial.read();
    }
    
    if (!isGPSAwake(gpsSerial)) {
        Serial.println("GPS is sleeping, waking up...");
        gpsSerial.println("$PCAS04,1*1D");
        delay(2000); // Increased wake-up time
    }
    
    char buffer[100];
    unsigned short int position = 0;
    unsigned long startTime = millis();
    const unsigned long TIMEOUT = 60000; // 60 second timeout
    unsigned long lastValidData = millis();
    
    Serial.println("Waiting for GPS data...");
    
    while ((millis() - startTime) < TIMEOUT) {
        if (gpsSerial.available()) {
            char c = gpsSerial.read();
            
            if (c == '\n') {
                buffer[position] = '\0';
                printNMEA(buffer); // Print each NMEA sentence for debugging
                
                if (strstr(buffer, "$GNGGA") != NULL) {
                    lastValidData = millis();
                    char* ptr = strtok(buffer, ",");
                    unsigned short int index = 0;
                    char lat_dir = 'N', lon_dir = 'E';
                    float lat = 0.0, lon = 0.0;
                    
                    while (ptr != NULL) {
                        switch(index) {
                            case 2: // Latitude
                                lat = atof(ptr);
                                break;
                            case 3: // Direction N/S
                                lat_dir = ptr[0];
                                break;
                            case 4: // Longitude
                                lon = atof(ptr);
                                break;
                            case 5: // Direction E/W
                                lon_dir = ptr[0];
                                break;
                        }
                        ptr = strtok(NULL, ",");
                        index++;
                    }
                    
                    if (lat != 0.0 && lon != 0.0) {
                        // Convert DDMM.MMMM to decimal degrees
                        int lat_degrees = (int)(lat / 100);
                        float lat_minutes = lat - (lat_degrees * 100);
                        lat = lat_degrees + (lat_minutes / 60.0);
                        
                        int lon_degrees = (int)(lon / 100);
                        float lon_minutes = lon - (lon_degrees * 100);
                        lon = lon_degrees + (lon_minutes / 60.0);
                        
                        if (lon_dir == 'W') lon = -lon;
                        if (lat_dir == 'S') lat = -lat;
                        
                        latitude = lat;
                        longitude = lon;
                        
                        Serial.println("Valid GPS fix obtained!");
                        Serial.println("Putting GPS to sleep...");
                        gpsSerial.println("$PCAS04,3*1F");
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
        
        // If no valid data received for 10 seconds, print debug info
        if (millis() - lastValidData > 10000) {
            Serial.println("No valid NMEA sentences received for 10 seconds");
            lastValidData = millis();
        }
    }
    
    Serial.println("GPS timeout occurred!");
    Serial.println("Putting GPS to sleep...");
    gpsSerial.println("$PCAS04,3*1F");
    gpsSerial.end();
    
    return false;
}

void setup() {
    Serial.begin(9600);
    while (!Serial) {
        ; // Wait for serial port to connect
    }
    Serial.println("Arduino GPS Debug Version Starting...");
}

void loop() {
    delay(5000);
    Serial.println("\n--- Starting GPS Data Collection ---");
    bool success = getGPSdata();
    Serial.println("\n--- GPS Results ---");
    if (success) {
        Serial.println("GPS Fix Obtained:");
        Serial.print("Latitude: ");
        Serial.println(latitude, 6);
        Serial.print("Longitude: ");
        Serial.println(longitude, 6);
    } else {
        Serial.println("Failed to get GPS fix");
    }
}

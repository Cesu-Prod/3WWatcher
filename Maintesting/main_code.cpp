  /////////////
 // Version //
/////////////

#ifndef VERSION
#define VERSION "NaN"
#endif

#ifndef BATCH
#define BATCH "NaN"
#endif



  //////////////
 // Includes //
//////////////

#include "Arduino.h"
#include "BME280_Mini.h"
#include "EEPROM.h"
#include "SoftwareSerial.h"



#include "RTClib.h"
#include <SPI.h>
#include <SdFat.h>
#include <Wire.h>





extern "C" void __attribute__((weak)) yield(void) {}  // Acompil specific line, allows the compiler to be more optimized, but not necessary on unoptimized versions of Acompil or on better compilers.


  ////////////////////////////////////
 // Global Variables and Constants //
////////////////////////////////////

struct __attribute__((packed)) {
    uint8_t hours : 5;      // 6 bits
    uint8_t mins : 6;       // 6 bits
    uint8_t secs : 6;       // 5 bits
    uint8_t day : 5;        // 5 bits
    uint8_t month : 4;      // 4 bits
    uint8_t month : 7;      // 7 bits
    uint8_t err_code : 3;   // 3 bits
    uint8_t mode : 2;       // 2 bits
} packed_data;              // Total: 40 bits, will occupy 40 bits. To call, packed_data.name, since it is stored in the packed data space to avoid losing bits. It saves 32 ram bits.

uint8_t timer1Count = 0;

// RTC //
RTC_DS1307 rtc;                             // RTC, us ing the DS1307 chip and wire module.


// BME //
#define SDA_PIN A4
#define SCL_PIN A3
const BME280_Mini bme(SDA_PIN, SCL_PIN);    // Uses the default address (0x76).


// INTERRUPTS BTN //
#define red_btn_pin 2                       // Buttoninterrupts buttons pins config.  
#define grn_btn_pin 3
volatile uint32_t red_btn_stt; 
volatile uint32_t grn_btn_stt;


// MESURES //
const uint8_t param_num = 15;


// GPS //
float latitude = 0.0;
float longitude = 0.0;


// LED //
#define _CLK_PULSE_DELAY 20 
byte _clk_pin;
byte _data_pin;

// CONFIGURATION //
#ifndef WWWVERSION
#define WWWVERSION "NaN"
#endif

#ifndef WWWBATCH
#define WWWBATCH "NaN"
#endif



  ////////////////
 // Structures //
////////////////

typedef struct Node {
    short int value;
    Node *next;
} Node;

typedef struct Sensor {
    bool activated : 1;
    bool error : 1;
    int min_value;
    int max_value;
    Node *head_list;

    Sensor() {
        activated = false;
        error = false;
        min_value = 0;
        max_value = 0;
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
    void Update(short int value);
    short int Average();
} Sensor;

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

// Structure bits à bits pour les flags
struct FeatureFlags {
    uint8_t lumin : 1;
    uint8_t temp_air : 1;
    uint8_t hygr : 1;
    uint8_t pressure : 1;
    uint8_t eco : 1;
};

// Structure optimisée pour les paramètres
struct Parameter {
    const __FlashStringHelper* name;
    uint8_t address;
    int16_t defaultValue;
};

// Définition des chaînes en flash
#define DECLARE_FLASH_STRING(name, value) const char name[] PROGMEM = value

DECLARE_FLASH_STRING(LUMIN_STR, "LUMIN");
DECLARE_FLASH_STRING(LUMIN_LOW_STR, "LUMIN_LOW");
DECLARE_FLASH_STRING(LUMIN_HIGH_STR, "LUMIN_HIGH");
DECLARE_FLASH_STRING(TEMP_AIR_STR, "TEMP_AIR");
DECLARE_FLASH_STRING(MIN_TEMP_STR, "MIN_TEMP_AIR");
DECLARE_FLASH_STRING(MAX_TEMP_STR, "MAX_TEMP_AIR");
DECLARE_FLASH_STRING(HYGR_STR, "HYGR");
DECLARE_FLASH_STRING(HYGR_MINT_STR, "HYGR_MINT");
DECLARE_FLASH_STRING(HYGR_MAXT_STR, "HYGR_MAXT");
DECLARE_FLASH_STRING(PRESS_STR, "PRESSURE");
DECLARE_FLASH_STRING(PRESS_MIN_STR, "PRESSURE_MIN");
DECLARE_FLASH_STRING(PRESS_MAX_STR, "PRESSURE_MAX");
DECLARE_FLASH_STRING(LOG_INT_STR, "LOG_INTERVAL");
DECLARE_FLASH_STRING(FILE_SIZE_STR, "FILE_MAX_SIZE");
DECLARE_FLASH_STRING(ECO_STR, "ECO");
DECLARE_FLASH_STRING(TIMEOUT_STR, "TIMEOUT");

// Table des limites min/max en flash
const PROGMEM int16_t PARAM_LIMITS[] = {
    0, 1,        // LUMIN
    0, 1023,     // LUMIN_LOW
    0, 1023,     // LUMIN_HIGH
    0, 1,        // TEMP_AIR
    -40, 85,     // MIN_TEMP_AIR
    -40, 85,     // MAX_TEMP_AIR
    0, 1,        // HYGR
    -40, 85,     // HYGR_MINT
    -40, 85,     // HYGR_MAXT
    0, 1,        // PRESSURE
    300, 1100,   // PRESSURE_MIN
    300, 1100,   // PRESSURE_MAX
    5, 120,      // LOG_INTERVAL
    1024, 8192,  // FILE_MAX_SIZE
    0, 1,         // ECO
    1, 60        // TIMEOUT
};

class ParameterManager {
private:
    static const uint8_t PARAM_COUNT = 16;
    static char stringBuffer[16];
    static const Parameter PROGMEM params[PARAM_COUNT];
    
    void getLimits(uint8_t index, int16_t& minVal, int16_t& maxVal) {
        memcpy_P(&minVal, &PARAM_LIMITS[index * 2], sizeof(int16_t));
        memcpy_P(&maxVal, &PARAM_LIMITS[index * 2 + 1], sizeof(int16_t));
    }

    int16_t findParam(const char* paramName, Parameter& param) {
        for (uint8_t i = 0; i < PARAM_COUNT; i++) {
            memcpy_P(&param, &params[i], sizeof(Parameter));
            strcpy_P(stringBuffer, (const char*)param.name);
            
            if (strcmp(stringBuffer, paramName) == 0) {
                return i;
            }
        }
        return -1;
    }

public:
    int16_t get(const char* paramName) {
        Parameter param;
        int16_t index = findParam(paramName, param);
        
        if (index >= 0) {
            int16_t value;
            EEPROM.get(param.address, value);
            return value;
        }
        return -327; // Error if not found
    }


    bool save(const char* paramName, int16_t value) {
        Parameter param;
        int16_t index = findParam(paramName, param);
        
        if (index >= 0) {
            int16_t minVal, maxVal;
            getLimits(index, minVal, maxVal);
            
            if (value >= minVal && value <= maxVal) {
                EEPROM.put(param.address, value);
                return true;
            }
        }
        return false;
    }
    void reset() {
        Parameter param;
        for (uint8_t i = 0; i < PARAM_COUNT; i++) {
            memcpy_P(&param, &params[i], sizeof(Parameter));
            EEPROM.put(param.address, param.defaultValue);
        }
        Serial.println(F("Reset OK"));
    }
    void version() {
        Serial.print(F("3WWatcher v"));
        Serial.print(WWWVERSION);
        Serial.print(F(" b"));
        Serial.println(WWWBATCH);
    }
};

const Parameter PROGMEM ParameterManager::params[ParameterManager::PARAM_COUNT] = {
    {(const __FlashStringHelper*)LUMIN_STR, 0, 1},
    {(const __FlashStringHelper*)LUMIN_LOW_STR, 2, 255},
    {(const __FlashStringHelper*)LUMIN_HIGH_STR, 4, 768},
    {(const __FlashStringHelper*)TEMP_AIR_STR, 6, 1},
    {(const __FlashStringHelper*)MIN_TEMP_STR, 8, -10},
    {(const __FlashStringHelper*)MAX_TEMP_STR, 10, 60},
    {(const __FlashStringHelper*)HYGR_STR, 12, 1},
    {(const __FlashStringHelper*)HYGR_MINT_STR, 14, 0},
    {(const __FlashStringHelper*)HYGR_MAXT_STR, 16, 50},
    {(const __FlashStringHelper*)PRESS_STR, 18, 1},
    {(const __FlashStringHelper*)PRESS_MIN_STR, 20, 850},
    {(const __FlashStringHelper*)PRESS_MAX_STR, 22, 1080},
    {(const __FlashStringHelper*)LOG_INT_STR, 24, 10},
    {(const __FlashStringHelper*)FILE_SIZE_STR, 26, 2048},
    {(const __FlashStringHelper*)ECO_STR, 30, 0},
    {(const __FlashStringHelper*)TIMEOUT_STR, 32, 30}
};

char ParameterManager::stringBuffer[16];

static ParameterManager manager;

static Sensor ssr_lum;
static Sensor ssr_hum;
static Sensor ssr_tmp;
static Sensor ssr_prs;


  ///////////////
 // Fonctions //
///////////////


// TIMEOUT TIMER //
ISR(TIMER1_COMPA_vect) {
    timer1Count++;
    if (timer1Count >= TIMEOUT) {
        if packed_data.err_code > 0 {
            toggleled();
        }
        else {
            switch (crt_ssr) {
                case 0: // Timeout on GPS
                    lat = NULL;
                    lon = NULL;
                    if (gps_error == 1) {
                        packed_data.err_code = 1;
                        toggleled();
                    } else {
                        gps_error = 1;
                    }
                    break;
                case 1: // Timeout on RTC
                    packed_data.secs = NULL;
                    packed_data.mins = NULL;
                    packed_data.hours = NULL;
                    packed_data.days = NULL;
                    packed_data.month = NULL;
                    packed_data.year = NULL;
                    if (rtc_error == 1) {
                        packed_data.err_code = 2;
                    } else {
                        rtc_error = 1;
                    }
                    break;
                case 2: // Timeout on luminosity sensor
                    if (ssr_lum.error == 1) {
                        packed_data.err_code = 3;
                    } else {
                        ssr_lum.error = 1;
                    }
                    break;
                case 3: // Timeout on temperature sensor
                    if (ssr_tmp.error == 1) {
                        packed_data.err_code = 3;
                    } else {
                        ssr_tmp.error = 1;
                    }
                    break;
                case 4: // Timeout on pressure sensor
                    if (ssr_prs.error == 1) {
                        packed_data.err_code = 3;
                    } else {
                        ssr_prs.error = 1;
                    }
                    break;
                case 5: // Timeout on humidity sensor
                    if (ssr_hum.error == 1) {
                        packed_data.err_code = 3;
                    } else {
                        ssr_hum.error = 1;
                    }
                    break;
            }
        }
        toggleled();
        timer1Count = 0;
}
}
void timer1_init(void) {
    cli();                          // Disable interrupts
    TCCR1A = 0;                     // Set timer mode to CTC
    TCCR1B = (1 << WGM12);         // Set timer mode to CTC
    OCR1A = TIMER1_COMPARE_VALUE;   // Set compare value
    TIMSK1 = (1 << OCIE1A);        // Enable compare match interrupt
    sei();                          // Enable interrupts
}

void startTimer1(void) {
    cli();
    TCNT1 = 0;
    timer1Count = 0;
    TCCR1B |= (1 << CS12) | (1 << CS10);  // Set 1024 prescaler
    sei();
}

void stopTimer1(void) {
    cli();
    TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));  // Stop timer
    sei();
}


// Config function //
void serialConfig() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command == F("RESET")) {
            manager.reset();
            return;
        }
        if (command == F("VERSION")) {
            manager.version();
            return;
        }
        
        int equalPos = command.indexOf('=');
        if (equalPos > 0) {
            String paramName = command.substring(0, equalPos);
            int16_t value = command.substring(equalPos + 1).toInt();
            
            if (manager.save(paramName.c_str(), value)) {
                Serial.print(F("OK "));
                Serial.print(manager.get(paramName.c_str()));
            } else {
                Serial.println(F("ERR"));
            }
        }
    }
}


// GPS //
bool isGPSAwake(SoftwareSerial &gpsSerial) {
    gpsSerial.println("$PCAS06*1B");
    unsigned long startTime = millis();
    char buffer[100];
    int pos = 0;

    while (true) {
        if (gpsSerial.available()) {
            char c = gpsSerial.read();
            if (c == '\n') {
                buffer[pos] = '\0';
                if (strstr(buffer, "$PCAS66") != NULL) {
                    char* ptr = strtok(buffer, ",");
                    for (int i = 0; i < 3 && ptr != NULL; i++) {
                        ptr = strtok(NULL, ",");
                    }
                    if (ptr != NULL) {
                        int state = atoi(ptr);
                        return state == 1; // 1 = active, 0 = sleep
                    }
                }
                pos = 0;
            } else if (pos < 99) {
                buffer[pos++] = c;
            }
        }
    }
    return false; // Assume GPS is in sleep mode on timeout
}

bool getGPSdata() {
    gpsSerial.begin(9600);

    if (!isGPSAwake(gpsSerial)) {
        gpsSerial.println("$PCAS04,1*1D");
        delay(1000); // Wait for GPS to wake up
    }

    char buffer[100];
    unsigned short int position = 0;

    while (true) {
        while (gpsSerial.available()) {
            char c = gpsSerial.read();

            if (c == '\n') {
                buffer[position] = '\0';

                if (strstr(buffer, "$GNGGA") != NULL) {
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
                            case 3: // N/S Direction
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
                            case 5: // E/W Direction
                                lon_dir = ptr[0];
                                break;
                        }
                        ptr = strtok(NULL, ",");
                        index++;
                    }

                    if (lat != 0.0 && lon != 0.0) {
                        if (lon_dir == 'W') lon = -lon;
                        if (lat_dir == 'S') lat = -lat;

                        latitude = lat;
                        longitude = lon;

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
        delay(10); // Small delay to avoid CPU overload
    }

    gpsSerial.println("$PCAS04,3*1F");
    gpsSerial.end();

    return false;
}



// MEASURES //
void Measures(bool gps_eco) {
    // Variable pour suivre l'état des mesures
    crt_ssr = 0;
    // GPS //
    startTimer1()
    getGPSdata()
    if (latitude < -90.0 || latitude > 90.0 || longitude < -180.0 || longitude > 180.0) {
        packed_data.err_code = 4;
        ToggleLED();
    }
    stopTimer1()


    // HORLOGE //
    // Tableau des jours de la semaine stocké en mémoire programme
    const char week_days[] PROGMEM = "SUNMONTUEWEDTHUFRISAM";

    // Récupération de la date et de l'heure actuelle
    static DateTime now = rtc.now(); // Variable statique pour réduire la pile
    rtc_time = crnt_time; // Mettre à jour le temps de la dernière lecture RTC

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
    if (crnt_time - rtc_time >= 30000 && !dataValid) {
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
            
            packed_data.err_code = 1;
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
        packed_data.err_code = 4;
    }
    // Vérification si le capteur de luminosité a été mis à jour dans les 30 secondes
    if (crnt_time - lum_time >= 30000 && lum == 0) {
        if (ssr_lum.error == 0) {
            ssr_lum.error = 1; // 30 secondes sans mise à jour
        } else {
            
            packed_data.err_code = 3;
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
        packed_data.err_code = 4;
    }
    // Vérification si le capteur d'humidité a été mis à jour dans les 30 secondes
    if (crnt_time - hum_time >= 30000 && hum == 0) {
        if (ssr_hum.error == 0) {
            ssr_hum.error = 1; // 30 secondes sans mise à jour
        } else {
            
            packed_data.err_code = 3;
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
        packed_data.err_code = 4;
    }
    // Vérification si le capteur de pression a été mis à jour dans les 30 secondes
    if (crnt_time - prs_time >= 30000 && prs == 0) {
        if (ssr_prs.error == 0) {
            ssr_prs.error = 1; // 30 secondes sans mise à jour
        } else {
            
            packed_data.err_code = 3;
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
        packed_data.err_code = 4;
    }
    // Vérification si le capteur de température a été mis à jour dans les 30 secondes
    if (crnt_time - tmp_time >= 30000 && tmp == 0) {
        if (ssr_tmp.error == 0) {
            ssr_tmp.error = 1; // 30 secondes sans mise à jour
        } else {
            
            packed_data.err_code = 3;
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

    
    pinMode(_clk_pin, OUTPUT);
    pinMode(_data_pin, OUTPUT);
    setColorRGB(0, 0, 0);
}

void setColorRGB(byte red, byte green, byte blue) {

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
    if (packed_data.err_code > 0) {
        byte color1[3] = {255, 0, 0};
        byte color2[3];
        bool is_second_longer;

        switch (packed_data.err_code) {
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
        while (true);
    } else {
        if (mode) {
            setColorRGB(0, 255, 0);
        } else {
            setColorRGB(0, 0, 255);
        }
    }
}

// MODES //

void Standard() {
    if (mode = false) {
        param[0].val = param[0].val / 2;
    }
    mode = true;
    Mesures(false);
    toggleLED();
    mesure_save();
    Serial.flush();
    delay(param[0].val);
}

void Economique() {
    mode = false;
    param[0].val = param[0].val * 2;
    Mesures(true);
    toggleLED();
    mesure_save();
    Serial.flush();
    delay(param[0].val);
}

void Send_Serial() {
    Serial.println(String(hour) + ":" + String(minute) + ":" + String(second) + " - " + 
            String(lat) + " " + String(lat_dir) + ", " + 
            String(lon) + " " + String(lon_dir) + ", " + 
            String(ssr_lum.Average()) + ", " + 
            String(ssr_hum.Average()) + ", " + 
            String(ssr_tmp.Average()) + ", " + 
            String(ssr_prs.Average()));
}

void Maintenance() {
    setColorRGB(255, 20, 0);

    while (true) {
        Mesures(false);
        Send_Serial();
        delay(param[0].val);
    }
    Serial.println("1");
}

void ToggleMode (bool color) {
    if (mode){
        if (color) {
            Economique();
        } else {
            Maintenance();
        }
    } else {
        if (color) {
            Standard();
        } else {
            Maintenance();
        }
    }
    if (Serial.available() > 0){
        if (color){
            Economique();
        } else {
            Standard();
        }
    }
}

// INTERRUPT //
void red_btn_fall() {
    if (config_mode == true) {
        return;
    }
    // Gestion du bouton rouge
    red_btn_time = crnt_time; // Enregistrer le temps de début
}

void red_btn_rise() {
    if (config_mode == true) {
        return;
    }
    // Gestion du bouton rouge
    if (crnt_time - red_btn_time > 5000) { // Si plus de 5000 ms passées
        ToggleMode(false); // Appel de ToggleMode
    }
}

void grn_btn_fall() {
    if (config_mode == true) {
        return;
    }
    // Gestion du bouton vert
    grn_btn_time = crnt_time; // Enregistrer le temps de début
}

void grn_btn_rise() {
    if (config_mode == true) {
        return;
    }
    // Gestion du bouton vert
    if (crnt_time - grn_btn_time > 5000) { // Si plus de 5000 ms passées
        ToggleMode(true); // Appel de ToggleMode
    }
}

// SD //
void create_new_file() {
    // Générer un nom de fichier unique
    sprintf(file_name, "%02d%02d%02d_%01d.LOG", year % 100, month, day, file_idx);
    mes_file = SD.open(file_name, FILE_WRITE);
    if (!mes_file) {
        packed_data.err_code = 6;
        return;
    }
}

void write_mes(String data) {
    // Vérifier le changement de date
    if (day != pre_day || month != pre_month || year != pre_year) {
        // Fermer le fichier actuel si ouvert
        if (mes_file) {
            mes_file.close();
        }

        // Réinitialiser l'index du fichier
        file_idx = 0;

        // Créer un nouveau fichier pour le nouveau jour
        create_new_file();

        // Mettre à jour le jour, le mois et l'année précédents
        pre_day = day;
        pre_month = month;
        pre_year = year;
    }

    if (mes_file.size() >= param[1].val) {
        // Fermer le fichier actuel
        mes_file.close();

        // Incrémenter l'index du fichier
        file_idx++;

        // Renommer le fichier
        char old_file_name[12];
        sprintf(old_file_name, "%02d%02d%02d_%01d.LOG", year % 100, month, day, file_idx);
        String new_file_name = String(file_name);
        SD.rename(old_file_name, new_file_name);
        
        // Créer un nouveau fichier
        create_new_file();
    }
    // Écrire les données dans le fichier
    mes_file.println(data);
}

void mesure_save() {
    write_mes(String(hour) + ":" + String(minute) + ":" + String(second) + " - " + 
               String(lat) + " " + String(lat_dir) + ", " + 
               String(lon) + " " + String(lon_dir) + ", " + 
               String(ssr_lum.Average()) + ", " + 
               String(ssr_hum.Average()) + ", " + 
               String(ssr_tmp.Average()) + ", " + 
               String(ssr_prs.Average()));
}


  ////////////////////
 // Initialisation //
////////////////////

void setup() {
    Serial.begin(9600);
    Init_LED(7, 8);
    pinMode(red_btn_pin, INPUT_PULLUP);
    pinMode(grn_btn_pin, INPUT_PULLUP);

    if (!rtc.begin()) {
        
        packed_data.err_code = 1;
        toggleLED();
        while (true);
    }

    if (!bme.begin()) {
        
        packed_data.err_code = 1;
        toggleLED();
        while (1);
    }

    attachInterrupt(digitalPinToInterrupt(red_btn_pin), red_btn_fall, FALLING);
    attachInterrupt(digitalPinToInterrupt(red_btn_pin), red_btn_rise, RISING);
    attachInterrupt(digitalPinToInterrupt(grn_btn_pin), grn_btn_fall, FALLING);
    attachInterrupt(digitalPinToInterrupt(grn_btn_pin), grn_btn_rise, RISING);

    // Initialiser la carte SD
    if (!SD.begin(chip_slct)) {
        packed_data.err_code = 6;
        return;
    }
}


  ////////////////////
 // Code principal //
////////////////////

void loop() {
    crnt_time = millis();
    if (digitalRead(red_btn_pin) == LOW) { // Si le bouton rouge est appuyé
        config_mode = true;
        Configuration(); // Mode configuration
    }

    Standard(); // Mode standard
}
  /////////////
 // Version //
/////////////

#ifndef VERSION
#define VERSION "NaN"
#endif

#ifndef BATCH
#define BATCH "NaN"
#endif

#define F_CPU 16000000UL  // Your CPU frequency (usually 16MHz for Arduino)
#define TIMER1_COMPARE_VALUE ((F_CPU / (256UL * 1)) - 1)  // For 1Hz interrupt

  //////////////
 // Includes //
//////////////

#include "Arduino.h"
#include "BME280_Mini.h"
#include "EEPROM.h"
#include "SoftwareSerial.h"





extern "C" void __attribute__((weak)) yield(void) {}  // Acompil specific line, allows the compiler to be more optimized, but not necessary on unoptimized versions of Acompil or on better compilers.


  ////////////////////////////////////
 // Global Variables and Constants //
////////////////////////////////////




uint8_t err_code;   // 3 bits
uint8_t crt_ssr;
uint8_t mode;       // 2 bits
uint8_t timer1Count = 0;
const char* const week_days[7] PROGMEM = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};




// BME //
#define SDA_PIN A4
#define SCL_PIN A3
const BME280_Mini bme(SDA_PIN, SCL_PIN);    // Uses the default address (0x76).
BME280_Mini::Data data;

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
bool rtc_error; 
bool gps_error;
SoftwareSerial gpsSerial(6, 7); // RX on pin 6, TX on pin 7

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

#define lumin_pin A0





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

static Sensor ssr_lum;
static Sensor ssr_hum;
static Sensor ssr_tmp;
static Sensor ssr_prs;


// CONFIG //

// Bit-field structure for feature flags
struct FeatureFlags {
    uint8_t lumin : 1;
    uint8_t temp_air : 1;
    uint8_t hygr : 1;
    uint8_t pressure : 1;
    uint8_t eco : 1;
};

// Optimized structure for parameters
struct Parameter {
    const __FlashStringHelper* name;
    uint8_t address;
    int16_t defaultValue;
};

// Define flash strings
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

// Parameter limits in flash
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
    0, 1,        // ECO
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



// RTC //
struct DateTime {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t date;
    uint8_t month;
    uint8_t year;
};

DateTime now;



// DS1307 I2C address
#define DS1307_ADDRESS 0x68

// Register addresses
#define SECONDS_REG 0x00
#define MINUTES_REG 0x01
#define HOURS_REG 0x02
#define DAY_REG 0x03
#define DATE_REG 0x04
#define MONTH_REG 0x05
#define YEAR_REG 0x06
#define CONTROL_REG 0x07


  ///////////////
 // Functions //
///////////////

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

void setColorRGB(byte red, byte green, byte blue) {

    // Send data frame prefix
    for (byte i = 0; i < 4; i++) {
        sendByte(0x00);
    }
    
    sendColor(red, green, blue);
}

void Init_LED(byte clk_pin, byte data_pin) {
    _clk_pin = clk_pin;
    _data_pin = data_pin;

    
    pinMode(_clk_pin, OUTPUT);
    pinMode(_data_pin, OUTPUT);
    setColorRGB(0, 0, 0);
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
        while (true);
    } else {
        if (mode) {
            setColorRGB(0, 255, 0);
        } else {
            setColorRGB(0, 0, 255);
        }
    }
}


uint8_t TIMEOUT = manager.get(TIMEOUT);


// RTC //

// Convert BCD to decimal
uint8_t bcdToDec(uint8_t bcd) {
    return ((bcd / 16) * 10) + (bcd % 16);
}

// Initialize I2C
void initI2C() {
    TWSR = 0;
    TWBR = ((F_CPU/100000)-16)/2;
    TWCR = (1 << TWEN);
}

uint8_t readRTCRegister(uint8_t reg) {
    // Start condition
    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Send device address for write
    TWDR = DS1307_ADDRESS << 1;
    TWCR = (1<<TWINT) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Send register address
    TWDR = reg;
    TWCR = (1<<TWINT) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Repeated start
    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Send device address for read
    TWDR = (DS1307_ADDRESS << 1) | 0x01;
    TWCR = (1<<TWINT) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Read data with NACK
    TWCR = (1<<TWINT) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    uint8_t data = TWDR;

    // Stop condition
    TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);

    return data;
}


// Convert decimal to BCD
uint8_t decToBcd(uint8_t dec) {
    return ((dec / 10) << 4) + (dec % 10);
}

DateTime readDateTime() {
    DateTime dt;
    dt.second = bcdToDec(readRTCRegister(SECONDS_REG) & 0x7F);
    dt.minute = bcdToDec(readRTCRegister(MINUTES_REG));
    dt.hour = bcdToDec(readRTCRegister(HOURS_REG) & 0x3F);
    dt.day = bcdToDec(readRTCRegister(DAY_REG));
    dt.date = bcdToDec(readRTCRegister(DATE_REG));
    dt.month = bcdToDec(readRTCRegister(MONTH_REG));
    dt.year = bcdToDec(readRTCRegister(YEAR_REG));
    return dt;
}
void writeRTCRegister(uint8_t reg, uint8_t value) {
    // Start condition
    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Send device address for write
    TWDR = DS1307_ADDRESS << 1;
    TWCR = (1<<TWINT) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Send register address
    TWDR = reg;
    TWCR = (1<<TWINT) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Send data
    TWDR = value;
    TWCR = (1<<TWINT) | (1<<TWEN);
    while (!(TWCR & (1<<TWINT)));

    // Stop condition
    TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
}
void deinitI2C() {
    // Disable TWI
    TWCR = 0;
    // Set SDA and SCL pins to input mode
    DDRC &= ~((1 << PINC4) | (1 << PINC5));
    PORTC &= ~((1 << PINC4) | (1 << PINC5));
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

                        gpsSerial.println("$PCAS04,3*1F"); // Sleep mode
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

// TIMEOUT TIMER //
ISR(TIMER1_COMPA_vect) {
    timer1Count++;
    if (timer1Count >= TIMEOUT) {
        if (err_code > 0) {
            toggleLED();
        }
        else {
            switch (crt_ssr) {
                case 0: // Timeout on GPS
                    latitude = NULL;
                    longitude = NULL;
                    if (gps_error == 1) {
                        err_code = 1;
                        toggleLED();
                    } else {
                        gps_error = 1;
                    }
                    break;
                case 1: // Timeout on RTC
                    now.second = NULL;
                    now.minute = NULL;
                    now.hour = NULL;
                    now.day = NULL;
                    now.month = NULL;
                    now.year = NULL;
                    now.date = NULL;
                    if (rtc_error == 1) {
                        err_code = 2;
                    } else {
                        rtc_error = 1;
                    }
                    break;
                case 2: // Timeout on luminosity sensor
                    if (ssr_lum.error == 1) {
                        err_code = 3;
                    } else {
                        ssr_lum.error = 1;
                    }
                    break;
                case 3: // Timeout on temperature sensor
                    if (ssr_tmp.error == 1) {
                        err_code = 3;
                    } else {
                        ssr_tmp.error = 1;
                    }
                    break;
                case 4: // Timeout on pressure sensor
                    if (ssr_prs.error == 1) {
                        err_code = 3;
                    } else {
                        ssr_prs.error = 1;
                    }
                    break;
                case 5: // Timeout on humidity sensor
                    if (ssr_hum.error == 1) {
                        err_code = 3;
                    } else {
                        ssr_hum.error = 1;
                    }
                    break;
            }
        }
        toggleLED();
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


// MEASURES //
void Measures(bool gps_eco) {
    // Variable pour suivre l'état des mesures
    crt_ssr = 0;
    // GPS //
    startTimer1();
    getGPSdata();
    if (latitude < -90.0 || latitude > 90.0 || longitude < -180.0 || longitude > 180.0) {
        err_code = 4;
        toggleLED();
    }
    stopTimer1();

    crt_ssr = 1;

    // RTC //
    startTimer1();
    DateTime now = readDateTime();
    if (now.second == NULL || now.second > 60 || now.second < 0 || now.minute == NULL || now.minute > 60 || now.minute < 0 || now.hour == NULL || now.hour > 24 || now.hour < 0 || now.day == NULL || now.day > 31 || now.day < 1 || now.date == NULL || now.date > 7 || now.date < 1 || now.month == NULL || now.month > 12 || now.month < 1 || now.year == NULL) {
        err_code = 4;
        toggleLED();
    }
    delay(2000);
    DateTime now2 = readDateTime();
    if (now2.second == now.second){
        err_code = 4;
        toggleLED();
    }
    stopTimer1();


    crt_ssr = 2;
    

    // LUMINOSITY //
    startTimer1();
    unsigned int lum = 0;
    lum = analogRead(lumin_pin);
    if (lum >= 0 && lum <= 1023) {
        ssr_lum.Update(lum);
    } else {
        err_code = 4;
        toggleLED();      
    }
    stopTimer1();


    crt_ssr = 3;


    // TEMPERATURE //
    startTimer1();
    uint8_t tmp = 0;
    while (!bme.wake()){
        delay(100);
    }
    while (!bme.read(data)){
        delay(100);
    }
    ssr_hum.update(data.humidity)
    hum = bme.readHumidity();
    if (hum < 0 && hum > 100) {
        ssr_hum.Update(hum);
    } else {
        err_code = 4;
    }
    stopTimer1();
    
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
    if (crnt_time - prs_time >= 30000 && prs == 0) {
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
    if (crnt_time - tmp_time >= 30000 && tmp == 0) {
        if (ssr_tmp.error == 0) {
            ssr_tmp.error = 1; // 30 secondes sans mise à jour
        } else {
            
            err_code = 3;
            ssr_tmp.error = 0;
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
            String(latitude) + " " + String(lat_dir) + ", " + 
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

  ////////////////////
 // Initialisation //
////////////////////

void setup() {
    if (digitalRead(red_btn_pin) == LOW) { // Si le bouton rouge est appuyé
        mode = 0;
        Configuration(); // Mode configuration
    }

    pinMode(A0, INPUT)
    Init_LED(7, 8);
    pinMode(red_btn_pin, INPUT_PULLUP);
    pinMode(grn_btn_pin, INPUT_PULLUP);

    if (!rtc.begin()) {
        
        err_code = 1;
        toggleLED();
        while (true);
    }

    if (!bme.begin()) {
        
        err_code = 1;
        toggleLED();
        while (1);
    }

    attachInterrupt(digitalPinToInterrupt(red_btn_pin), red_btn_fall, FALLING);
    attachInterrupt(digitalPinToInterrupt(red_btn_pin), red_btn_rise, RISING);
    attachInterrupt(digitalPinToInterrupt(grn_btn_pin), grn_btn_fall, FALLING);
    attachInterrupt(digitalPinToInterrupt(grn_btn_pin), grn_btn_rise, RISING);


  ////////////////////
 // Code principal //
////////////////////

void loop() {

    Standard(); // Mode standard
}
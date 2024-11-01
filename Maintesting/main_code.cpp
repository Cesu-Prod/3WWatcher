  /////////////
 // Version //
/////////////

#ifndef VERSION
#define VERSION "NaN"
#endif

#ifndef BATCH
#define BATCH "NaN"
#endif

// Timer Configuration Constants
#define F_CPU 16000000UL
#define TIMER1_PRESCALER 1024


// Calculate compare values for 1Hz operation
#define TIMER1_COMPARE_VALUE ((F_CPU / (TIMER1_PRESCALER * 1)) - 1)  // For 1Hz interrupt



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


unsigned long lastLogTime = 0;
const unsigned long LOG_INTERVAL_MS = 60000; // 1 minute in milliseconds


uint8_t err_code;   // 3 bits
uint8_t crt_ssr;
uint8_t mode;       // 2 bits
uint8_t timer1Count = 0;
uint32_t last_command = 0;
const char* const week_days[7]= {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};




// BME //
#define SDA_PIN A4
#define SCL_PIN A3
const BME280_Mini bme(SDA_PIN, SCL_PIN);    // Uses the default address (0x76).
BME280_Mini::Data data;

// INTERRUPTS BTN //
#define red_btn_pin 2                       // Buttoninterrupts buttons pins config.  
#define grn_btn_pin 3
volatile unsigned long red_btn_stt = 0;
volatile unsigned long grn_btn_stt = 0;
#define switch_duration 5000

// Variables pour l'état des boutons
volatile bool red_btn_pressed = false;
volatile bool grn_btn_pressed = false;


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
    float value;
    Node *next;
} Node;

typedef struct Sensor {
    bool error : 1;
    Node *head_list;
    
    Sensor() {
        error = false;
        head_list = nullptr;
        
        // Create initial list with 4 nodes (0,0,0,0)
        for (uint8_t i = 0; i < 4; i++) {
            Node *node = new Node();
            node->value = 0.0;
            node->next = head_list;
            head_list = node;
        }
    }
    
    void Update(float value) {
        // Serial.println("Updating sensor with value: " + String(value));
        
        // Add new node at the beginning
        Node *node = new Node();
        node->value = value;
        node->next = head_list;
        head_list = node;
        
        // Find the second-to-last node
        Node *current = head_list;
        Node *prev = nullptr;
        int count = 0;
        
        while (current->next != nullptr) {
            prev = current;
            current = current->next;
            count++;
            
            // Break if we've found the second-to-last node
            if (count >= 3) {
                break;
            }
        }
        
        // Delete the last node and update the pointer
        if (prev != nullptr) {
            delete current;
            prev->next = nullptr;
        }
    }
    
    float Average() {
        Node *current;
        uint8_t count = 0;
        float total = 0;
        
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
} Sensor;

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

void stopTimer1(void) {
    cli();
    TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));  // Stop timer
    sei();
}


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
    //Serial.println("Sending color");
    //Serial.print("Red: ");
    //Serial.println(red);
    //Serial.print("Green: ");
    //Serial.println(green);
    //Serial.print("Blue: ");
    //Serial.println(blue);
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
    delay(100);
}

void setColorRGB(byte red, byte green, byte blue) {
    //Serial.println("Setting color");
    //Serial.print("Red: ");
    //Serial.println(red);
    //Serial.print("Green: ");
    //Serial.println(green);
    //Serial.print("Blue: ");
    //Serial.println(blue);
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
    mode = 4;
    
    while (true) {
    setColorRGB(couleur2[0], couleur2[1], couleur2[2]);
    delay(1000UL);
    setColorRGB(couleur1[0], couleur1[1], couleur1[2]);
    if (is_second_longer) {
        delay(2000UL);
    } else {
        delay(1000UL);
    }}
}

void toggleLED() {
    stopTimer1();
    //Serial.println("Got into toggleLED");
    //Serial.print("Error code: ");
    //Serial.println(err_code);
    //Serial.print("Mode: ");
    //Serial.println(mode);
    if (err_code > 0) {
        // Serial.println("ERROR IS FOUND");
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
    }   else {
        if (mode == 0) {
            setColorRGB(255,255,0);
        } else if (mode == 1) {
            setColorRGB(0,255,0);
        } else if (mode == 2) {
            setColorRGB(0,0,255);
        } else if (mode == 3) {
            setColorRGB(255,30,0);
        }
    }
}


uint8_t TIMEOUT = manager.get("TIMEOUT");


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

void setTime(uint8_t hour, uint8_t minute, uint8_t second) {
    writeRTCRegister(SECONDS_REG, decToBcd(second));
    writeRTCRegister(MINUTES_REG, decToBcd(minute));
    writeRTCRegister(HOURS_REG, decToBcd(hour));
}

void setDate(uint8_t date, uint8_t month, uint8_t year) {
    writeRTCRegister(DATE_REG, decToBcd(date));
    writeRTCRegister(MONTH_REG, decToBcd(month));
    writeRTCRegister(YEAR_REG, decToBcd(year));
}

void setDay(uint8_t day) {
    writeRTCRegister(DAY_REG, decToBcd(day));
}

// Config function //

uint8_t getDayNumber(const String& day) {
    if (day == F("MON")) return 1;
    if (day == F("TUE")) return 2;
    if (day == F("WED")) return 3;
    if (day == F("THU")) return 4;
    if (day == F("FRI")) return 5;
    if (day == F("SAT")) return 6;
    if (day == F("SUN")) return 7;
    return 0;
}


void serialConfig() {
    if (Serial.available()) {
        last_command = millis();
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        // Handle existing commands
        if (command == F("RESET")) {
            manager.reset();
            return;
        }
        if (command == F("VERSION")) {
            manager.version();
            return;
        }

        // Handle CLOCK command
        if (command.startsWith(F("CLOCK "))) {
            String timeStr = command.substring(6);
            int firstColon = timeStr.indexOf(':');
            int secondColon = timeStr.lastIndexOf(':');
            
            if (firstColon > 0 && secondColon > firstColon) {
                uint8_t hour = timeStr.substring(0, firstColon).toInt();
                uint8_t minute = timeStr.substring(firstColon + 1, secondColon).toInt();
                uint8_t second = timeStr.substring(secondColon + 1).toInt();
                
                if (hour <= 23 && minute <= 59 && second <= 59) {
                    // For testing, print the values
                    setTime(hour, minute, second);
                    Serial.println(F("OK"));
                    return;
                }
            }
            Serial.println("INVALID COMMAND/PARAMETER");
            return;
        }

        // Handle DATE command
        if (command.startsWith(F("DATE "))) {
            String dateStr = command.substring(5);
            int firstComma = dateStr.indexOf(',');
            int secondComma = dateStr.lastIndexOf(',');
            
            if (firstComma > 0 && secondComma > firstComma) {
                uint8_t day = dateStr.substring(0, firstComma).toInt();
                uint8_t month = dateStr.substring(firstComma + 1, secondComma).toInt();
                uint8_t year = dateStr.substring(secondComma + 1).toInt();
                
                if (month >= 1 && month <= 12 && 
                    day >= 1 && day <= 31 && 
                    year >= 0 && year <= 99) {
                    // For testing, print the values
                    setDate(day, month, year);
                    Serial.println(F("OK"));
                    return;
                }
            }
            Serial.println("INVALID COMMAND/PARAMETER");
            return;
        }

        // Handle DAY command
        if (command.startsWith(F("DAY "))) {
            String dayStr = command.substring(4);
            dayStr.trim();
            uint8_t dayNum = getDayNumber(dayStr);
            
            if (dayNum > 0) {
                // For testing, print the day number
                setDay(dayNum);
                Serial.println(F("OK"));
                return;
            }
            Serial.println("INVALID COMMAND/PARAMETER");
            return;
        }
        
        // Handle existing parameter setting
        int equalPos = command.indexOf('=');
        if (equalPos > 0) {
            String paramName = command.substring(0, equalPos);
            int16_t value = command.substring(equalPos + 1).toInt();
            
            if (manager.save(paramName.c_str(), value)) {
                Serial.print(F("OK "));
                Serial.print(manager.get(paramName.c_str()));
            } else {
                Serial.println(F("INVALID COMMAND/PARAMETER"));
            }
        }
    }
}


// GPS //


bool getGPSdata() {
    gpsSerial.begin(9600);
    // Serial.println("IN GET GPS DATA");

    // Serial.println("WAKING GPS");
    gpsSerial.println("$PCAS04,1*1D");
    delay(1000); // Wait for GPS to wake up

    char buffer[100];
    unsigned short int position = 0;
    bool validdata = false;
    while (!validdata) {
        // Serial.println("LOOPING IN GPS LOOP");
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
                        
                        validdata = true;
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

// TIMEOUT1 TIMER //
ISR(TIMER1_COMPA_vect) {
    timer1Count++;
    // Serial.println("Timergot called");
    if (timer1Count >= TIMEOUT) {
        // Serial.println("GOT A TIMEOUT");
        if (err_code > 0) {
            // Serial.println("PRE ERROR ISR");
            toggleLED();
        }
        else {
            // Serial.println("SETTING ERROR");
            switch (crt_ssr) {
                case 0: // Timeout on GPS
                    latitude = NULL;
                    longitude = NULL;
                    if (gps_error == 1) {
                        err_code = 2;
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
                        err_code = 1;
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
    // Serial.println("Done");
}





// MEASURES //
void Measures(bool gps_eco) {
    // Serial.println("IN measures");

    crt_ssr = 0;   // To follow current sensor

    // Serial.println("Starting gps");

    // GPS //
    if (gps_eco) {
        // Serial.println("starting timer");
        delay(2000);
        startTimer1();
        // Serial.println("GETTING GPS DATA");
        getGPSdata();
        if (latitude < -90.0 || latitude > 90.0 || longitude < -180.0 || longitude > 180.0) {
            // Serial.println("GOT ERROR");
            err_code = 4;
            toggleLED();
        }
        // Serial.println("Stopping timer1");
        gps_error == 0;
        stopTimer1();
    }
    // Serial.print(latitude);
    // Serial.print("  ");
    // Serial.println(longitude);
    // Serial.println("SENSOR IS NOW 2");
    crt_ssr = 1;

    // RTC //
    // Serial.println("STARTING TIMER1");
    startTimer1();
    // Serial.println("Initializing I2C");
    initI2C();
    // Serial.println("READING RTC");
    DateTime tmp = readDateTime();
    // Serial.print(week_days[int(tmp.day) - 1]);
    // Serial.print(" ");
    // Serial.print(tmp.date);
    // Serial.print("/");
    // Serial.print(tmp.month);
    // Serial.print("/");
    // Serial.print(tmp.year);
    // Serial.print(" ");
    // Serial.print(tmp.hour);
    // Serial.print(":");
    // Serial.print(tmp.minute);
    // Serial.print(":");
    // Serial.println(tmp.second);
    if (tmp.second > 60 || tmp.second < 0 || tmp.minute > 60 || tmp.minute < 0 || tmp.hour > 24 || tmp.hour < 0 || tmp.day == NULL || tmp.day > 7 || tmp.day < 1 || tmp.date == NULL || tmp.date > 31 || tmp.date < 1 || tmp.month == NULL || tmp.month > 12 || tmp.month < 1 || tmp.year == NULL) {
        err_code = 4;
        toggleLED();
    }
    // Serial.println("Waiting 2 seconds");
    delay(2000);
    // Serial.println("Verifying RTC");
    if (tmp.second == now.second){
        // Serial.println("RTC IS NOT OK");
        err_code = 4;
        toggleLED();
    }
    now.day = tmp.day;
    now.date = tmp.date;
    now.month = tmp.month;
    now.year = tmp.year;
    now.hour = tmp.hour;
    now.minute = tmp.minute;
    now.second = tmp.second;

    // Serial.println("DEINITIALIZING I2C");
    deinitI2C();
    // Serial.println("STOPPING TIMER1");
    rtc_error = 0;
    stopTimer1();


    // Serial.println("CURRENT SENSOR IS 3");
    crt_ssr = 2;


    // LUMINOSITY //
    if (manager.get("LUMIN"))
    {
        // Serial.println("GOT INTO LUM SENSOR");
        startTimer1();
        // Serial.println("STARTED TIMER");
        unsigned int lum = 0;
        lum = analogRead(lumin_pin);
        if (lum >= 0 && lum <= 1023) {
            // Serial.println("LUMINOSITY IS OK");
            // Serial.println(lum);
            ssr_lum.Update(lum);
            // Serial.println("UPDATED LUM");
        } else {
            // Serial.println("LUM IS NOT OK");
            err_code = 4;
            toggleLED();
        }
        // Serial.println("STOPPING TIMER1");
        stopTimer1();
        ssr_lum.error = false;
    } else {
        ssr_lum.error = true;
    }

    // Serial.println(ssr_lum.Average());
    // Serial.println("CURRENT SENSOR IS 4");
    crt_ssr = 3;


    // TEMPERATURE //
    if (manager.get("TEMP_AIR")){
        // Serial.println("STARTING TIMER1");
        startTimer1();
        // Serial.println("Waking bme");
        while (!bme.wake()){
            delay(100);
        }
        // Serial.println("Readign data");
        bme.read(data);
        delay(100);
        // Serial.println("Checking data");
        // Serial.println(data.temperature);
        // Serial.println(manager.get("MIN_TEMP_AIR"));
        // Serial.println(manager.get("MAX_TEMP_AIR"));
        if (data.temperature < -40 || data.temperature > 85 || data.temperature < manager.get("MIN_TEMP_AIR") || data.temperature > manager.get("MAX_TEMP_AIR")) {  // would separate, but the given instruction says to put the sensor in error if outside the logical boundaries.
            err_code = 4;
            toggleLED();
        } else {
            // Serial.println("UPDATING TEMP");
            ssr_tmp.Update(data.temperature);
            ssr_tmp.error = false;
        }
        // Serial.println("Sleeping bme");
        while (!bme.sleep()){
            delay(100);
        }
        // Serial.println("STOPPING TIMER1");
        stopTimer1();
    } else {
        ssr_tmp.error = true ;
    }

    // Serial.println(ssr_tmp.Average());
    // Serial.println("CURRENT SENSOR IS 5");
    crt_ssr = 4;


    // HUMIDITY //
    if (manager.get("HYGR")){
        // Serial.println("STARTING TIMER1");
        startTimer1();
        // Serial.println("Waking bme");
        while (!bme.wake()){
            delay(100);
        }
        // Serial.println("Reading data");
        while (!bme.read(data)){
            delay(100);
        }
        // Serial.println("Checking data");
        // Serial.println(data.humidity);
        if (data.humidity < 0 || data.humidity > 1100) {
            err_code = 4;
            toggleLED();
        } 
        else if (data.temperature < manager.get("HYGR_MINT") || data.temperature > manager.get("HYGR_MAXT")){
            ssr_hum.error = true;
        }
        else {
            ssr_hum.Update(data.humidity);
            ssr_hum.error = false;
        }
        while (!bme.sleep()){
            delay(100);
        }
        // Serial.println("STOPPING TIMER1");
        stopTimer1();
    } else {
        ssr_tmp.error = true ;
        }

    // Serial.println(ssr_hum.Average());
    // Serial.println("CURRENT SENSOR IS 6");
    crt_ssr = 5;


    // PRESSURE //
    if (manager.get("PRESSURE")){
        // Serial.println("STARTING TIMER2");
        startTimer1();
        // Serial.println("Waking bme");
        while (!bme.wake()){
            delay(100);
        }
        // Serial.println("Reading data");
        while (!bme.read(data)){
            delay(100);
        }
        // Serial.println("Checking data");
        // Serial.println(data.pressure);
        if (data.pressure < 300 || data.pressure > 1100 || data.pressure < manager.get("PRESSURE_MIN") || data.pressure > manager.get("PRESSURE_MAX")) {  // would separate, but the given instruction says to put the sensor in error if outside the logical boundaries.
            err_code = 4;
            toggleLED();
        } else {
            ssr_prs.Update(data.pressure);
            ssr_prs.error = false;
        }
        while (!bme.sleep()){
            delay(100);
        }
        stopTimer1();
    } else {
        ssr_prs.error = true ;
    }
}


// For 
void Send_Serial() {
    // Print formatted output
    Serial.write("\033[2J");
    Serial.println();
    Serial.println("3WWatcher version " + String(WWWVERSION) + " , batch" + String(WWWBATCH));
    Serial.println("----------------------------------------");
    Serial.print("Date : ");
    Serial.print(week_days[int(now.day) - 1]);
    Serial.print(" ");
    Serial.print(now.date);
    Serial.print("/");
    Serial.print(now.month);
    Serial.print("/");
    Serial.println(now.year);
    Serial.print("Time: ");
    Serial.print(now.hour);
    Serial.print(":");
    Serial.print(now.minute);
    Serial.print(":");
    Serial.println(now.second);
    Serial.println("----------------------------------------");
    Serial.println("Location: " + String(latitude) + "  " + String(longitude));
    Serial.print("Temperature: ");
    Serial.println(ssr_tmp.Average());
    Serial.print("Luminosity: ");
    if (ssr_lum.Average() < manager.get("LUMIN_LOW")) {
        Serial.println("LOW");
    } else if (ssr_lum.Average() > manager.get("LUMIN_HIGH")) {
        Serial.println("HIGH");
    } else {
        Serial.println("MEDIUM");
    }

    Serial.print("Humidity: ");
    Serial.println(String(ssr_hum.Average()));
    Serial.print("Pressure: ");
    Serial.println(ssr_prs.Average());
    Serial.println("----------------------------------------");
}

void checkLoggingInterval() {
    unsigned long currentTime = millis();
    if (currentTime - lastLogTime >= (unsigned long)manager.get("LOG_INTERVAL") * LOG_INTERVAL_MS) {
        Serial.println(F("Called Save to sd"));
        Send_Serial();
        lastLogTime = currentTime;
    }
}


// MODES //

void Standard() {
    Serial.println("Standard");
    stopTimer1();
    setColorRGB(0, 255, 0);
    if (manager.get("ECO") == 1) {
        float tmp_inter = manager.get("LOG_INTERVAL");
        manager.save("LOG_INTERVAL", tmp_inter/2);
    }
    manager.save("ECO", 0);
    mode = 1;
    toggleLED();
    while (true) {
    Measures(true);
    Serial.println("Measured Standard");
    delay(manager.get("LOG_INTERVAL")*12000);  // Why 12000? We've got to do 3 measures between each log, so we multiply by a third of a minute in milliseconds.
    checkLoggingInterval();
    }
}

void Economic() {
    Serial.println("Economic");
    stopTimer1();
    setColorRGB(0, 20, 255);
    if (manager.get("ECO") == 0) {
        float tmp_inter = manager.get("LOG_INTERVAL");
        manager.save("LOG_INTERVAL", tmp_inter*2);
    }
    manager.save("ECO", 1);
    mode = 2;
    toggleLED();

    while (true) {
    Measures(false);
    Serial.println("Measured Economic");
    delay(manager.get("LOG_INTERVAL")*12000);  // Why 12000? We've got to do 3 measures between each log, so we multiply by a third of a minute in milliseconds.
    checkLoggingInterval();
    Measures(true);
    Serial.println("Measured Standard (eco)");
    delay(manager.get("LOG_INTERVAL")*12000);  // Why 12000? We've got to do 3 measures between each log, so we multiply by a third of a minute in milliseconds.
    checkLoggingInterval();
    }
}

void Maintenance() {
    Serial.println("Maintenance");
    stopTimer1();
    setColorRGB(255, 30, 0);
    mode = 3;
    
    // Serial.println("Starting loop");
    while (true) {
    // Serial.println("Starting measures.");
    Measures(true);
    // Serial.println("Sendign to SERIAL");
    Send_Serial();
    }
}

void Configuration() {
    mode = 0;
    stopTimer1();
    setColorRGB(255, 255, 0);
    toggleLED();
    manager.version();
    while (millis() - last_command < 1800000) { // 30 * 60 * 1000 = 30 minutes
        serialConfig();
    }
    if (manager.get("ECO") == 1) {
        Economic();
    } else {
        Standard();
    }
}




void ToggleMode(uint8_t button) {
    //Serial.println("Toggling mode");
    //Serial.print("Button: ");
    //Serial.println(button);
    //Serial.print("Mode: ");
    //Serial.println(mode);
   if (button == 1) {
switch (mode) {

    case 1:
        Maintenance();
        break;
    case 2:
        Maintenance();
        break;
    case 3:
        if (manager.get("ECO") == 1) {
            Economic();
        } else {
            Standard();
        }
        break;
    default:
        break;
}
   } else if (button == 0) {
switch (mode) {
    case 1:
        Economic();
        break;
    case 2:
        Standard();
        break;
    default:
        break;
}
   }
}

// INTERRUPT //
void red_btn_change() {
  if (digitalRead(red_btn_pin) == LOW) { // Bouton pressé
    red_btn_stt = millis();
    red_btn_pressed = true;
    Serial.println("Red button pressed");
  } else if ((millis() - red_btn_stt) > switch_duration &&  red_btn_pressed) {
    red_btn_pressed = false;
    ToggleMode(1);
    Serial.println("Toggle mode red");
  } else {
    Serial.println("Red button released");
}
}

// Gestionnaire d'interruption pour le bouton 3
void grn_btn_change() {
  if (digitalRead(grn_btn_pin) == LOW) { // Bouton pressé
    grn_btn_stt = millis();
    grn_btn_pressed = true;
    Serial.println("Green button pressed");
  } else if ((millis() - grn_btn_stt) > switch_duration &&  grn_btn_pressed) {
    grn_btn_pressed = false;
    ToggleMode(0);
    Serial.println("Toggle mode green");
  } else {
    Serial.println("Green button released");
}
}


  ////////////////////
 // Initialisation //
////////////////////

void setup() {
    bme.begin();
    Serial.begin(9600);
    Serial.println("Arduino UP!");
    if (digitalRead(red_btn_pin) == LOW) { // Si le bouton rouge est appuyé
        mode = 0;
        Serial.println("CONFIG");
        Configuration(); // Mode configuration
    }
    //Serial.println("Starting pinmode config");
    pinMode(A0, INPUT);
    pinMode(red_btn_pin, INPUT_PULLUP);
    pinMode(grn_btn_pin, INPUT_PULLUP);
    //Serial.println("Starting LED");
    Init_LED(7, 8);
    attachInterrupt(digitalPinToInterrupt(red_btn_pin), red_btn_change, CHANGE);
    attachInterrupt(digitalPinToInterrupt(grn_btn_pin), grn_btn_change, CHANGE);
    if (manager.get("ECO") == 1) {
        Economic();
    } else {
        Standard();
    }
}

  ////////////////////
 // Code principal //
////////////////////

void loop() {
}
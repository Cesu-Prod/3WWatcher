#include "BME280_Arduino_I2C.h"
#include "Arduino.h"

BME280_Arduino_I2C bme;

extern "C" void __attribute__((weak)) yield(void) {}

void setup() {
    Serial.begin(9600);
    while (!Serial) {
        // Wait for serial port to connect. Needed for native USB port only
    }

    if (bme.begin() == 0) {
        Serial.println("BME280 initialized successfully.");
    } else {
        Serial.println("Failed to initialize BME280.");
        /*
            Failed to initialize:
            Returning code 1: Wire is not available
            Returning code 2: Device has not been found
        */
    }
}

void loop() {
    BME280Data* data = bme.read();

    // nullptr if read failed
    if (data != nullptr) {
        Serial.print("Temperature: ");
        Serial.print(data->temperature);
        Serial.print(" °C, Humidity: ");
        Serial.print(data->humidity);
        Serial.print(" %, Pressure: ");
        Serial.print(data->pressure);
        Serial.println(" hPa");
    } else {
        Serial.println("Failed to read data from BME280.");
    }

    delay(2000); // Wait for 2 seconds
}

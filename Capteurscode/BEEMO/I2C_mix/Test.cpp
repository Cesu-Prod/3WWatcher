#include "BME280_Simple.h"
#include "Arduino.h"

extern "C" void __attribute__((weak)) yield(void) {}

// Define the SDA and SCL pins
const int SDA_PIN = A4; // Change these pin numbers according to your setup
const int SCL_PIN = A3;

// Create BME280 instance
BME280_Simple bme(SDA_PIN, SCL_PIN);

void setup() {
    // Initialize serial communication
    Serial.begin(9600);
    Serial.println("BME280 Simple Test");
    
    // Initialize the sensor
    if (!bme.begin()) {
        Serial.println("Could not find BME280 sensor!");
        while (1); // Don't continue if sensor initialization failed
    }
    
    Serial.println("BME280 sensor found and initialized!");
    delay(1000); // Wait a second before starting measurements
}

void loop() {
    // Read and print temperature
    float temperature = bme.readTemperature();
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");
    
    // Read and print pressure
    float pressure = bme.readPressure();
    Serial.print("Pressure: ");
    Serial.print(pressure / 100.0); // Convert Pa to hPa
    Serial.println(" hPa");
    
    // Read and print humidity
    uint8_t humidity = bme.readHumidity();
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
    
    Serial.println(); // Empty line for readability
    
    // Put sensor to sleep
    bme.sleep();
    delay(2000); // Wait 2 seconds
    
    // Wake up sensor
    bme.wake();
    delay(100); // Give sensor time to wake up
}
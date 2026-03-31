#ifndef SOIL_SENSOR_H
#define SOIL_SENSOR_H

#include <Arduino.h>

class SoilSensor {
public:
    SoilSensor(uint8_t pin);
    
    // Initializes the sensor (e.g. pin mode)
    void begin();
    
    // Reads the analog value from the sensor
    uint16_t readMoistureRaw();
    
    // Optional: read current moisture as a percentage (a basic linear mapping)
    // Assumes 0 = max moisture (wet), 4095 = min moisture (dry) which is common for some soil sensors.
    // Adjust mapping based on actual sensor calibration if needed.
    uint8_t readMoisturePercentage();
    
private:
    uint8_t _pin;
    
    // Calibration bounds (typical analog readings for dry vs wet soil)
    uint16_t _dryValue;
    uint16_t _wetValue;
};

#endif // SOIL_SENSOR_H

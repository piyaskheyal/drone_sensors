#ifndef GAS_SENSOR_H
#define GAS_SENSOR_H

#include <Arduino.h>

class GasSensor {
public:
    GasSensor(uint8_t pin, uint16_t threshold);
    
    // Initializes the sensor (e.g. pin mode)
    void begin();
    
    // Reads the analog value from the sensor
    uint16_t readRawValue();
    
    // Checks if the gas level is hazardous based on threshold
    bool isHazardous();
    
    // Setter for threshold so it can be tuned later
    void setThreshold(uint16_t newThreshold);
    
    // Getter for threshold
    uint16_t getThreshold() const;

private:
    uint8_t _pin;
    uint16_t _threshold;
};

#endif // GAS_SENSOR_H

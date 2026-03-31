#include "SoilSensor.h"

// Note: Dry/Wet calibration values might need adjusting based on the specific sensor and soil type.
// Standard ESP32 12-bit ADC gives 0-4095. Usually, in air, reading is high. In water, reading is low.
SoilSensor::SoilSensor(uint8_t pin) 
    : _pin(pin), _dryValue(3500), _wetValue(1500) {
}

void SoilSensor::begin() {
    pinMode(_pin, INPUT);
}

uint16_t SoilSensor::readMoistureRaw() {
    return analogRead(_pin);
}

uint8_t SoilSensor::readMoisturePercentage() {
    uint16_t rawValue = readMoistureRaw();
    
    // Constrain the value between calibration boundaries
    if (rawValue > _dryValue) rawValue = _dryValue;
    if (rawValue < _wetValue) rawValue = _wetValue;
    
    // Map bounds to percentage (0% to 100%)
    long mapped = map(rawValue, _dryValue, _wetValue, 0, 100);
    
    // Safety check just in case map() evaluates weirdly, though constrain already handled it
    return static_cast<uint8_t>(constrain(mapped, 0, 100));
}

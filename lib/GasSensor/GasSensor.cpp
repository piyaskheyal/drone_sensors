#include "GasSensor.h"

GasSensor::GasSensor(uint8_t pin, uint16_t threshold) 
    : _pin(pin), _threshold(threshold) {
}

void GasSensor::begin() {
    pinMode(_pin, INPUT);
}

uint16_t GasSensor::readRawValue() {
    // Read the 12-bit ADC value (0-4095 on ESP32)
    return analogRead(_pin);
}

bool GasSensor::isHazardous() {
    return readRawValue() >= _threshold;
}

void GasSensor::setThreshold(uint16_t newThreshold) {
    _threshold = newThreshold;
}

uint16_t GasSensor::getThreshold() const {
    return _threshold;
}

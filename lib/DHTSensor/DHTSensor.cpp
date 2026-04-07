#include "DHTSensor.h"

DHTSensor::DHTSensor(uint8_t pin) : dhtPin(pin), dht(pin, DHT11), currentTemperature(0.0), currentHumidity(0.0), lastReadTime(0) {
}

void DHTSensor::begin() {
    dht.begin();
    // Do an initial read
    update();
}

void DHTSensor::update() {
    unsigned long currentMillis = millis();
    // DHT11 should be read at most once every 2 seconds
    if (currentMillis - lastReadTime >= 2000 || lastReadTime == 0) {
        float h = dht.readHumidity();
        float t = dht.readTemperature();
        
        // Check if any reads failed and exit early (to try again).
        if (!isnan(h) && !isnan(t)) {
            currentHumidity = h;
            currentTemperature = t;
        }
        
        lastReadTime = currentMillis;
    }
}

float DHTSensor::getTemperature() const {
    return currentTemperature;
}

float DHTSensor::getHumidity() const {
    return currentHumidity;
}

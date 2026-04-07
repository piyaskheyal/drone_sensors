#ifndef DHT_SENSOR_H
#define DHT_SENSOR_H

#include <Arduino.h>
#include <DHT.h>

class DHTSensor {
public:
    DHTSensor(uint8_t pin);
    void begin();
    
    // Updates reading, ideally call it occasionally (DHT11 is slow, <= 0.5Hz max)
    void update();
    
    float getTemperature() const;
    float getHumidity() const;

private:
    uint8_t dhtPin;
    DHT dht;
    
    float currentTemperature;
    float currentHumidity;
    unsigned long lastReadTime;
};

#endif

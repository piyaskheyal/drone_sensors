#ifndef SEED_DISPENSER_H
#define SEED_DISPENSER_H

#include <Arduino.h>
#include <ESP32Servo.h>

class SeedDispenser {
public:
    SeedDispenser(uint8_t pin);
    void begin();
    
    // Call this repeatedly in the main loop to keep the process non-blocking
    void update();
    
    // Set the frequency/interval in milliseconds (1000 - 5000)
    void setInterval(unsigned long intervalMillis);
    unsigned long getInterval() const;
    
    // Determine current state for reporting
    bool getIsOpen() const;

private:
    uint8_t servoPin;
    Servo servo;
    
    unsigned long closedInterval;
    const unsigned long openDuration = 500; // 0.5 sec constant

    unsigned long stateStartTime;
    bool isOpen;
};

#endif

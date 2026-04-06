#include "SeedDispenser.h"

SeedDispenser::SeedDispenser(uint8_t pin) : servoPin(pin), closedInterval(2000), isOpen(false), stateStartTime(0) {
    // 2s default duration, closed state
}

void SeedDispenser::begin() {
    ESP32PWM::allocateTimer(0);
    servo.setPeriodHertz(50);      // Standard 50Hz servo
    servo.attach(servoPin, 500, 2400); // Attach to pin, min/max pulse width
    
    servo.write(0);                // Start closed
    stateStartTime = millis();
}

void SeedDispenser::setInterval(unsigned long intervalMillis) {
    // Clip to our bounds (1s to 5s)
    if (intervalMillis < 1000) intervalMillis = 1000;
    if (intervalMillis > 5000) intervalMillis = 5000;
    
    closedInterval = intervalMillis;
}

unsigned long SeedDispenser::getInterval() const {
    return closedInterval;
}

bool SeedDispenser::getIsOpen() const {
    return isOpen;
}

void SeedDispenser::update() {
    unsigned long currentMillis = millis();
    unsigned long timeInState = currentMillis - stateStartTime;
    
    if (isOpen) {
        if (timeInState >= openDuration) {
            isOpen = false;
            servo.write(0); // Close (0 degrees)
            stateStartTime = currentMillis;
        }
    } else {
        if (timeInState >= closedInterval) {
            isOpen = true;
            servo.write(60); // Open (60 degrees)
            stateStartTime = currentMillis;
        }
    }
}

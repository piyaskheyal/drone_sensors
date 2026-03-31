#include "MotorControl.h"

MotorControl::MotorControl(uint8_t pwm_pin, uint8_t in1_pin, uint8_t in2_pin)
    : _pwmPin(pwm_pin), _in1Pin(in1_pin), _in2Pin(in2_pin) {}

void MotorControl::begin() {
    pinMode(_pwmPin, OUTPUT);
    pinMode(_in1Pin, OUTPUT);
    pinMode(_in2Pin, OUTPUT);
    
    // Initialize motor to off (coast/stop)
    digitalWrite(_in1Pin, LOW);
    digitalWrite(_in2Pin, LOW);
    analogWrite(_pwmPin, 0); 
}

void MotorControl::setSpeed(int16_t speed) {
    // Constrain the speed to valid 8-bit PWM limits (-255 to 255)
    if (speed > 255) speed = 255;
    if (speed < -255) speed = -255;

    if (speed > 0) {
        // Forward
        digitalWrite(_in1Pin, HIGH);
        digitalWrite(_in2Pin, LOW);
        analogWrite(_pwmPin, speed);
    } 
    else if (speed < 0) {
        // Reverse
        digitalWrite(_in1Pin, LOW);
        digitalWrite(_in2Pin, HIGH);
        analogWrite(_pwmPin, -speed); // Convert back to positive 0-255 for analogWrite
    } 
    else {
        // Stop
        digitalWrite(_in1Pin, LOW);
        digitalWrite(_in2Pin, LOW);
        analogWrite(_pwmPin, 0);
    }
}

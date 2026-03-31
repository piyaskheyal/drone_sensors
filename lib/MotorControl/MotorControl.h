#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <Arduino.h>

class MotorControl {
public:
    // Initialize with the pins connected to the TB6612FNG driver
    MotorControl(uint8_t pwm_pin, uint8_t in1_pin, uint8_t in2_pin);
    
    // Call in setup() to configure pins
    void begin();
    
    // Set speed from -255 (full reverse) to 255 (full forward)
    // 0 will stop the motor
    void setSpeed(int16_t speed);

private:
    uint8_t _pwmPin;
    uint8_t _in1Pin;
    uint8_t _in2Pin;
};

#endif

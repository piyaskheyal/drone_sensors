#include <Arduino.h>
#include "MotorControl.h"

const int rcPin = 26; // Pin connected to the RC receiver signal

// Motor Driver Pins (TB6612FNG)
const int motorPWM = 27; 
const int motorIN1 = 14; 
const int motorIN2 = 12;

MotorControl motor(motorPWM, motorIN1, motorIN2);

// Volatile variables because they are updated inside the interrupt
volatile unsigned long pulseStartTime = 0;
volatile unsigned long rcValue = 1500; // Default to neutral

// Interrupt Service Routine (ISR) to read the PWM signal non-blocking
void IRAM_ATTR handleRCInterrupt() {
  if (digitalRead(rcPin) == HIGH) {
    pulseStartTime = micros();
  } else {
    // Calculate pulse width when it drops LOW
    rcValue = micros() - pulseStartTime;
  }
}

void setup() {
  Serial.begin(115200);
  
  motor.begin();
  
  pinMode(rcPin, INPUT);
  
  // Attach the interrupt to listen for ANY pin state change (RISING or FALLING)
  attachInterrupt(digitalPinToInterrupt(rcPin), handleRCInterrupt, CHANGE);
  
  Serial.println("RC Receiver & Motor Test Started (Interrupt Mode)");
}

void loop() {
  // Grab a safe copy of the volatile variable
  noInterrupts();
  unsigned long currentRC = rcValue;
  interrupts();
  
  int16_t motorSpeed = 0;
  
  // Add a deadband at the center (1450 to 1550) so the motor doesn't jitter at neutral
  if (currentRC > 1550 && currentRC <= 2000) {
    // Map forward stick (1551-2000) to 0-255 speed
    motorSpeed = map(currentRC, 1550, 2000, 0, 255);
  } 
  else if (currentRC < 1450 && currentRC >= 999) {
    // Map reverse stick (999-1449) to -255 to 0 speed
    // map function will return values like -255 as appropriate
    motorSpeed = map(currentRC, 1450, 999, 0, -255);
  }

  // Set the motor speed
  motor.setSpeed(motorSpeed);
  
  Serial.printf("RC Input: %lu | Motor Target Speed: %d\n", currentRC, motorSpeed);
  
  // Notice we only delay 100ms for serial printing. The RC reading happens instantly in the background!
  delay(100);
}

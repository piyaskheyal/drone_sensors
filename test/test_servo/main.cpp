#include <Arduino.h>
#include <ESP32Servo.h>

Servo testServo;
const int SERVO_PIN = 18; // The servo signal pin

void setup() {
    Serial.begin(115200);
    Serial.println("--- Starting Servo Sweep Test ---");

    // Allow allocation of all timers
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    
    // Standard 50Hz servo
    testServo.setPeriodHertz(50);
    
    // Attach servo to pin 18 (min and max pulse width in microseconds)
    testServo.attach(SERVO_PIN, 500, 2400); 
    
    Serial.println("Servo attached to pin 18. Moving to default 90 degrees...");
    testServo.write(90);
    delay(2000);
}

void loop() {
    Serial.println("Moving to 0 degrees (Open)");
    testServo.write(0);
    delay(1500);
    
    Serial.println("Moving to 90 degrees (Closed/Rest)");
    testServo.write(90);
    delay(1500);

    Serial.println("Moving to 180 degrees (Max)");
    testServo.write(180);
    delay(1500);
    
    Serial.println("Returning to 90 degrees...");
    testServo.write(90);
    delay(1500);
}

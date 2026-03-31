#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <GasSensor.h>
#include <SoilSensor.h>
#include "MotorControl.h"

// WiFi Configuration - UPDATE THESE LATER
const char *ssid = "kheyalpare";
const char *password = "asoleikheyalpare";

#define DEBUG 1 // Set to 1 to see prints, 0 to hide them

// Hardware Pins (ADC1 pins: 32, 33, 34, 35, 36, 39 to work with WiFi active)
#define GAS_PIN 32
#define SOIL_PIN 33
#define RC_PIN 26
#define MOTOR_PWM 27
#define MOTOR_IN1 16
#define MOTOR_IN2 17

uint16_t currentGasThreshold = 2000;
GasSensor gasSensor(GAS_PIN, currentGasThreshold);
SoilSensor soilSensor(SOIL_PIN);
MotorControl motor(MOTOR_PWM, MOTOR_IN1, MOTOR_IN2);

AsyncWebServer server(80);

// Volatile variables for RC interrupt
volatile unsigned long pulseStartTime = 0;
volatile unsigned long rcValue = 1500;

// Globals to store current motor state for the web server
int16_t currentMotorSpeed = 0;
unsigned long currentRCPulse = 1500;

void IRAM_ATTR handleRCInterrupt() {
    if (digitalRead(RC_PIN) == HIGH) {
        pulseStartTime = micros();
    } else {
        rcValue = micros() - pulseStartTime;
    }
}

struct SensorReadings
{
    uint16_t gasRaw;
    bool gasHazardous;
    uint16_t soilRaw;
    uint8_t soilPercent;
    int16_t motorSpeed;
    unsigned long rcPulse;
};
SensorReadings latestReadings;

unsigned long previousMillis = 0;
const long samplingInterval = 1000;

// Embedded HTML UI
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPEhtml>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Drone Sensors</title>
    <style>
        body { font-family: sans-serif; background: #f0f2f5; margin: 0; padding: 20px; text-align: center;}
        h1 { color: #333; }
        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 20px; max-width: 600px; margin: auto; }
        .card { background: white; padding: 20px; border-radius: 10px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }
        .normal { border-top: 5px solid #3498db; }
        .hazard-card { border-top: 5px solid #e74c3c; box-shadow: 0 0 15px rgba(231,76,60,0.5); }
        .val { font-size: 2.5rem; font-weight: bold; margin: 15px 0; color: #2c3e50; }
        .status { padding: 10px; border-radius: 5px; color: white; font-weight: bold; }
        .safe { background: #2ecc71; }
        .hazard { background: #e74c3c; animation: pulse 1s infinite alternate; }
        @keyframes pulse {
            from { background-color: #e74c3c; }
            to { background-color: #c0392b; }
        }
    </style>
</head>
<body>
    <h1>Drone Sensors Dashboard</h1>
    <div class="grid">
        <div id="gas-card" class="card normal">
            <h2>Gas Sensor</h2>
            <div class="val" id="gasValue">--</div>
            <div id="gasStatus" class="status safe">Safe</div>
            <p>Threshold: <span id="threshValue">--</span></p>
        </div>
        <div class="card normal" style="border-top-color: #2ecc71;">
            <h2>Soil Moisture</h2>
            <div class="val"><span id="soilValue">--</span>%</div>
            <p>Raw ADC: <span id="soilRaw">--</span></p>
        </div>
        <div class="card normal" style="border-top-color: #f39c12;">
            <h2>Motor Status</h2>
            <div class="val"><span id="motorSpeed">--</span></div>
            <p>RC Pulse: <span id="rcPulse">--</span> us</p>
        </div>
    </div>
    <script>
        async function fetchData() {
            try {
                let res = await fetch('/api/data');
                if(!res.ok)return;
                let data = await res.json();
                
                document.getElementById('gasValue').innerText = data.gas.raw;
                document.getElementById('threshValue').innerText = data.gas.threshold;
                
                let gasCard = document.getElementById('gas-card');
                let gasStatus = document.getElementById('gasStatus');
                
                if(data.gas.hazardous) {
                    gasStatus.className = 'status hazard';
                    gasStatus.innerText = 'HAZARDOUS DETECTED!';
                    gasCard.className = 'card hazard-card';
                } else {
                    gasStatus.className = 'status safe';
                    gasStatus.innerText = 'Safe';
                    gasCard.className = 'card normal';
                }
                
                document.getElementById('soilValue').innerText = data.soil.percentage;
                document.getElementById('soilRaw').innerText = data.soil.raw;

                // Motor updates
                document.getElementById('motorSpeed').innerText = data.motor.speed;
                document.getElementById('rcPulse').innerText = data.motor.pulse;
            } catch(e) { console.error(e); }
        }
        setInterval(fetchData, 1000);
        fetchData();
    </script>
</body>
</html>
)rawliteral";

void updateSensors()
{
    latestReadings.gasRaw = gasSensor.readRawValue();
    latestReadings.gasHazardous = gasSensor.isHazardous();
    latestReadings.soilRaw = soilSensor.readMoistureRaw();
    latestReadings.soilPercent = soilSensor.readMoisturePercentage();
    latestReadings.motorSpeed = currentMotorSpeed;
    latestReadings.rcPulse = currentRCPulse;

#if DEBUG
    Serial.printf("Gas: %d %s | Soil: %d%% (%d) | Motor: %d (RC: %lu)\n",
                  latestReadings.gasRaw,
                  (latestReadings.gasHazardous ? "[HAZ]" : "[SAFE]"),
                  latestReadings.soilPercent,
                  latestReadings.soilRaw,
                  latestReadings.motorSpeed,
                  latestReadings.rcPulse);
#endif
}

void setupRouting()
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", index_html); });

    server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        String json = "{";
        json += "\"gas\":{\"raw\":" + String(latestReadings.gasRaw) + ",";
        json += "\"threshold\":" + String(gasSensor.getThreshold()) + ",";
        json += "\"hazardous\":" + String(latestReadings.gasHazardous ? "true" : "false") + "},";
        json += "\"soil\":{\"raw\":" + String(latestReadings.soilRaw) + ",";
        json += "\"percentage\":" + String(latestReadings.soilPercent) + "},";
        json += "\"motor\":{\"speed\":" + String(latestReadings.motorSpeed) + ",";
        json += "\"pulse\":" + String(latestReadings.rcPulse) + "}}";
        request->send(200, "application/json", json); });
}

void setup()
{
#if DEBUG
    Serial.begin(115200);
#endif

    gasSensor.begin();
    soilSensor.begin();
    
    // Motor and RC initialization
    motor.begin();
    pinMode(RC_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(RC_PIN), handleRCInterrupt, CHANGE);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
#if DEBUG
    Serial.print("Connecting to WiFi");
#endif

    uint8_t retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20)
    {
        delay(500);
#if DEBUG
        Serial.print(".");
#endif
        retries++;
    }
    if (WiFi.status() == WL_CONNECTED)
    {
#if DEBUG
        Serial.println("\nConnected. IP: ");
        Serial.println(WiFi.localIP());
#endif
    }
    else
    {
#if DEBUG
        Serial.println("\nWiFi Failed. Running offline.");
#endif
    }

    setupRouting();
    server.begin();
}

void loop()
{
    // High-frequency responsive motor control
    noInterrupts();
    unsigned long currentRC = rcValue;
    interrupts();
    
    int16_t motorSpeed = 0;
    if (currentRC > 1550 && currentRC <= 2000) {
        motorSpeed = map(currentRC, 1550, 2000, 0, 255);
    } 
    else if (currentRC < 1450 && currentRC >= 999) {
        motorSpeed = map(currentRC, 1450, 999, 0, -255);
    }

    motor.setSpeed(motorSpeed);
    
    // Save for the web interface updates
    currentMotorSpeed = motorSpeed;
    currentRCPulse = currentRC;

    // Routine low-frequency sensor reading for Web UI (1 second interval)
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= samplingInterval)
    {
        previousMillis = currentMillis;
        updateSensors();
    }
    
    // YIELD TO WATCHDOG / SYSTEM TASKS using a tiny delay
    delay(2);
}

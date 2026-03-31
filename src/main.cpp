#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <GasSensor.h>
#include <SoilSensor.h>

// WiFi Configuration - UPDATE THESE LATER
const char *ssid = "Router 01";
const char *password = "kheyal2g";

// Hardware Pins (ADC1 pins: 32, 33, 34, 35, 36, 39 to work with WiFi active)
#define GAS_PIN 32
#define SOIL_PIN 33

uint16_t currentGasThreshold = 2000;
GasSensor gasSensor(GAS_PIN, currentGasThreshold);
SoilSensor soilSensor(SOIL_PIN);

AsyncWebServer server(80);

struct SensorReadings
{
    uint16_t gasRaw;
    bool gasHazardous;
    uint16_t soilRaw;
    uint8_t soilPercent;
};
SensorReadings latestReadings;

unsigned long previousMillis = 0;
const long samplingInterval = 1000;

// Embedded HTML UI
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
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
    </div>
    <script>
        async function fetchData() {
            try {
                let res = await fetch('/api/data');
                if(!res.ok) return;
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

    Serial.printf("Gas: %d %s | Soil: %d%% (%d)\n",
                  latestReadings.gasRaw,
                  (latestReadings.gasHazardous ? "[HAZ]" : "[SAFE]"),
                  latestReadings.soilPercent,
                  latestReadings.soilRaw);
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
        json += "\"percentage\":" + String(latestReadings.soilPercent) + "}}";
        request->send(200, "application/json", json); });
}

void setup()
{
    Serial.begin(115200);
    gasSensor.begin();
    soilSensor.begin();

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");

    uint8_t retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20)
    {
        delay(500);
        Serial.print(".");
        retries++;
    }
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nConnected. IP: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("\nWiFi Failed. Running offline.");
    }

    setupRouting();
    server.begin();
}

void loop()
{
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= samplingInterval)
    {
        previousMillis = currentMillis;
        updateSensors();
    }
}
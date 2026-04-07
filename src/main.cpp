#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <GasSensor.h>
#include <SoilSensor.h>
#include <SeedDispenser.h>
#include <DHTSensor.h>

// WiFi Configuration - UPDATE THESE LATER
const char *ssid = "drone_network";
const char *password = "drone_pass";

#define DEBUG 1 // Set to 1 to see prints, 0 to hide them

// Hardware Pins (ADC1 pins: 32, 33, 34, 35, 36, 39 to work with WiFi active)
#define GAS_PIN 32
#define SOIL_PIN 33
#define SERVO_PIN 18
#define DHT_PIN 4

uint16_t currentGasThreshold = 700;
float dispenserMinSec = 1.0;
float dispenserMaxSec = 5.0;
float dispenserStepSec = 0.5;

GasSensor gasSensor(GAS_PIN, currentGasThreshold);
SoilSensor soilSensor(SOIL_PIN);
SeedDispenser dispenser(SERVO_PIN);
DHTSensor dhtSensor(DHT_PIN);

AsyncWebServer server(80);

struct SensorReadings
{
    uint16_t gasRaw;
    bool gasHazardous;
    uint16_t soilRaw;
    uint8_t soilPercent;
    unsigned long dispenserInterval;
    bool dispenserEnabled;
    float temperatureCelsius;
    float humidityPercent;
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
        <div class="card normal" style="border-top-color: #9b59b6;">
            <h2>Environment</h2>
            <div class="val"><span id="tempValue">--</span>&deg;C</div>
            <p>Humidity: <span id="humValue">--</span>%</p>
        </div>
        <div class="card normal" style="border-top-color: #f39c12;">
            <h2>Seed Dispenser</h2>
            <div style="margin: 15px 0;">
                <label style="font-size: 1.5rem; font-weight: bold; display: flex; align-items: center; justify-content: center; gap: 10px; cursor: pointer;">
                    <input type="checkbox" id="dispenserToggle" onchange="toggleDispenser()" style="width: 25px; height: 25px;">
                    <span id="dispenserStatusText">System Off</span>
                </label>
            </div>
            <p>Current Frequency: <span id="dispenserInterval">--</span> s</p>
            <div style="margin-top: 15px;">
                <label style="display:block; margin-bottom: 5px; font-weight: bold;">Set to: <span id="sliderValueDisplay">2.0</span> s</label>
                <input type="range" id="intervalSlider" min="1" max="5" step="0.5" value="2" style="width: 100%;" oninput="document.getElementById('sliderValueDisplay').innerText = this.value">
                <button onclick="updateInterval()" style="margin-top: 10px; width: 100%; padding: 10px; border-radius: 5px; background: #f39c12; color: white; border: none; font-weight: bold; cursor: pointer;">Set Frequency</button>
            </div>
        </div>
    </div>
    <script>
        async function updateInterval() {
            let val = document.getElementById('intervalSlider').value;
            try {
                let formData = new URLSearchParams();
                formData.append('val', val * 1000); // Scale up to ms for backend
                await fetch('/api/set_interval', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: formData
                });
            } catch(e) { console.error('Failed to update interval'); }
        }

        async function toggleDispenser() {
            let isEnabled = document.getElementById('dispenserToggle').checked;
            try {
                let formData = new URLSearchParams();
                formData.append('enable', isEnabled ? '1' : '0');
                await fetch('/api/toggle_dispenser', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: formData
                });
            } catch(e) { console.error('Failed to toggle dispenser'); }
        }

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

                document.getElementById('tempValue').innerText = data.dht.temperature;
                document.getElementById('humValue').innerText = data.dht.humidity;

                // Dispenser updates
                let intervalSec = (data.dispenser.interval / 1000.0).toFixed(1);
                document.getElementById('dispenserInterval').innerText = intervalSec;
                
                let slider = document.getElementById('intervalSlider');
                if (!slider.hasAttribute('data-initialized')) {
                    slider.min = data.dispenser.min;
                    slider.max = data.dispenser.max;
                    slider.step = data.dispenser.step;
                    slider.value = intervalSec;
                    document.getElementById('sliderValueDisplay').innerText = intervalSec;
                    slider.setAttribute('data-initialized', 'true');
                }
                
                let toggle = document.getElementById('dispenserToggle');
                let statusText = document.getElementById('dispenserStatusText');
                
                if (document.activeElement !== toggle) {
                    toggle.checked = data.dispenser.enabled;
                }
                
                statusText.innerText = data.dispenser.enabled ? "System On " : "System Off";
                statusText.style.color = data.dispenser.enabled ? "#2ecc71" : "#e74c3c";
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
    dhtSensor.update();
    
    latestReadings.gasRaw = gasSensor.readRawValue();
    latestReadings.gasHazardous = gasSensor.isHazardous();
    latestReadings.soilRaw = soilSensor.readMoistureRaw();
    latestReadings.soilPercent = soilSensor.readMoisturePercentage();
    latestReadings.dispenserInterval = dispenser.getInterval();
    latestReadings.dispenserEnabled = dispenser.getEnabled();
    latestReadings.temperatureCelsius = dhtSensor.getTemperature();
    latestReadings.humidityPercent = dhtSensor.getHumidity();

#if DEBUG
    Serial.printf("Gas: %d %s | Soil: %d%% (%d) | Dispenser: %lu ms (Enabled: %s) | Temp: %.1fC | Hum: %.1f%%\n",
                  latestReadings.gasRaw,
                  (latestReadings.gasHazardous ? "[HAZ]" : "[SAFE]"),
                  latestReadings.soilPercent,
                  latestReadings.soilRaw,
                  latestReadings.dispenserInterval,
                  latestReadings.dispenserEnabled ? "YES" : "NO",
                  latestReadings.temperatureCelsius,
                  latestReadings.humidityPercent);
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
        json += "\"dht\":{\"temperature\":" + String(latestReadings.temperatureCelsius, 1) + ",";
        json += "\"humidity\":" + String(latestReadings.humidityPercent, 1) + "},";
        json += "\"dispenser\":{\"interval\":" + String(latestReadings.dispenserInterval) + ",";
        json += "\"enabled\":" + String(latestReadings.dispenserEnabled ? "true" : "false") + ",";
        json += "\"min\":" + String(dispenserMinSec, 1) + ",";
        json += "\"max\":" + String(dispenserMaxSec, 1) + ",";
        json += "\"step\":" + String(dispenserStepSec, 1) + "}}";
        request->send(200, "application/json", json); });

    server.on("/api/set_interval", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        if (request->hasParam("val", true)) {
            String valStr = request->getParam("val", true)->value();
            unsigned long newInterval = valStr.toInt();
            dispenser.setInterval(newInterval);
            request->send(200, "text/plain", "Interval updated");
        } else {
            request->send(400, "text/plain", "Missing val parameter");
        } });

    server.on("/api/toggle_dispenser", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        if (request->hasParam("enable", true)) {
            bool enable = request->getParam("enable", true)->value() == "1";
            dispenser.setEnabled(enable);
            request->send(200, "text/plain", "Dispenser toggled");
        } else {
            request->send(400, "text/plain", "Missing enable parameter");
        } });
}

void setup()
{
#if DEBUG
    Serial.begin(115200);
#endif

    gasSensor.begin();
    soilSensor.begin();
    dhtSensor.begin();
    
    // Dispenser initialization
    dispenser.begin();

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
    // Update the servo dispenser state non-blocking
    dispenser.update();

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

# ESP32 Drone Sensors Telematics

This project aims to run environmental and physical sensors on an ESP32 module tailored for drone telematics. It offers a modern, asynchronous web interface to view real-time data readings without blocking critical sensor sampling loops or hardware actuators.

## Features

- **Gas Sensor Monitoring**: Reads analog air quality and dynamically triggers a "Hazardous" visual alert on the UI if the configured threshold is exceeded.
- **Soil Moisture Sensor**: Maps analog soil moisture levels to an intuitive percentage layout.
- **Environment Sensor (DHT11)**: Periodically samples ambient temperature (°C) and humidity (%) without blocking the main CPU loop.
- **Seed Dispenser (Servo Controller)**: Non-blocking servo mechanism to open (0°) and close (90°) a payload gate at adjustable frequencies (1-5 seconds), controlled seamlessly from the web dashboard.
- **Asynchronous Web Dashboard**: Embedded HTML UI providing a self-refreshing JSON API using `ESPAsyncWebServer`, fully decoupled from the physical sensor loops.

---

## Hardware Pin Connections

The system is configured to use ESP32's ADC1 pins for analog reading to ensure compatibility even when the WiFi stack is active.

| Component | ESP32 Pin | Type/Notes |
| :--- | :--- | :--- |
| **Gas Sensor** | `GPIO 32` | Analog Input (ADC1) |
| **Soil Moisture Sensor** | `GPIO 33` | Analog Input (ADC1) |
| **DHT11 Sensor** | `GPIO 4` | Digital I/O |
| **Seed Dispenser (Servo)** | `GPIO 18` | PWM Output |

> **⚠️ IMPORTANT HARDWARE WARNING (Brownout):** 
> To avoid ESP32 brownout loops, **DO NOT** power the Servo Motor directly from the ESP32's 5V/3V3 pins. Supplying power via USB is insufficient for the servo's current spike. Always run the servo's `VCC` on a dedicated 5V source (like a Drone BEC) while bridging the `GND` back to the ESP32 logic ground.

---

## Configuration Guide

All primary configurations can be found at the top of the `src/main.cpp` file.

### 1. Changing Wi-Fi Credentials
Provide the network details so that the ESP32 can host the frontend. Open `src/main.cpp` and update the following lines:
```cpp
const char *ssid = "your_network_name";
const char *password = "your_password";
```

### 2. Enabling Debug Mode
If you need to view raw logs and connection statuses via the Serial Monitor (115200 baud), switch the debug definition:
```cpp
// Change 0 to 1 to enable Serial prints
#define DEBUG 1 
```

### 3. Modifying the Gas Sensor Threshold
The gas sensor triggers a hazardous state when the raw ADC value surpasses this number. 
```cpp
// Adjust down for higher sensitivity, or up for lower sensitivity
uint16_t currentGasThreshold = 700; 
```

### 4. Customizing Dispenser Limits
You can modify the bounds of the web interface's seed dispenser slider by changing these variables in `main.cpp`:
```cpp
float dispenserMinSec = 1.0;  // Minimum slider value (seconds)
float dispenserMaxSec = 5.0;  // Maximum slider value (seconds)
float dispenserStepSec = 0.5; // Step intervals on the slider
```

---

## Building and Uploading

This project is built using [PlatformIO](https://platformio.org/).

1. Open the project folder in VS Code with the PlatformIO extension installed.
2. Build the project to resolve dependencies (Wait for `ESPAsyncWebServer` and `ESP32Servo` to download).
3. Connect your ESP32 and click the **Upload** button, or run the following command in your terminal:
   ```bash
   pio run -t upload
   ```
4. Once flashed, (if Debug is enabled) check the Serial Monitor for the assigned `IP Address`, then type that IP into any local web browser (e.g., your smartphone).

## Project Structure & Modularity

The codebase is engineered with strict C++ object-oriented principles. All hardware modules are encapsulated to prevent blocking operations:
* `lib/GasSensor/`: ADC wrapper and threshold logic.
* `lib/SoilSensor/`: Raw mapping logic for moisture content.
* `lib/SeedDispenser/`: Non-blocking `ESP32Servo` interval timer.
* `lib/DHTSensor/`: Non-blocking DHT11 environment parsing.
* `lib/MotorControl/`: Legacy FlySky hardware interrupt structures (currently inactive in the main loop).

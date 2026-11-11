# Smart-Traffic_Light_System

## About the Project

This project is a smart traffic light system built using an ESP32. The idea is to make traffic monitoring more organized by controlling two traffic lights, detecting vehicles using ultrasonic sensors, and monitoring possible red-light violations.

The system also includes countdown displays, OLED screens showing traffic information, and a web dashboard that can be accessed from a mobile phone through the ESP32's Wi-Fi access point.

## Features

- **Two Traffic Lights:** Controls two traffic directions with alternating green, yellow, and red signals.
- **Automatic Traffic Control:** Uses predefined timing for each traffic light phase.
- **Vehicle Detection:** Uses two ultrasonic sensors to detect nearby vehicles.
- **Red-Light Violation Detection:** Detects vehicles entering the sensor's detection range while their traffic light is red.
- **Countdown Displays:** TM1650 7-segment displays show the remaining time for each traffic light.
- **OLED Displays:** Two OLED screens show traffic status, countdown timers, and pedestrian indicators.
- **Mobile Dashboard:** Provides a live dashboard that displays traffic light statuses, countdowns, sensor readings, and alerts.
- **Phone Notifications:** Supports browser notifications and vibration when a new violation is detected.

## Components Used

- ESP32 development board
- 6 LEDs (2 red, 2 yellow, 2 green)
- 2 HC-SR04 ultrasonic sensors
- 2 SH1106 OLED displays (128x64)
- 2 TM1650 4-digit 7-segment displays
- Breadboards and jumper wires
- Resistors and other required wiring components
- USB power supply

## Hardware Connections

### Traffic Light LEDs

| Component | ESP32 GPIO |
|---|---:|
| Traffic 1 Red | 25 |
| Traffic 1 Yellow | 26 |
| Traffic 1 Green | 27 |
| Traffic 2 Red | 18 |
| Traffic 2 Yellow | 19 |
| Traffic 2 Green | 23 |

### Ultrasonic Sensors

| Sensor | Trigger | Echo |
|---|---:|---:|
| Traffic 1 | GPIO 16 | GPIO 35 |
| Traffic 2 | GPIO 17 | GPIO 34 |

### OLED Displays

| Display | SDA | SCL |
|---|---:|---:|
| OLED 1 (Hardware I2C) | GPIO 21 | GPIO 22 |
| OLED 2 (Software I2C) | GPIO 13 | GPIO 14 |

### TM1650 Displays

| Display | SDA | SCL |
|---|---:|---:|
| Display 1 | GPIO 21 | GPIO 22 |
| Display 2 | GPIO 32 | GPIO 33 |

**Important:** The ESP32 uses 3.3V GPIO logic. Make sure the ultrasonic sensor Echo outputs are reduced to a safe 3.3V level before connecting them to the ESP32.

## Technologies and Libraries

- Arduino IDE
- C++ (Arduino)
- ESP32 Wi-Fi
- WebServer
- U8g2 graphics library
- Wire (I2C communication)
- HTML, CSS, and JavaScript

## How It Works

The system operates through four traffic phases:

1. Traffic 1 Green – Traffic 2 Red.
2. Traffic 1 Yellow – Traffic 2 Red.
3. Traffic 1 Red – Traffic 2 Green.
4. Traffic 1 Red – Traffic 2 Yellow.

Each phase runs for a predefined duration. The ultrasonic sensors continuously measure the distance of nearby objects. If a vehicle is detected within 20 cm while its corresponding traffic light is red, the system generates a violation alert.

The OLED and 7-segment displays update the traffic information and remaining time. At the same time, the ESP32 hosts a web dashboard that allows users to monitor the system from their phones.

## Getting Started

### Requirements

1. Install the Arduino IDE.
2. Install the ESP32 board package.
3. Install the U8g2 library from the Arduino Library Manager.
4. Connect all components according to the pin configuration in the source code.

### Running the Project

1. Open the Arduino code in the Arduino IDE.
2. Select your ESP32 board and the correct COM port.
3. Upload the code to the ESP32.
4. Open the Serial Monitor at **115200 baud**.
5. Connect your phone to the ESP32 Wi-Fi network.
6. Open the IP address displayed in the Serial Monitor in your phone's browser.
7. The dashboard will display the current traffic information and alerts.

### Wi-Fi Configuration

The default Wi-Fi settings in the code are:

- Network name: `ESP32_MUSIC`
- Password: `12345678`

You can change these values in the Wi-Fi configuration section of the source code.

**Note:** Change the default password before publishing or deploying the project.

## Future Improvements

- Add a camera for more accurate vehicle detection.
- Improve violation detection by identifying actual stop-line crossings.
- Add vehicle counting and traffic density analysis.
- Introduce adaptive traffic timing based on traffic conditions.
- Store violation records for later analysis.

## Project Status

Developed as a university engineering project.



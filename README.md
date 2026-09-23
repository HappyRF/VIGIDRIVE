# Team Details

### Team Name: SyncTech

### Team Lead : Monica G

### Team Members :
- Shruthi B
- Kothaiarasi M
- Rifaath Fathimah S

### College: Sri Sairam Engineering College

# 🚗 VigiDrive — Intelligent Accident Detection & Emergency Alert System

VigiDrive is an **ESP32-based intelligent accident detection and emergency alert system** designed to detect abnormal vehicle impacts, provide a short cancellation window for false alarms, determine the vehicle's location using GPS, and notify emergency contacts through Telegram.

The project combines **embedded sensing, edge-based accident detection, GPS tracking, wireless communication, and a separate web dashboard** for real-time monitoring.

---

## 🎯 Problem Statement

Road accidents can become more dangerous when emergency assistance is delayed, especially when the driver or passengers are unable to manually request help.

Traditional systems may depend entirely on manual reporting or a single sensor threshold, which can result in delayed alerts or false positives.

VigiDrive addresses this by continuously monitoring vehicle motion and providing an automated accident-alert workflow.

---

## 💡 Proposed Solution

VigiDrive uses an **ESP32** as the main processing unit.

The system continuously monitors acceleration using an **MPU6050 accelerometer/gyroscope**. When a significant impact is detected:

1. The system identifies a possible accident.
2. A buzzer is activated.
3. A warning period is provided to allow the user to cancel a false alarm.
4. If the warning is not cancelled, the system attempts to obtain the vehicle's GPS location.
5. An emergency notification is sent through Telegram.
6. The vehicle location can be viewed using a Google Maps link.
7. The separate VigiDrive web dashboard can monitor available system information through the ESP32's USB serial connection.

---

# 🏗️ System Architecture

```text
                    ┌──────────────────────┐
                    │      VEHICLE         │
                    │                      │
                    │  MPU6050             │
                    │  GPS Module          │
                    │  Buzzer              │
                    │  Reset/Cancel Button │
                    └──────────┬───────────┘
                               │
                               ▼
                    ┌──────────────────────┐
                    │        ESP32         │
                    │                      │
                    │ Accident Detection   │
                    │ Warning Management   │
                    │ GPS Processing       │
                    │ Communication        │
                    └───────┬───────┬──────┘
                            │       │
                  Wi-Fi     │       │ USB Serial
                            │       │
                            ▼       ▼
                     ┌──────────┐  ┌──────────────┐
                     │ Telegram │  │ Python Bridge│
                     │  Alert   │  │  / Flask     │
                     └──────────┘  └───────┬──────┘
                                           │
                                           ▼
                                  ┌────────────────┐
                                  │ VigiDrive Web  │
                                  │   Dashboard    │
                                  └────────────────┘
```

---

# 🔄 Working Principle

### 1. Sensor Monitoring

The ESP32 continuously reads acceleration data from the MPU6050.

The acceleration values are combined to calculate the total acceleration:

```text
Total G = √(Ax² + Ay² + Az²)
```

The current prototype uses a predefined impact threshold to identify a possible accident.

### 2. Accident Detection

When the measured acceleration crosses the configured threshold, the system enters the accident-warning state.

### 3. Warning Period

The buzzer is activated and the user is given a short period to cancel the alert.

This helps reduce false emergency notifications caused by sudden movements or non-accident impacts.

### 4. Accident Confirmation

If the cancellation button is not pressed within the warning period, the system treats the event as a confirmed accident.

### 5. GPS Location

The ESP32 communicates with the GPS module to obtain available location information, including:

* Latitude
* Longitude
* Number of satellites
* Altitude
* Speed

### 6. Emergency Notification

The system sends an emergency notification through Telegram when Wi-Fi connectivity is available.

The notification can contain the accident status and available location information.

### 7. Dashboard Monitoring

The ESP32 is connected to the computer through its USB-to-UART interface.

```text
ESP32
   ↓
CP2102 USB-to-UART
   ↓
COM6
   ↓
Python Serial Bridge
   ↓
Flask
   ↓
Web Dashboard
```

The dashboard operates independently from the embedded firmware.

---

# 🔌 Hardware Components

| Component    | Purpose                                |
| ------------ | -------------------------------------- |
| ESP32        | Main processing and communication unit |
| MPU6050      | Motion and acceleration sensing        |
| NEO-6M GPS   | Vehicle location tracking              |
| Buzzer       | Accident warning indication            |
| Push Button  | Accident cancellation                  |
| 3.7V Battery | Power source                           |
| TP4056       | Battery charging                       |
| MT3608       | Voltage boosting                       |

---

# 📌 ESP32 Pin Configuration

| Component           | ESP32 Pin |
| ------------------- | --------- |
| MPU6050 SDA         | GPIO 21   |
| MPU6050 SCL         | GPIO 22   |
| MPU6050 I²C Address | 0x68      |
| Buzzer              | GPIO 25   |
| Reset/Cancel Button | GPIO 27   |
| GPS TX              | GPIO 16   |
| GPS RX              | GPIO 17   |

---

# 💻 Software Components

### Embedded System

The existing ESP32 firmware handles:

* MPU6050 sensor monitoring
* Accident detection
* Warning countdown
* Accident cancellation
* GPS data processing
* Wi-Fi connection
* Telegram notifications
* Serial diagnostic output
* ESP32-hosted status page

### Dashboard

The separate dashboard uses:

* Python
* Flask
* PySerial
* HTML
* CSS
* JavaScript

The dashboard reads information exposed by the **already-programmed ESP32 through USB serial communication**.

> **Important:** The dashboard does not compile, upload, modify, or execute the ESP32 firmware.

---

# 📊 VigiDrive Dashboard

The dashboard provides a separate interface for monitoring the system.

Depending on the information available through the ESP32's serial output, the dashboard can display:

* 🚨 Accident status
* 💥 Impact / acceleration information
* 🔢 Accident count
* ⏱️ Warning countdown
* 📍 GPS latitude and longitude
* 🛰️ GPS satellite information
* 📶 Wi-Fi status
* 📡 GPS status
* 🔧 MPU6050 status
* 📲 Telegram status
* 📝 System event log
* 🗺️ Google Maps location
* 📈 Available live sensor information

The dashboard only displays information that is actually available from the existing firmware output.

---

# 📁 Project Structure

```text
VigiDrive/
│
├── embedded/
│   ├── VigiDrive_ESP32.ino
│   └── README.md
│
├── dashboard/
│   ├── app.py
│   ├── requirements.txt
│   ├── README.md
│   │
│   ├── templates/
│   │   └── index.html
│   │
│   └── static/
│       ├── style.css
│       └── script.js
│
├── presentation/
│   └── VigiDrive_Presentation.pptx
│
├── demo/
│   └── VigiDrive_Demo.pdf
│
├── README.md
│
└── .gitignore
```

---

# 🔗 Communication Architecture

VigiDrive uses two independent communication paths.

### Emergency Alert Path

```text
Sensors
   ↓
ESP32
   ↓
Accident Detection
   ↓
GPS
   ↓
Wi-Fi
   ↓
Telegram
   ↓
Emergency Notification
```

### Dashboard Path

```text
ESP32
   ↓
USB Serial
   ↓
CP2102
   ↓
COM Port
   ↓
Python
   ↓
Flask
   ↓
Web Dashboard
```

These two paths are independent.

The dashboard does not control or modify the embedded firmware.

---

# ⚙️ Dashboard Setup

## Requirements

Install:

* Python 3
* ESP32 with the existing firmware
* USB data cable
* CP210x USB-to-UART driver
* Required Python packages

Install the Python dependencies:

```bash
pip install -r requirements.txt
```

## Connect the ESP32

Connect the ESP32 to the computer using the USB cable.

Verify that Windows detects the CP210x USB-to-UART interface under:

```text
Device Manager
    ↓
Ports (COM & LPT)
```

Example:

```text
Silicon Labs CP210x USB to UART Bridge (COM6)
```

The COM number may be different on another computer.

## Run the Dashboard

From the `dashboard` directory:

```bash
python app.py
```

The Flask server will start locally.

Open the local address displayed in the terminal using a web browser.

---

# 🔧 Serial Communication

The current ESP32 firmware uses:

```text
Baud Rate: 115200
```

The dashboard communicates with the ESP32 through the USB serial interface.

### Important

Only one application should use the COM port at a time.

For example, if a serial monitor is already connected to `COM6`, the Python dashboard may not be able to access it.

---

# 🧪 Demonstration Workflow

A typical demonstration can follow this sequence:

```text
1. Power ON VigiDrive
        ↓
2. ESP32 initializes sensors
        ↓
3. GPS and Wi-Fi status are checked
        ↓
4. Dashboard connects through USB
        ↓
5. System continuously monitors acceleration
        ↓
6. Abnormal impact is detected
        ↓
7. Buzzer and warning state activate
        ↓
8. User can cancel the warning
        ↓
9. If not cancelled → accident confirmed
        ↓
10. GPS location is obtained
        ↓
11. Telegram emergency alert is sent
        ↓
12. Dashboard displays available system information
```

---

# 🚀 Future Scope

The current prototype can be extended in several ways:

### Multi-Sensor Accident Verification

Instead of relying primarily on a single acceleration threshold, multiple sensor inputs and AI-based verification can be combined to improve accident-event classification.

### False Positive / False Negative Detection

An intelligent classification algorithm can be developed to identify:

* False positives
* False negatives
* Genuine accident events
* Non-accident high-impact events

### Improved Location Reliability

The GPS subsystem can be enhanced using improved GNSS hardware and alternative location mechanisms for situations where GPS signals are weak or unavailable.

### Communication Fallback

Additional communication mechanisms can be incorporated to reduce dependence on a single internet connection.

### Automotive-Grade Hardware

The prototype hardware can eventually be replaced with automotive-grade electronics, sensors, power-management systems, and protective enclosures.

### Emergency-Response Integration

A secure authorized backend could be integrated with emergency-response systems to enable automated and verified incident reporting.

### Intelligent Accident Severity Estimation

Machine-learning models could estimate accident severity using multiple sensor parameters instead of relying only on a fixed threshold.

---

# ⚠️ Current Limitations

* The current prototype uses a predefined acceleration threshold for initial accident detection.
* GPS accuracy depends on satellite availability and environmental conditions.
* Internet connectivity is required for Telegram-based alerts.
* The prototype uses development-board-level hardware rather than automotive-grade electronics.
* The dashboard depends on the information exposed through the existing ESP32 serial output.
* Emergency-service integration is not currently implemented.
* Sensor noise and sudden non-accident impacts can potentially produce false detections.

---

# 🔐 Security Note

The original embedded firmware uses network and Telegram configuration values.

**Do not publish real Wi-Fi passwords, Telegram bot tokens, chat IDs, API keys, or other credentials in a public GitHub repository.**

Before making the repository public:

* Remove sensitive credentials from the GitHub copy.
* Use placeholders where necessary.
* Regenerate exposed Telegram credentials if they were previously shared publicly.
* Never commit `.env` files containing secrets.

The `.gitignore` file should be used to prevent accidental inclusion of sensitive files.

---

# 📚 Documentation

The repository includes additional project documentation:

### 📊 Project Presentation

The `presentation/` folder contains the project presentation:

```text
presentation/VigiDrive_Presentation.pptx
```

### 📕 Project Demonstration

The `demo/` folder contains the project demonstration/documentation PDF:

```text
demo/VigiDrive_Demo.pdf
```

### 🔧 Embedded Firmware

The `embedded/` folder contains the ESP32 firmware used as the hardware-side program.

### 🌐 Dashboard

The `dashboard/` folder contains the independent web dashboard implementation.

---

# 🎯 Project Objectives

VigiDrive aims to:

* Detect potential vehicle accidents automatically.
* Reduce delays in emergency notification.
* Provide a cancellation mechanism for false alarms.
* Obtain accident location using GPS.
* Send automated emergency notifications.
* Provide a separate real-time monitoring dashboard.
* Demonstrate an integrated IoT and edge-computing solution.
* Provide a foundation for future AI-based accident verification.

---

# 👥 Project Team

**Project:** VigiDrive — Intelligent Accident Detection & Emergency Alert System

**Domain:** IoT / Embedded Systems / Web Technologies / Intelligent Transportation

---

# 📜 Note

VigiDrive is a prototype developed for academic and demonstration purposes.

The current system should not be considered a certified automotive safety or emergency-response product. Further validation, safety testing, automotive-grade hardware, communication redundancy, and authorized emergency-service integration would be required before real-world deployment.

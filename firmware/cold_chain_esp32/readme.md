# ESP32 Cold Chain Firmware

## 1. Project Overview

This firmware uses an ESP32 with three sensor modules:

- MFRC522 RFID reader
- DHT11 temperature and humidity sensor
- BMP180 temperature and pressure sensor

The ESP32 uses FreeRTOS to separate the work between its two CPU cores.

```text
ESP32
│
├── Core 0
│   └── Sensor Task
│       ├── RFID
│       ├── DHT11
│       └── BMP180
│
└── Core 1
    └── Network Task
        └── Reads SensorData
            └── Prints data to Serial
```

The current firmware does **not** implement MQTT, HTTP, Wi-Fi, or backend communication.

---

# 2. FreeRTOS Architecture

## Core 0 — Sensor Task

Core 0 is responsible for reading all sensors.

The main task is:

```cpp
sensorTask()
```

It runs on:

```text
Core 0
```

The sensor task calls:

```cpp
readSensors()
```

which handles:

```cpp
readRFID()
readDHT()
readBMP()
```

The sensor task updates the shared:

```cpp
SensorData
```

structure.

### Core 0 responsibilities

```text
Read RFID
Read DHT11
Read BMP180
Update SensorData
Protect shared data with mutex
```

No network communication is performed inside the sensor functions.

---

# 3. Core 1 — Network Task

Core 1 runs:

```cpp
networkTask()
```

The current version does not communicate with a server.

Instead, Core 1:

1. Reads the latest sensor data.
2. Copies the data safely using the mutex.
3. Prints the data to Serial.

```text
Core 0
Sensor Task
     │
     │ SensorData
     ↓
Shared Data
     │
     │ mutex
     ↓
Core 1
Network Task
     │
     ↓
Serial Monitor
```

MQTT can be added to Core 1 later without changing the sensor functions.

---

# 4. Shared Sensor Data

The ESP32 uses:

```cpp
struct SensorData
```

The structure currently contains:

```text
RFID UID
Card Type
DHT Temperature
Humidity
BMP Temperature
Pressure
```

Example:

```text
RFID UID       → A3:7B:91:2F
Card Type      → MIFARE 1KB
DHT Temperature→ 5.4 °C
Humidity       → 62.0 %
BMP Temperature→ 5.8 °C
Pressure       → 101325 Pa
```

Core 0 writes the sensor data.

Core 1 reads a copy of the latest data.

A mutex called:

```cpp
dataMutex
```

protects the shared data.

---

# 5. Hardware

## Main Controller

```text
ESP32
```

The ESP32 provides:

- GPIO
- SPI
- I2C
- FreeRTOS
- Dual-core processing

---

# 6. MFRC522 RFID Reader

The RFID reader communicates with the ESP32 using SPI.

### Pin Connections

| MFRC522 | ESP32 |
|---|---:|
| SDA / SS | GPIO 5 |
| RST | GPIO 27 |
| SCK | GPIO 18 |
| MISO | GPIO 19 |
| MOSI | GPIO 23 |
| 3.3V | 3.3V |
| GND | GND |
| IRQ | Not connected |

### Voltage

```text
VCC: 3.3V
Logic: 3.3V
```

The MFRC522 is connected to the ESP32's 3.3V supply.

### RFID Data

When a tag is detected, the ESP32 reads:

```text
UID
Card Type
```

Example:

```text
RFID UID: A3:7B:91:2F
Card Type: MIFARE 1KB
```

---

# 7. DHT11

The DHT11 provides:

```text
Temperature
Humidity
```

### Pin Connections

| DHT11 | ESP32 |
|---|---:|
| DATA | GPIO 26 |
| VCC | 3.3V |
| GND | GND |

### Voltage

```text
VCC: 3.3V
```

### DHT11 Data

Example:

```text
Temperature: 5.4 °C
Humidity: 62.0 %
```

The DHT11 temperature is currently the main temperature value used in the shared sensor data.

---

# 8. BMP180

The BMP180 communicates with the ESP32 using I2C.

### Pin Connections

| BMP180 | ESP32 |
|---|---:|
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| VCC | 3.3V |
| GND | GND |

### Voltage

```text
VCC: 3.3V
```

### BMP180 Data

The BMP180 provides:

```text
Temperature
Pressure
```

Example:

```text
BMP Temperature: 5.8 °C
Pressure: 101325 Pa
```

Pressure can also be displayed as:

```text
1013.25 hPa
```

---

# 9. Complete Pin Table

| Component | Pin | ESP32 |
|---|---|---:|
| MFRC522 | SDA / SS | GPIO 5 |
| MFRC522 | RST | GPIO 27 |
| MFRC522 | SCK | GPIO 18 |
| MFRC522 | MISO | GPIO 19 |
| MFRC522 | MOSI | GPIO 23 |
| MFRC522 | VCC | 3.3V |
| MFRC522 | GND | GND |
| DHT11 | DATA | GPIO 26 |
| DHT11 | VCC | 3.3V |
| DHT11 | GND | GND |
| BMP180 | SDA | GPIO 21 |
| BMP180 | SCL | GPIO 22 |
| BMP180 | VCC | 3.3V |
| BMP180 | GND | GND |

---

# 10. Communication Interfaces

The project currently uses two hardware communication interfaces.

## SPI

Used by:

```text
MFRC522
```

ESP32 SPI pins:

```text
SCK  → GPIO 18
MISO → GPIO 19
MOSI → GPIO 23
SS   → GPIO 5
```

RFID reset:

```text
RST → GPIO 27
```

## I2C

Used by:

```text
BMP180
```

ESP32 I2C pins:

```text
SDA → GPIO 21
SCL → GPIO 22
```

---

# 11. Sensor Data Flow

The complete current data flow is:

```text
                 ESP32
                   │
          ┌────────┴────────┐
          │                 │
       Core 0            Core 1
          │                 │
    Sensor Task        Network Task
          │                 │
     ┌────┼────┐            │
     │    │    │            │
   RFID DHT  BMP            │
     │    │    │            │
     └────┼────┘            │
          │                 │
          ↓                 │
     SensorData ────────────┘
                            │
                            ↓
                       Serial Monitor
```

---

# 12. Current Serial Output

The current ESP32 does not send backend JSON.

The current Core 1 output looks like:

```text
NETWORK TASK
Core: 1
RFID: A3:7B:91:2F
Temperature: 5.4
Humidity: 62.0
BMP Temperature: 5.8
Pressure: 101325
```

This means Core 1 is successfully receiving the latest sensor information from Core 0.

---

# 13. Current ESP32 Data

The current firmware produces the following sensor information:

| Data | Source | Current Status |
|---|---|---|
| RFID UID | MFRC522 | Available |
| Card Type | MFRC522 | Available |
| Temperature | DHT11 | Available |
| Humidity | DHT11 | Available |
| BMP Temperature | BMP180 | Available |
| Pressure | BMP180 | Available |

Example complete data:

```text
RFID UID: A3:7B:91:2F
Card Type: MIFARE 1KB

DHT11:
Temperature: 5.4 °C
Humidity: 62.0 %

BMP180:
Temperature: 5.8 °C
Pressure: 101325 Pa
```

---

# 14. Core Responsibilities

## Core 0

```text
Core 0
│
└── Sensor Task
    │
    ├── readRFID()
    ├── readDHT()
    ├── readBMP()
    │
    └── Update SensorData
```

Core 0 is responsible only for sensor acquisition.

## Core 1

```text
Core 1
│
└── Network Task
    │
    ├── Read SensorData
    └── Print data to Serial
```

Core 1 is responsible for processing the shared data and will later handle network communication.

---

# 15. Important Separation Rule

Keep the sensor and network responsibilities separate.

Do not put network communication inside:

```cpp
readRFID()
readDHT()
readBMP()
```

The sensor functions should only read sensors and update the sensor data.

The network task should handle communication later.

Correct architecture:

```text
Core 0
│
├── RFID
├── DHT11
├── BMP180
│
└── SensorData
       │
       ↓
     Mutex
       │
       ↓
Core 1
│
└── Network Task
```

---

# 16. Current Status

### Implemented

```text
✓ ESP32
✓ FreeRTOS
✓ Core 0 Sensor Task
✓ Core 1 Network Task
✓ MFRC522 RFID
✓ DHT11
✓ BMP180
✓ SPI
✓ I2C
✓ Shared SensorData
✓ Mutex protection
✓ Serial output
```

### Not Implemented

```text
✗ Wi-Fi
✗ MQTT
✗ HTTP
✗ HTTPS
✗ JSON transmission
✗ Battery measurement
✗ Local Flash buffering
✗ Backend communication
```

---

# 17. Current Firmware Architecture

```text
                         ESP32
                           │
            ┌──────────────┴──────────────┐
            │                             │
         CORE 0                        CORE 1
            │                             │
      Sensor Task                    Network Task
            │                             │
     ┌──────┼──────┐                      │
     │      │      │                      │
   RFID   DHT11  BMP180                    │
     │      │      │                      │
     └──────┼──────┘                      │
            │                             │
            ↓                             │
       SensorData                          │
            │                             │
            └────────── Mutex ────────────┘
                                          │
                                          ↓
                                    Serial Output
```

The current goal is to keep the ESP32 sensor system stable and clearly separated between Core 0 and Core 1 before adding network communication.# ESP32 Cold Chain Firmware

## 1. Project Overview

This firmware uses an ESP32 with three sensor modules:

- MFRC522 RFID reader
- DHT11 temperature and humidity sensor
- BMP180 temperature and pressure sensor

The ESP32 uses FreeRTOS to separate the work between its two CPU cores.

```text
ESP32
│
├── Core 0
│   └── Sensor Task
│       ├── RFID
│       ├── DHT11
│       └── BMP180
│
└── Core 1
    └── Network Task
        └── Reads SensorData
            └── Prints data to Serial
```

The current firmware does **not** implement MQTT, HTTP, Wi-Fi, or backend communication.

---

# 2. FreeRTOS Architecture

## Core 0 — Sensor Task

Core 0 is responsible for reading all sensors.

The main task is:

```cpp
sensorTask()
```

It runs on:

```text
Core 0
```

The sensor task calls:

```cpp
readSensors()
```

which handles:

```cpp
readRFID()
readDHT()
readBMP()
```

The sensor task updates the shared:

```cpp
SensorData
```

structure.

### Core 0 responsibilities

```text
Read RFID
Read DHT11
Read BMP180
Update SensorData
Protect shared data with mutex
```

No network communication is performed inside the sensor functions.

---

# 3. Core 1 — Network Task

Core 1 runs:

```cpp
networkTask()
```

The current version does not communicate with a server.

Instead, Core 1:

1. Reads the latest sensor data.
2. Copies the data safely using the mutex.
3. Prints the data to Serial.

```text
Core 0
Sensor Task
     │
     │ SensorData
     ↓
Shared Data
     │
     │ mutex
     ↓
Core 1
Network Task
     │
     ↓
Serial Monitor
```

MQTT can be added to Core 1 later without changing the sensor functions.

---

# 4. Shared Sensor Data

The ESP32 uses:

```cpp
struct SensorData
```

The structure currently contains:

```text
RFID UID
Card Type
DHT Temperature
Humidity
BMP Temperature
Pressure
```

Example:

```text
RFID UID       → A3:7B:91:2F
Card Type      → MIFARE 1KB
DHT Temperature→ 5.4 °C
Humidity       → 62.0 %
BMP Temperature→ 5.8 °C
Pressure       → 101325 Pa
```

Core 0 writes the sensor data.

Core 1 reads a copy of the latest data.

A mutex called:

```cpp
dataMutex
```

protects the shared data.

---

# 5. Hardware

## Main Controller

```text
ESP32
```

The ESP32 provides:

- GPIO
- SPI
- I2C
- FreeRTOS
- Dual-core processing

---

# 6. MFRC522 RFID Reader

The RFID reader communicates with the ESP32 using SPI.

### Pin Connections

| MFRC522 | ESP32 |
|---|---:|
| SDA / SS | GPIO 5 |
| RST | GPIO 27 |
| SCK | GPIO 18 |
| MISO | GPIO 19 |
| MOSI | GPIO 23 |
| 3.3V | 3.3V |
| GND | GND |
| IRQ | Not connected |

### Voltage

```text
VCC: 3.3V
Logic: 3.3V
```

The MFRC522 is connected to the ESP32's 3.3V supply.

### RFID Data

When a tag is detected, the ESP32 reads:

```text
UID
Card Type
```

Example:

```text
RFID UID: A3:7B:91:2F
Card Type: MIFARE 1KB
```

---

# 7. DHT11

The DHT11 provides:

```text
Temperature
Humidity
```

### Pin Connections

| DHT11 | ESP32 |
|---|---:|
| DATA | GPIO 26 |
| VCC | 3.3V |
| GND | GND |

### Voltage

```text
VCC: 3.3V
```

### DHT11 Data

Example:

```text
Temperature: 5.4 °C
Humidity: 62.0 %
```

The DHT11 temperature is currently the main temperature value used in the shared sensor data.

---

# 8. BMP180

The BMP180 communicates with the ESP32 using I2C.

### Pin Connections

| BMP180 | ESP32 |
|---|---:|
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| VCC | 3.3V |
| GND | GND |

### Voltage

```text
VCC: 3.3V
```

### BMP180 Data

The BMP180 provides:

```text
Temperature
Pressure
```

Example:

```text
BMP Temperature: 5.8 °C
Pressure: 101325 Pa
```

Pressure can also be displayed as:

```text
1013.25 hPa
```

---

# 9. Complete Pin Table

| Component | Pin | ESP32 |
|---|---|---:|
| MFRC522 | SDA / SS | GPIO 5 |
| MFRC522 | RST | GPIO 27 |
| MFRC522 | SCK | GPIO 18 |
| MFRC522 | MISO | GPIO 19 |
| MFRC522 | MOSI | GPIO 23 |
| MFRC522 | VCC | 3.3V |
| MFRC522 | GND | GND |
| DHT11 | DATA | GPIO 26 |
| DHT11 | VCC | 3.3V |
| DHT11 | GND | GND |
| BMP180 | SDA | GPIO 21 |
| BMP180 | SCL | GPIO 22 |
| BMP180 | VCC | 3.3V |
| BMP180 | GND | GND |

---

# 10. Communication Interfaces

The project currently uses two hardware communication interfaces.

## SPI

Used by:

```text
MFRC522
```

ESP32 SPI pins:

```text
SCK  → GPIO 18
MISO → GPIO 19
MOSI → GPIO 23
SS   → GPIO 5
```

RFID reset:

```text
RST → GPIO 27
```

## I2C

Used by:

```text
BMP180
```

ESP32 I2C pins:

```text
SDA → GPIO 21
SCL → GPIO 22
```

---

# 11. Sensor Data Flow

The complete current data flow is:

```text
                 ESP32
                   │
          ┌────────┴────────┐
          │                 │
       Core 0            Core 1
          │                 │
    Sensor Task        Network Task
          │                 │
     ┌────┼────┐            │
     │    │    │            │
   RFID DHT  BMP            │
     │    │    │            │
     └────┼────┘            │
          │                 │
          ↓                 │
     SensorData ────────────┘
                            │
                            ↓
                       Serial Monitor
```

---

# 12. Current Serial Output

The current ESP32 does not send backend JSON.

The current Core 1 output looks like:

```text
NETWORK TASK
Core: 1
RFID: A3:7B:91:2F
Temperature: 5.4
Humidity: 62.0
BMP Temperature: 5.8
Pressure: 101325
```

This means Core 1 is successfully receiving the latest sensor information from Core 0.

---

# 13. Current ESP32 Data

The current firmware produces the following sensor information:

| Data | Source | Current Status |
|---|---|---|
| RFID UID | MFRC522 | Available |
| Card Type | MFRC522 | Available |
| Temperature | DHT11 | Available |
| Humidity | DHT11 | Available |
| BMP Temperature | BMP180 | Available |
| Pressure | BMP180 | Available |

Example complete data:

```text
RFID UID: A3:7B:91:2F
Card Type: MIFARE 1KB

DHT11:
Temperature: 5.4 °C
Humidity: 62.0 %

BMP180:
Temperature: 5.8 °C
Pressure: 101325 Pa
```

---

# 14. Core Responsibilities

## Core 0

```text
Core 0
│
└── Sensor Task
    │
    ├── readRFID()
    ├── readDHT()
    ├── readBMP()
    │
    └── Update SensorData
```

Core 0 is responsible only for sensor acquisition.

## Core 1

```text
Core 1
│
└── Network Task
    │
    ├── Read SensorData
    └── Print data to Serial
```

Core 1 is responsible for processing the shared data and will later handle network communication.

---

# 15. Important Separation Rule

Keep the sensor and network responsibilities separate.

Do not put network communication inside:

```cpp
readRFID()
readDHT()
readBMP()
```

The sensor functions should only read sensors and update the sensor data.

The network task should handle communication later.

Correct architecture:

```text
Core 0
│
├── RFID
├── DHT11
├── BMP180
│
└── SensorData
       │
       ↓
     Mutex
       │
       ↓
Core 1
│
└── Network Task
```

---

# 16. Current Status

### Implemented

```text
✓ ESP32
✓ FreeRTOS
✓ Core 0 Sensor Task
✓ Core 1 Network Task
✓ MFRC522 RFID
✓ DHT11
✓ BMP180
✓ SPI
✓ I2C
✓ Shared SensorData
✓ Mutex protection
✓ Serial output
```

### Not Implemented

```text
✗ Wi-Fi
✗ MQTT
✗ HTTP
✗ HTTPS
✗ JSON transmission
✗ Battery measurement
✗ Local Flash buffering
✗ Backend communication
```

---

# 17. Current Firmware Architecture

```text
                         ESP32
                           │
            ┌──────────────┴──────────────┐
            │                             │
         CORE 0                        CORE 1
            │                             │
      Sensor Task                    Network Task
            │                             │
     ┌──────┼──────┐                      │
     │      │      │                      │
   RFID   DHT11  BMP180                    │
     │      │      │                      │
     └──────┼──────┘                      │
            │                             │
            ↓                             │
       SensorData                          │
            │                             │
            └────────── Mutex ────────────┘
                                          │
                                          ↓
                                    Serial Output
```

The current goal is to keep the ESP32 sensor system stable and clearly separated between Core 0 and Core 1 before adding network communication.

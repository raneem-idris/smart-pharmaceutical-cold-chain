# ESP32 Firmware

## Purpose

The ESP32 is the edge node for one refrigerator. It samples the environmental sensors, reads the door switch and shelf RFID zones, then sends timestamped telemetry to the backend.

## Hardware

| Component | Quantity | Role |
| --- | ---: | --- |
| ESP32 DevKit | 1 | Sensor controller and network client |
| DHT22 | 1 | Primary temperature and humidity |
| BMP280 or BMP180 | 1 | Pressure and secondary temperature |
| MFRC522 13.56 MHz reader | 2 | One reader per shelf/zone |
| RFID tag | 2 | One tracked product tag per zone |
| Reed switch and magnet | 1 | Refrigerator door state |
| Regulated 5 V supply | 1 | System power; 5 V/2 A minimum starting point, 5 V/3 A recommended for margin |

Breadboard, USB cable, jumper wires, and the physical chamber materials are needed for the prototype. Pin assignments are not specified yet and should be documented after the wiring is finalized.

## RFID Zones

Reader 1 represents shelf 1 and reader 2 represents shelf 2. Each product's tag is associated with its shelf reader. A product is considered removed when its tag is no longer detected by that reader; firmware should poll consistently and avoid treating a brief missed read as a confirmed inventory change.

For readers mounted back-to-back, orient antenna faces toward their respective zones and use a physical separator/gap. Validate reader spacing and cross-reading behavior on the assembled refrigerator because RF coupling depends on the final placement.

## Sampling and Telemetry

The target reading cycle samples DHT22 temperature/humidity, BMP temperature/pressure, door state, and active RFID tags. The system specification targets telemetry every 5-30 seconds; the production interval should be selected and kept consistent with backend and ML window settings.

Example device envelope:

```json
{
  "device_id": "VAULT_FRIDGE_01",
  "timestamp": "2026-10-04T14:29:00Z",
  "connection_type": "MQTT",
  "battery_level": 98,
  "telemetry": {
    "temperature_dht": 5.2,
    "humidity": 48.5,
    "temperature_bmp": 5.4,
    "pressure": 1012.3,
    "door": "closed"
  },
  "rfid": [
    {"reader_id": 1, "tag_uid": "A1B2C3D4", "product": "Vaccine_A"},
    {"reader_id": 2, "tag_uid": "E5F6G7H8", "product": "Vaccine_B"}
  ]
}
```

## Communication and Recovery

1. Publish telemetry through MQTT over TLS as the primary transport (typically TCP port 8883).
2. If publish/acknowledgement fails, POST the telemetry to `https://<backend-host>/api/v1/telemetry/backup` over TLS (TCP port 443), set `connection_type` to `HTTPS`, and treat HTTP `200` or `201` as success.
3. If both transports fail, append the reading to non-volatile LittleFS or SPIFFS storage and retry when connectivity returns. Preserve timestamps and avoid dropping buffered records until delivery is acknowledged.

BLE is reserved in the system design for local diagnostics and field service; its behavior and security are not defined yet.

## Power and Electrical Safety

- Use a regulated 5 V supply with a common ground for the prototype.
- Never apply 5 V directly to ESP32 GPIO pins; ESP32 logic is 3.3 V.
- Check each sensor/reader board's supply and logic-level requirements before wiring it to the ESP32.
- Validate MFRC522 antenna separation and sensor readings in the final enclosure.

## Software Target

The proposed firmware stack is Arduino C/C++ or PlatformIO with MFRC522, DHT, BMP, PubSubClient, HTTPClient, and ArduinoJson libraries. TLS certificate validation and API-key handling must be configured for deployment; do not embed production secrets in source control.

## Current Repository Status

The `cold_chain_esp32.ino` sketch is currently empty. Hardware, payload, and failover behavior in this README describe the target design and are not implemented firmware yet.
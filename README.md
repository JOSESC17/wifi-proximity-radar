# Wi-Fi Proximity Radar (RSSI), Arduino UNO and ESP-01

Learning IoT prototype that estimates the presence/proximity of an obstacle using Wi-Fi signal strength (RSSI) between an Arduino UNO (with an ESP-01 module) and a known access point, with local feedback (LEDs + buzzer) and telemetry to [ThingSpeak](https://thingspeak.com).

**Full technical report:**
- 🇬🇧 English: [`docs/Radar_WiFi_Technical_Report_Salvuchi_EN.docx`](docs/Radar_WiFi_Technical_Report_Salvuchi_EN.docx)
- 🇪🇸 Español: [`docs/Informe_Tecnico_Radar_WiFi_Salvuchi_ES.docx`](docs/Informe_Tecnico_Radar_WiFi_Salvuchi_ES.docx)

See either version for the detailed analysis, methodology, findings, and limitations.

## Summary

The project was originally conceived as a distance estimation system (log-distance path loss model). The experimental process revealed that this model, while mathematically correct, has significant limitations in real indoor environments due to multipath fading and attenuation from occlusion (NLOS). This reoriented the project toward a more robust application: **presence/obstruction event detection through RSSI variation** (device-free sensing), rather than absolute distance measurement.

## Hardware

- Arduino UNO
- ESP-01 module (communication via `SoftwareSerial`, pins 2 and 3)
- MB102 power supply (dedicated power for the ESP-01 and breadboard, separated from the Arduino's USB)
- Voltage divider on the Arduino TX → ESP-01 RX line (5V → 3.3V adaptation)
- 3 LEDs (green, yellow, red) — proximity indicators
- Active buzzer module (3-pin, KY-012 type)

## Key Technical Findings

1. **ESP-01 brownout**: current bursts (~300–400 mA) during transmission caused restarts when sharing a power source. Resolved with a dedicated power supply (MB102) + decoupling.
2. **`AT+CWLAP` limitations**: the blocking scan introduced variable latency (1–3 s), temporal offset, SSID ambiguity, and truncation risk from `SoftwareSerial`'s 64-byte buffer. Proposed and implemented fix: migration to `AT+CWJAP?`.
3. **Unreliable empirical calibration**: attempts to calibrate constants A and n by manually holding the prototype in front of the router introduced the operator as a source of occlusion, invalidating the measurements.

See the full report for details on each finding, quantitative evidence (ThingSpeak dataset), and future work recommendations.

## Code

The complete firmware is in [`src/radar_wifi.ino`](src/radar_wifi.ino), and includes:
- Wi-Fi connection via AT commands
- RSSI query without blocking scan (`AT+CWJAP?`)
- Moving average filter (10 samples)
- Distance estimation (log-distance path loss)
- LED/buzzer control based on proximity
- Telemetry upload to ThingSpeak (instantaneous RSSI, average RSSI, estimated distance)

## Data

The complete test dataset (exported from ThingSpeak) is available in `data/feeds.csv` — timestamp, raw RSSI, filtered RSSI, and estimated distance per reading.

## Videos

- [Prototype walkthrough](https://youtu.be/RH5jOgd25E0)
- [Demo clip 1](https://youtube.com/shorts/PMqQQswal_M)
- [Demo clip 2](https://youtube.com/shorts/fofxZYuFtro)

## Author

José Salvuchi — [LinkedIn](https://www.linkedin.com/in/jose-salvuchi-3a72b1384/)


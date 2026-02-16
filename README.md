# xiaoheartrate — XIAO ESP32-C3 Heart Rate Monitoring System

xiaoheartrate is a real-time heart rate monitoring system built using the Seeed Studio XIAO ESP32-C3 and an analog pulse sensor. It reads pulse signals, calculates Beats Per Minute (BPM), and provides a live wireless dashboard accessible from any browser.

The system runs fully standalone by hosting its own WiFi network and web interface.

This project demonstrates embedded signal processing, heartbeat detection, and real-time wireless monitoring using ESP32.

---

## Overview

The system continuously reads analog signals from the pulse sensor and processes them using filtering and peak detection to identify heartbeats.

Once detected, the ESP32 calculates BPM and streams the data to a live web dashboard hosted directly on the device.

The system provides:

- Real-time BPM calculation  
- Live pulse signal monitoring  
- Wireless web dashboard  
- Standalone operation  
- Real-time signal filtering  

---

## Features

### Real-Time BPM Detection

Heart rate is calculated using time between detected pulse peaks:

```
BPM = 60000 / Beat Interval (milliseconds)
```

This provides continuous heart rate updates.

---

### Signal Processing

The system improves signal reliability using:

- Moving average filtering  
- Dynamic baseline tracking  
- Adaptive threshold detection  
- Noise reduction  

This helps detect actual heartbeats instead of noise.

---

### Wireless Web Dashboard

The XIAO ESP32-C3 creates its own WiFi access point and hosts a live dashboard.

The dashboard displays:

- Current BPM  
- Raw sensor signal value  
- Animated heartbeat indicator  
- Visual feedback based on BPM  

Dashboard refresh rate: 200 ms

No internet required.

---

### Standalone Operation

All processing happens on the ESP32:

- Reads sensor input  
- Filters signal  
- Detects heartbeat  
- Calculates BPM  
- Hosts web server  

No external server required.

---

## Hardware Used

- Seeed Studio XIAO ESP32-C3  
- Analog Pulse Sensor  
- Jumper wires  
- USB power supply  

Optional:

- Battery for portable operation  

---

## System Architecture

**Sensor Layer**  
Pulse sensor generates analog signal from blood flow

**Processing Layer**  
ESP32 filters signal and detects heartbeat peaks

**Calculation Layer**  
ESP32 calculates BPM

**Network Layer**  
ESP32 hosts WiFi access point and web server

**Interface Layer**  
Browser displays live data

---

## Detection Logic

Heartbeat detection process:

1. Read analog signal  
2. Apply smoothing filter  
3. Track baseline signal level  
4. Detect peaks above threshold  
5. Calculate time between peaks  
6. Convert to BPM  

Invalid values outside safe range are ignored.

---

## Dashboard Functions

- Live BPM display  
- Raw signal display  
- Animated heartbeat indicator  
- Real-time updates  

---

## Performance

- Sampling rate: ~200 Hz  
- Dashboard refresh rate: 5 Hz  
- Detection latency: <1 second  
- Standalone operation: Yes  
- Internet required: No  

---

## Applications

- Embedded systems learning  
- Signal processing experiments  
- IoT monitoring systems  
- Wearable prototype development  
- Educational demonstrations  

---

## Notice

This project is for educational purposes only.

It is not intended for medical use or diagnosis.

---

## Technical Concepts Demonstrated

- Embedded programming  
- Analog signal processing  
- Peak detection algorithms  
- ESP32 web server implementation  
- Real-time data streaming  

---

## Future Improvements

- OLED display integration  
- Data logging  
- Battery optimization  
- Mobile app integration  

---

Project by **Hariom Sharnam**

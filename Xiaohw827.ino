#include <WiFi.h>
#include <WebServer.h>

// ===== Settings =====
// NOTE: GPIO 0 is often the Boot button. Ensure your sensor is connected here
// or change this to a standard ADC pin like 32, 33, 34, etc.
#define PULSE_PIN 0  

const char* ssid = "ESP32_HR";
const char* password = "12345678";

// ===== Heart Rate Logic =====
unsigned long lastBeatTime = 0;
int bpm = 0;
int prevSignal = 0;
int rawSignal = 0;

const int DELTA_THRESHOLD = 2;
const int MIN_INTERVAL = 350; // Max 171 BPM

// ===== Web Server =====
WebServer server(80);

// ===== HTML Page (Material You Design) =====
const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Vital Monitor</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link href="https://fonts.googleapis.com/css2?family=Roboto:wght@400;500;700&display=swap" rel="stylesheet">
  <style>
    :root {
      /* Material 3 Dark Tokens */
      --md-sys-color-background: #111318;
      --md-sys-color-on-background: #e2e2e6;
      --md-sys-color-surface-container: #1e1f25;
      --md-sys-color-primary: #aec6ff;
      --md-sys-color-on-primary: #002e69;
      --md-sys-color-secondary-container: #334a92;
      --md-sys-color-on-secondary-container: #dce1ff;
      --md-sys-color-error: #ffb4ab;
      --md-sys-color-outline: #8e9099;
    }

    body {
      background-color: var(--md-sys-color-background);
      color: var(--md-sys-color-on-background);
      font-family: 'Roboto', sans-serif;
      margin: 0;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      height: 100vh;
      transition: background 0.5s ease;
    }

    h2 {
      font-weight: 500;
      font-size: 1.2rem;
      opacity: 0.8;
      margin-bottom: 24px;
      letter-spacing: 0.5px;
    }

    /* Main Sensor Card */
    .card {
      background: var(--md-sys-color-surface-container);
      padding: 40px;
      border-radius: 32px; /* Material You rounded corners */
      width: 85%;
      max-width: 320px;
      text-align: center;
      box-shadow: 0 4px 8px rgba(0,0,0,0.2);
      position: relative;
      overflow: hidden;
    }

    /* The Beating Heart SVG */
    .heart-icon {
      width: 64px;
      height: 64px;
      fill: var(--md-sys-color-primary);
      margin-bottom: 10px;
      transition: transform 0.1s ease; /* Smooth beat return */
    }

    .value-container {
      display: flex;
      justify-content: center;
      align-items: baseline;
      gap: 8px;
    }

    .value {
      font-size: 80px;
      font-weight: 500;
      color: var(--md-sys-color-primary);
      line-height: 1;
    }

    .unit {
      font-size: 24px;
      color: var(--md-sys-color-primary);
      opacity: 0.7;
      font-weight: 500;
    }

    /* Raw Signal Pill */
    .debug-pill {
      margin-top: 30px;
      background: var(--md-sys-color-secondary-container);
      color: var(--md-sys-color-on-secondary-container);
      padding: 8px 16px;
      border-radius: 100px;
      font-size: 14px;
      font-family: monospace;
      display: inline-block;
    }

    /* Footer */
    .footer {
      margin-top: 40px;
      font-size: 12px;
      color: var(--md-sys-color-outline);
      text-align: center;
      max-width: 300px;
      line-height: 1.5;
    }
    
    .warning-icon {
      display: block;
      font-size: 18px;
      margin-bottom: 4px;
    }

    /* Animation Class applied via JS */
    .beat {
      animation: heartbeat 0.2s ease-in-out;
    }

    @keyframes heartbeat {
      0% { transform: scale(1); }
      25% { transform: scale(1.3); }
      50% { transform: scale(1.15); }
      100% { transform: scale(1); }
    }
  </style>
</head>
<body>

  <h2>Heart Rate Monitor</h2>

  <div class="card">
    <svg class="heart-icon" id="heart" viewBox="0 0 24 24">
      <path d="M12 21.35l-1.45-1.32C5.4 15.36 2 12.28 2 8.5 2 5.42 4.42 3 7.5 3c1.74 0 3.41.81 4.5 2.09C13.09 3.81 14.76 3 16.5 3 19.58 3 22 5.42 22 8.5c0 3.78-3.4 6.86-8.55 11.54L12 21.35z"/>
    </svg>

    <div class="value-container">
      <div class="value" id="bpm">--</div>
      <div class="unit">BPM</div>
    </div>

    <div class="debug-pill">
      RAW: <span id="raw">--</span>
    </div>
  </div>

  <div class="footer">
    <span class="warning-icon">⚠️</span>
    <strong>DEMONSTRATION ONLY</strong><br>
    Not intended for medical use.<br>
    Do not rely on this for health diagnostics.
  </div>

<script>
  const heart = document.getElementById('heart');
  const bpmDisplay = document.getElementById('bpm');
  const rawDisplay = document.getElementById('raw');
  
  // Surprise: CSS Variable manipulation for dynamic coloring
  const root = document.documentElement;

  setInterval(() => {
    fetch('/data')
      .then(res => res.json())
      .then(d => {
        bpmDisplay.innerText = d.bpm;
        rawDisplay.innerText = d.raw;

        // SURPRISE LOGIC: 
        // 1. Trigger Animation based on calculated interval
        // We simulate the "beat" visually based on the BPM rate
        if (d.bpm > 0) {
           let beatInterval = 60000 / d.bpm;
           heart.style.animation = `heartbeat ${beatInterval/1000}s infinite`;
           heart.style.animationDuration = (60/d.bpm) + 's';
        } else {
           heart.style.animation = 'none';
        }

        // 2. Dynamic Color Shift (Relaxed Blue vs Intense Red)
        if (d.bpm > 100) {
           // High BPM: Shift to Red/alert tones
           root.style.setProperty('--md-sys-color-primary', '#ffb4ab'); // Reddish
           root.style.setProperty('--md-sys-color-surface-container', '#3c2f2f');
        } else {
           // Normal BPM: Keep Blue/Relaxed tones
           root.style.setProperty('--md-sys-color-primary', '#aec6ff'); // Blueish
           root.style.setProperty('--md-sys-color-surface-container', '#1e1f25');
        }
      });
  }, 200);
</script>
</body>
</html>
)rawliteral";

// ===== Routes =====
void handleRoot() {
  server.send(200, "text/html", PAGE);
}

void handleData() {
  String json = "{";
  json += "\"bpm\":" + String(bpm) + ",";
  json += "\"raw\":" + String(rawSignal);
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  // WiFi AP
  WiFi.softAP(ssid, password);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // Server
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();

  rawSignal = analogRead(PULSE_PIN);
  unsigned long now = millis();

  // Basic edge detection
  int delta = rawSignal - prevSignal;

  if (delta >= DELTA_THRESHOLD) {
    if (now - lastBeatTime > MIN_INTERVAL) {
      if (lastBeatTime != 0) {
        bpm = 60000 / (now - lastBeatTime);
      }
      lastBeatTime = now;
    }
  }

  prevSignal = rawSignal;
  delay(5); // Small delay to stabilize loop
}

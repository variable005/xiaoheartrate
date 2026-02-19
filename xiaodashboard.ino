#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

// ================== CONFIG ==================
#define PULSE_PIN 2   // D0 / A0 on XIAO ESP32-C3

const char* ssid = "Vitality_Monitor"; 
const char* password = "password123";  

const byte DNS_PORT = 53;

// ================== GLOBALS ==================
unsigned long lastBeatTime = 0;
unsigned long lastSampleTime = 0; 
int bpm = 0;
int rawSignal = 0;

int filtered = 0;
int baseline = 0;
bool inBeat = false;

int beatIntervals[4] = {0}; 
int intervalIndex = 0;

// ================== SERVERS ==================
WebServer server(80);
DNSServer dnsServer;

// ================== HTML/UI ==================
const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Vitality Dashboard</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600&display=swap');

    :root {
      --bg-color: #050505;
      --text: #ffffff;
      --text-muted: #a1a1aa;
      --primary: #00f0ff;      
      --alert: #ff4b4b;        
      --glass-bg: rgba(255, 255, 255, 0.03);
      --glass-border: rgba(255, 255, 255, 0.08);
    }

    * { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }

    body {
      background-color: var(--bg-color);
      color: var(--text);
      font-family: 'Inter', sans-serif;
      margin: 0;
      padding: 24px 16px;
      overscroll-behavior-y: none;
      min-height: 100vh;
      position: relative;
      overflow-x: hidden;
    }

    /* Ambient Background */
    .ambient-bg { position: fixed; top: 0; left: 0; width: 100vw; height: 100vh; z-index: -1; overflow: hidden; background: #050505; }
    .orb { position: absolute; border-radius: 50%; filter: blur(80px); opacity: 0.4; animation: float 20s infinite ease-in-out alternate; }
    .orb-1 { width: 400px; height: 400px; background: #1d4ed8; top: -100px; left: -100px; }
    .orb-2 { width: 300px; height: 300px; background: #4c1d95; bottom: 10%; right: -50px; animation-delay: -10s; }
    .orb-3 { width: 250px; height: 250px; background: #065f46; top: 40%; left: 30%; animation-duration: 25s; }

    @keyframes float { 0% { transform: translate(0, 0) scale(1); } 100% { transform: translate(30px, 50px) scale(1.2); } }

    /* Layout */
    .header { max-width: 1200px; margin: 0 auto 32px auto; padding-bottom: 16px; border-bottom: 1px solid var(--glass-border); display: flex; justify-content: space-between; align-items: center; }
    .header h1 { margin: 0; font-size: 28px; font-weight: 600; letter-spacing: -1px; background: linear-gradient(to right, #fff, #a1a1aa); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }
    
    .dashboard-grid { 
      display: grid; 
      grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); 
      gap: 24px; 
      max-width: 1200px; 
      margin: 0 auto; 
    }

    @media (min-width: 768px) {
      .span-2 { grid-column: span 2; }
      .span-full { grid-column: 1 / -1; }
    }

    .disclaimer { font-size: 13px; color: var(--alert); background: rgba(255, 75, 75, 0.05); border: 1px solid rgba(255, 75, 75, 0.2); padding: 12px 16px; border-radius: 12px; display: flex; align-items: center; gap: 12px; line-height: 1.4; }

    /* Cards */
    .panel { background: var(--glass-bg); backdrop-filter: blur(16px); -webkit-backdrop-filter: blur(16px); border: 1px solid var(--glass-border); border-radius: 24px; padding: 24px; display: flex; flex-direction: column; box-shadow: 0 16px 32px rgba(0,0,0,0.3); transition: transform 0.3s ease, box-shadow 0.3s ease; }
    .panel:hover { transform: translateY(-2px); box-shadow: 0 20px 40px rgba(0,0,0,0.4); }
    .panel h2 { margin-top: 0; font-size: 13px; color: var(--text-muted); text-transform: uppercase; letter-spacing: 1.5px; margin-bottom: 24px; font-weight: 600; }

    /* Typography & Inputs */
    .input-style { background: rgba(0,0,0,0.4); color: var(--text); border: 1px solid var(--glass-border); padding: 12px 16px; border-radius: 12px; margin-bottom: 16px; font-family: inherit; font-size: 15px; width: 100%; outline: none; transition: border-color 0.3s; }
    .input-style:focus { border-color: var(--primary); box-shadow: 0 0 0 2px rgba(0, 240, 255, 0.2); }
    
    button { background: var(--text); color: #000; border: none; padding: 16px 24px; border-radius: 50px; cursor: pointer; font-size: 15px; font-weight: 600; transition: all 0.2s ease; width: 100%; letter-spacing: 0.5px; margin-top: auto; }
    button:hover { transform: scale(1.02); box-shadow: 0 0 20px rgba(255,255,255,0.2); }
    button.secondary { background: rgba(255,255,255,0.05); color: var(--text); border: 1px solid var(--glass-border); }

    /* Vitals UI */
    .monitor-display { text-align: center; margin-bottom: 16px; flex-grow: 1; display: flex; flex-direction: column; justify-content: center; align-items: center;}
    .heart-icon { width: 42px; height: 42px; fill: var(--primary); margin-bottom: 16px; filter: drop-shadow(0 0 12px var(--primary)); transition: fill 0.4s ease, filter 0.4s ease; }
    .value { font-size: 72px; font-weight: 300; color: var(--text); font-variant-numeric: tabular-nums; line-height: 1; text-shadow: 0 0 20px rgba(255,255,255,0.2); }
    .unit { font-size: 18px; color: var(--text-muted); font-weight: 400; margin-left: 4px; }
    .status-text { margin-top: 12px; font-size: 14px; font-weight: 500; color: var(--primary); transition: color 0.4s ease; }
    canvas { width: 100%; height: 120px; background: rgba(0,0,0,0.2); border-radius: 16px; border: 1px solid var(--glass-border); margin-bottom: 16px; }

    /* Stats UI */
    .stat-row { display: flex; justify-content: space-between; padding: 16px 0; border-bottom: 1px solid var(--glass-border); }
    .stat-row:last-of-type { border-bottom: none; margin-bottom: 16px; }
    .stat-label { color: var(--text-muted); font-size: 15px; }
    .stat-value { font-weight: 500; font-variant-numeric: tabular-nums; font-size: 20px; color: var(--text); }

    /* Breathing UI */
    .breathe-container { height: 160px; display: flex; align-items: center; justify-content: center; margin: 10px 0 24px 0; }
    .breathe-circle { width: 100px; height: 100px; background: var(--primary); border-radius: 50%; display: flex; align-items: center; justify-content: center; color: #000; font-weight: 600; font-size: 16px; letter-spacing: 1px; box-shadow: 0 0 40px var(--primary); transition: background 0.4s ease, box-shadow 0.4s ease; }
    
    @keyframes breathe-standard { 0% { transform: scale(0.7); opacity: 0.6; } 50% { transform: scale(1.4); opacity: 1; } 100% { transform: scale(0.7); opacity: 0.6; } }
    @keyframes breathe-box { 0% { transform: scale(0.7); opacity: 0.6; } 25% { transform: scale(1.4); opacity: 1; } 50% { transform: scale(1.4); opacity: 1; } 75% { transform: scale(0.7); opacity: 0.6; } 100% { transform: scale(0.7); opacity: 0.6; } }

    /* Mood Logger UI */
    textarea { resize: vertical; min-height: 80px; }
    .mood-grid { display: flex; gap: 8px; margin-bottom: 16px; }
    .mood-btn { flex: 1; padding: 12px 0; background: rgba(255,255,255,0.05); border: 1px solid var(--glass-border); font-size: 22px; border-radius: 12px; text-align: center; cursor: pointer; transition: all 0.2s; margin-top: 0;}
    .mood-btn.active { background: rgba(0, 240, 255, 0.15); border-color: var(--primary); transform: scale(1.05); }

    .log-list { margin-top: 20px; max-height: 180px; overflow-y: auto; padding-right: 8px; display: flex; flex-direction: column; gap: 12px;}
    .log-list::-webkit-scrollbar { width: 6px; }
    .log-list::-webkit-scrollbar-thumb { background: var(--glass-border); border-radius: 10px; }
    .log-entry { background: rgba(255,255,255,0.02); padding: 16px; border-radius: 12px; border-left: 3px solid var(--primary); font-size: 14px; line-height: 1.5; color: #e2e2e6; }
    .log-time { color: var(--text-muted); font-size: 12px; margin-bottom: 8px; display: block; text-transform: uppercase; letter-spacing: 0.5px;}
  </style>
</head>
<body>

  <div class="ambient-bg">
    <div class="orb orb-1"></div>
    <div class="orb orb-2"></div>
    <div class="orb orb-3"></div>
  </div>

  <div class="header">
    <h1>Vitality Flow</h1>
  </div>

  <div class="dashboard-grid">
    
    <div class="disclaimer span-full">
      <strong>⚠️ Project Disclaimer:</strong> This hardware prototype is an educational electronics project. It is not a medical device and must not be used for diagnostic, treatment, or health monitoring purposes.
    </div>

    <div class="panel">
      <h2>Live Vitals</h2>
      <div class="monitor-display">
        <svg class="heart-icon" id="heart" viewBox="0 0 24 24">
          <path d="M12 21.35l-1.45-1.32C5.4 15.36 2 12.28 2 8.5 2 5.42 4.42 3 7.5 3c1.74 0 3.41.81 4.5 2.09C13.09 3.81 14.76 3 16.5 3 19.58 3 22 5.42 22 8.5c0 3.78-3.4 6.86-8.55 11.54L12 21.35z"/>
        </svg>
        <div><span class="value" id="bpm">--</span><span class="unit">bpm</span></div>
        <div class="status-text" id="stress">Awaiting connection...</div>
      </div>
      <button id="toggleBtn" onclick="toggleMonitor()">Initiate Scan</button>
    </div>

    <div class="panel">
      <h2>Pulse Waveform</h2>
      <canvas id="graphCanvas"></canvas>
      <div style="text-align: center; color: var(--text-muted); font-size: 13px; margin-top: auto;">Real-time analog sensor data</div>
    </div>

    <div class="panel">
      <h2>Guided Resonance</h2>
      <select class="input-style" style="margin-bottom: 8px;" onchange="setBreatheMode(this.value)">
        <option value="standard">Standard (4s In / 4s Out)</option>
        <option value="box">Box Breathing</option>
      </select>
      <div class="breathe-container">
        <div class="breathe-circle" id="breatheCircle"><span id="breatheText">Inhale</span></div>
      </div>
    </div>

    <div class="panel">
      <h2>Ambient Audio</h2>
      <p style="font-size: 14px; color: var(--text-muted); margin-top: 0; margin-bottom: 24px;">Generate algorithmic soundscapes to help ground your focus.</p>
      <select class="input-style" id="noiseType" onchange="changeNoiseType(this.value)">
        <option value="white">White Noise (Static)</option>
        <option value="pink">Pink Noise (Rain)</option>
        <option value="brown">Brown Noise (Ocean)</option>
      </select>
      <button class="secondary" id="noiseBtn" onclick="togglePlayNoise()">Play Soundscape</button>
    </div>

    <div class="panel">
      <h2>Session Stats</h2>
      <div class="stat-row">
        <span class="stat-label">Session Duration</span>
        <span class="stat-value" id="sessionTime">00:00</span>
      </div>
      <div class="stat-row">
        <span class="stat-label">Average Heart Rate</span>
        <span class="stat-value" id="avgBpm">--</span>
      </div>
    </div>

    <div class="panel span-2">
      <h2>Mindfulness Log</h2>
      <div class="dashboard-grid" style="gap: 24px; display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));">
        <div>
          <p style="font-size: 14px; color: var(--text-muted); margin-top: 0; margin-bottom: 16px;">How are you feeling right now?</p>
          <div class="mood-grid" id="moodGrid">
            <button class="mood-btn active" onclick="selectMood(this, '😌')">😌</button>
            <button class="mood-btn" onclick="selectMood(this, '😊')">😊</button>
            <button class="mood-btn" onclick="selectMood(this, '😐')">😐</button>
            <button class="mood-btn" onclick="selectMood(this, '😟')">😟</button>
          </div>
          <textarea class="input-style" id="journalInput" placeholder="Add a note..."></textarea>
          <button onclick="saveLog()">Save Entry</button>
        </div>
        <div class="log-list" id="logList">
          <div style="text-align: center; color: var(--text-muted); margin-top: 20px; font-size: 14px;">No entries yet.</div>
        </div>
      </div>
    </div>

  </div>

<script>
  // --- UI & State ---
  const root = document.documentElement;
  const canvas = document.getElementById('graphCanvas');
  const ctx = canvas.getContext('2d');
  
  let isMonitoring = false;
  let monitorInterval;
  let graphData = new Array(150).fill(0);
  let sessionSeconds = 0;
  let bpmHistory = [];
  let timerInterval;

  // --- Graph Drawing ---
  function drawGraph() {
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    let max = Math.max(...graphData) || 1;
    let min = Math.min(...graphData) || 0;
    let range = max - min;
    if(range === 0) range = 1;

    const primaryColor = getComputedStyle(root).getPropertyValue('--primary').trim();

    ctx.beginPath();
    ctx.strokeStyle = primaryColor;
    ctx.lineWidth = 3;
    ctx.lineCap = 'round'; ctx.lineJoin = 'round';
    ctx.shadowBlur = 12; ctx.shadowColor = primaryColor;

    for(let i=0; i<graphData.length; i++) {
      let x = (i / graphData.length) * canvas.width;
      let y = (canvas.height - 20) - ((graphData[i] - min) / range) * (canvas.height - 40) + 10; 
      if(i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.stroke();
    ctx.shadowBlur = 0; 
  }
  
  function resizeCanvas() { 
    if(canvas.parentElement) {
      canvas.width = canvas.parentElement.clientWidth - 48; 
      drawGraph();
    }
  }
  window.addEventListener('resize', resizeCanvas);
  setTimeout(resizeCanvas, 100); // Initial sizing

  // --- Monitoring Controls ---
  function toggleMonitor() {
    isMonitoring = !isMonitoring;
    const btn = document.getElementById('toggleBtn');
    const statusText = document.getElementById('stress');
    
    if (isMonitoring) {
      btn.innerText = "End Scan"; btn.className = "secondary";
      statusText.innerText = "Calibrating sensor...";
      
      timerInterval = setInterval(() => {
        sessionSeconds++;
        let m = Math.floor(sessionSeconds / 60).toString().padStart(2, '0');
        let s = (sessionSeconds % 60).toString().padStart(2, '0');
        document.getElementById('sessionTime').innerText = m + ":" + s;
      }, 1000);

      monitorInterval = setInterval(fetchData, 100);
    } else {
      btn.innerText = "Initiate Scan"; btn.className = "";
      statusText.innerText = "Monitor paused";
      document.getElementById('heart').style.animation = 'none';
      clearInterval(monitorInterval);
      clearInterval(timerInterval);
    }
  }

  function fetchData() {
    fetch('/data').then(res => res.json()).then(d => processSensorData(d)).catch(e => console.log("Fetch error"));
  }

  function processSensorData(d) {
    const bpmVal = d.bpm;
    document.getElementById('bpm').innerText = bpmVal > 0 ? bpmVal : "--";
    
    graphData.push(d.raw);
    graphData.shift();
    drawGraph();

    if (bpmVal > 0) {
        bpmHistory.push(bpmVal);
        let sum = bpmHistory.reduce((a, b) => a + b, 0);
        let avg = Math.round(sum / bpmHistory.length);
        document.getElementById('avgBpm').innerText = avg;

        let beatInterval = 60000 / bpmVal;
        document.getElementById('heart').style.animation = `heartbeat ${beatInterval/1000}s infinite ease-in-out`;
        
        if (bpmVal > 100) {
            document.getElementById('stress').innerText = "Elevated Heart Rate";
            root.style.setProperty('--primary', 'var(--alert)');
            document.getElementById('stress').style.color = "var(--alert)";
        } else {
            document.getElementById('stress').innerText = "Resting / Relaxed";
            root.style.setProperty('--primary', '#00f0ff');
            document.getElementById('stress').style.color = "var(--primary)";
        }
    } else {
        document.getElementById('heart').style.animation = 'none';
    }
  }

  // --- Journal & Mood Logger ---
  let selectedMood = "😌";
  function selectMood(btn, mood) {
    document.querySelectorAll('.mood-btn').forEach(b => b.classList.remove('active'));
    btn.classList.add('active');
    selectedMood = mood;
  }

  function saveLog() {
    const input = document.getElementById('journalInput');
    const text = input.value.trim();
    if (!text) return;

    const now = new Date();
    const timeStr = now.toLocaleTimeString([], {hour: '2-digit', minute:'2-digit'});
    const currentBpm = document.getElementById('bpm').innerText;
    
    const logList = document.getElementById('logList');
    
    // Clear "No entries yet" message if it exists
    if(logList.children.length === 1 && logList.children[0].innerText === "No entries yet.") {
      logList.innerHTML = '';
    }

    const newEl = document.createElement('div');
    newEl.className = 'log-entry';
    newEl.innerHTML = `<span class="log-time">${timeStr} | BPM: ${currentBpm} | Mood: ${selectedMood}</span>${text}`;
    
    logList.prepend(newEl);
    input.value = ''; input.blur(); 
  }

  // --- Guided Breathing ---
  let breatheInterval;
  function setBreatheMode(mode) {
    const circle = document.getElementById('breatheCircle');
    const textEl = document.getElementById('breatheText');
    clearInterval(breatheInterval);
    
    circle.style.animation = 'none';
    void circle.offsetWidth; // trigger reflow
    
    if(mode === 'standard') {
      circle.style.animation = 'breathe-standard 8s infinite ease-in-out';
      textEl.innerText = "Inhale";
      let isExhale = false;
      breatheInterval = setInterval(() => {
        isExhale = !isExhale;
        textEl.innerText = isExhale ? "Exhale" : "Inhale";
      }, 4000);
    } else {
      circle.style.animation = 'breathe-box 16s infinite linear';
      let phase = 0;
      const phases = ["Inhale", "Hold", "Exhale", "Hold"];
      textEl.innerText = phases[0];
      breatheInterval = setInterval(() => {
        phase = (phase + 1) % 4;
        textEl.innerText = phases[phase];
      }, 4000);
    }
  }
  setBreatheMode('standard');

  // --- Web Audio API Noise Generator ---
  let audioCtx, noiseSource, filter, gainNode;
  let isPlaying = false;
  let currentNoiseType = 'white';

  function playNoise() {
    if(!audioCtx) audioCtx = new (window.AudioContext || window.webkitAudioContext)();
    let bufferSize = 2 * audioCtx.sampleRate;
    let noiseBuffer = audioCtx.createBuffer(1, bufferSize, audioCtx.sampleRate);
    let output = noiseBuffer.getChannelData(0);
    
    if (currentNoiseType === 'white') {
       for (let i = 0; i < bufferSize; i++) { output[i] = Math.random() * 2 - 1; }
    } else if (currentNoiseType === 'pink') {
       let b0=0, b1=0, b2=0, b3=0, b4=0, b5=0, b6=0;
       for (let i = 0; i < bufferSize; i++) {
          let white = Math.random() * 2 - 1;
          b0 = 0.99886 * b0 + white * 0.0555179; b1 = 0.99332 * b1 + white * 0.0750759;
          b2 = 0.96900 * b2 + white * 0.1538520; b3 = 0.86650 * b3 + white * 0.3104856;
          b4 = 0.55000 * b4 + white * 0.5329522; b5 = -0.7616 * b5 - white * 0.0168980;
          output[i] = (b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362) * 0.11;
       }
    } else if (currentNoiseType === 'brown') {
       let lastOut = 0.0;
       for (let i = 0; i < bufferSize; i++) {
          let white = Math.random() * 2 - 1;
          output[i] = (lastOut + 0.02 * white) / 1.02;
          lastOut = output[i];
          output[i] *= 3.5; 
       }
    }
    
    noiseSource = audioCtx.createBufferSource();
    noiseSource.buffer = noiseBuffer;
    noiseSource.loop = true;
    
    filter = audioCtx.createBiquadFilter();
    filter.type = 'lowpass';
    filter.frequency.value = 1000;
    
    gainNode = audioCtx.createGain();
    gainNode.gain.value = 0.5;

    noiseSource.connect(filter);
    filter.connect(gainNode);
    gainNode.connect(audioCtx.destination);
    noiseSource.start();
  }

  function togglePlayNoise() {
    if(isPlaying) {
      if(noiseSource) noiseSource.stop();
      isPlaying = false;
      document.getElementById('noiseBtn').innerText = "Play Soundscape";
    } else {
      playNoise();
      isPlaying = true;
      document.getElementById('noiseBtn').innerText = "Pause Audio";
    }
  }

  function changeNoiseType(type) {
    currentNoiseType = type;
    if(isPlaying) {
      if(noiseSource) noiseSource.stop();
      playNoise();
    }
  }

  const styleSheet = document.createElement("style");
  styleSheet.innerText = `@keyframes heartbeat { 0% { transform: scale(1); } 20% { transform: scale(1.15); } 40% { transform: scale(1); } 60% { transform: scale(1.15); } 80% { transform: scale(1); } 100% { transform: scale(1); } }`;
  document.head.appendChild(styleSheet);
</script>
</body>
</html>
)rawliteral";

// ================== ROUTES ==================
void handleRoot() { server.send(200, "text/html", PAGE); }

void handleData() {
  String json = "{";
  json += "\"bpm\":" + String(bpm) + ",";
  json += "\"raw\":" + String(rawSignal);
  json += "}";
  server.send(200, "application/json", json);
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  WiFi.softAP(ssid, password);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/data", handleData);

  server.onNotFound([]() {
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
    server.send(302, "text/plain", "");
  });

  server.begin();
}

// ================== LOOP ==================
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  unsigned long now = millis();

  if (now - lastSampleTime >= 5) {
    lastSampleTime = now;
    rawSignal = analogRead(PULSE_PIN);

    filtered = (filtered * 9 + rawSignal) / 10;
    baseline = (baseline * 49 + filtered) / 50;

    int threshold = baseline + 50; 

    if (!inBeat && filtered > threshold && (now - lastBeatTime) > 250) {
      inBeat = true;

      if (lastBeatTime > 0) {
        int currentInterval = now - lastBeatTime;
        if (currentInterval > 300 && currentInterval < 1500) {
          beatIntervals[intervalIndex] = currentInterval;
          intervalIndex = (intervalIndex + 1) % 4;

          int totalInterval = 0;
          int validCount = 0;
          for (int i = 0; i < 4; i++) {
            if (beatIntervals[i] > 0) {
              totalInterval += beatIntervals[i];
              validCount++;
            }
          }

          if (validCount > 0) {
            int avgInterval = totalInterval / validCount;
            bpm = 60000 / avgInterval;
          }
        }
      }
      lastBeatTime = now;
    }

    if (filtered < baseline) {
      inBeat = false;
    }
  }
}

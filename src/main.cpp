// by mircemk May, 2026

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <si5351.h>
#include <Wire.h>

Si5351 si5351;

uint32_t frequency = 7000000UL;

// TODO do the calibration https://github.com/etherkit/Si5351Arduino/blob/master/examples/si5351_calibration/si5351_calibration.ino

const char* ssid = "Si5351_VFO_Final_Complete";
const char* password = "vfo12345678";

ESP8266WebServer server(80);

const uint8_t I2C_SDA = 0;
const uint8_t I2C_SCL = 2;

const char VFO_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=0">
<style>
  :root { 
    /* ТУКА СЕ МЕНУВААТ БОИТЕ - ВЕРЗИЈА V2.4 */
    --panel-bg: #E0AB07;      
    --inner-bezel:#E0AB07 //#56748F;   
    --lcd-bg: #0077c2;        
    --btn-band: #7f0000;      
    --btn-step: #27ae60;      
    --btn-mode: #2980b9;      
    --btn-mem: #8e44ad;       
    --gold-border: #f1c40f;   
    --text-color: #ecf0f1;    
  }

  * { -webkit-tap-highlight-color: transparent; box-sizing: border-box; -webkit-touch-callout: none; -webkit-user-select: none; user-select: none; }
  body { background: #000; margin: 0; padding: 0; display: flex; justify-content: center; font-family: 'Arial Black', sans-serif; color: var(--text-color); overflow: hidden; }
  .vfo-main-frame { background: var(--panel-bg); width: 100%; max-width: 400px; height: 100vh; display: flex; flex-direction: column; align-items: center; border-left: 2px solid #444; border-right: 2px solid #111; }
  
  .bezel-display { background: var(--inner-bezel); width: 92%; margin-top: 15px; padding: 10px; border-radius: 8px; box-shadow: inset 4px 4px 10px #000, 2px 2px 5px rgba(255,255,255,0.1); position: relative; }
  
  .fs-zone { position: absolute; left: 0; top: 0; width: 30%; height: 100%; z-index: 10; cursor: pointer; }
  .mem-zone { position: absolute; left: 30%; top: 0; width: 40%; height: 100%; z-index: 10; cursor: pointer; }
  .reset-zone { position: absolute; right: 0; top: 0; width: 30%; height: 100%; z-index: 10; cursor: pointer; }

  .display { background: var(--lcd-bg); border: 4px solid #111; padding: 10px; box-shadow: inset 0 0 25px #000; height: 125px; display: flex; flex-direction: column; justify-content: space-between; position: relative; transition: background 0.2s; }
  .display.mem-active { background: #e67e22; }
  .display.reset-flash { background: #e74c3c; }

  .display-info { display: flex; justify-content: space-between; font-size: 14px; color: rgba(255,255,255,0.9); font-family: Arial, sans-serif; }
  #f-display { font-size: 55px; font-weight: 900; margin: 0; text-align: right; text-shadow: 2px 2px 4px #000; letter-spacing: -1px; line-height: 1; }
  
  .display-footer { display: flex; align-items: center; border-top: 1px solid rgba(255,255,255,0.2); padding-top: 5px; margin-bottom: 4px; }
  #mode-label, .sig-text { font-size: 15px; font-weight: bold; }
  
  .s-meter-container { display: flex; align-items: center; gap: 6px; flex-grow: 1; justify-content: flex-end; margin-left: 25px; }
  .s-grid { display: flex; gap: 1px; height: 10px; width: 115px; background: rgba(0,0,0,0.3); border: 1px solid #111; }
  .s-seg { flex: 1; background: #222; }
  .s-on { background: #ffffff; box-shadow: 0 0 6px #ffffff; }

  .bezel-knob { background: var(--inner-bezel); width: 270px; height: 270px; margin: 25px 0; border-radius: 50%; box-shadow: inset 3px 3px 10px #000, 2px 2px 5px rgba(255,255,255,0.05); display: flex; justify-content: center; align-items: center; }
  #knob { width: 240px; height: 240px; background: conic-gradient(from 0deg, #333, #777 25%, #333 50%, #777 75%, #333); border-radius: 50%; border: 12px solid #1a1a1a; position: relative; will-change: transform; cursor: pointer; box-shadow: 5px 10px 20px #000; }
  #knob::after { content: ''; position: absolute; top: 25px; left: 50%; transform: translateX(-50%); width: 24px; height: 24px; background: #111; border-radius: 50%; box-shadow: inset 2px 2px 5px #000; }

  .controls-container { width: 94%; display: flex; flex-direction: column; }
  .grid { display: grid; gap: 6px; width: 100%; grid-template-columns: repeat(4, 1fr); }
  .group-margin { margin-bottom: 12px; }
  
  .btn { border: 3px solid var(--gold-border); border-radius: 8px; color: #fff; font-weight: 900; font-size: 15px; padding: 11px 0; text-align: center; cursor: pointer; box-shadow: 3px 5px 8px #000; text-transform: uppercase; transition: transform 0.05s; }
  .btn:active { transform: translateY(2px); box-shadow: 1px 2px 4px #000; }
  
  .b-band { background: var(--btn-band); }
  .b-step { background: var(--btn-step); font-size: 20px; padding: 12px; grid-column: span 4; }
  .b-mode { background: var(--btn-mode); }
  .b-mem { background: var(--btn-mem); border-color: #555; font-size: 13px; }
  
  .signature { color: #555; font-size: 16px; margin-top: 20px; text-align: center; width: 100%; padding-bottom: 15px; font-weight: normal; }
</style>
</head><body>
  <div class="vfo-main-frame">
    <div class="bezel-display">
      <div class="fs-zone" onclick="toggleFS()"></div>
      <div class="mem-zone" onclick="startMem()"></div>
      <div class="reset-zone" onclick="clearAllMem()"></div>
      
      <div class="display" id="main-display">
        <div class="display-info"><span id="band-label">40M HAM</span><span id="step-label">100Hz</span></div>
        <h1 id="f-display">07.000.000</h1>
        <div class="display-footer">
          <span id="mode-label">USB</span>
          <div class="s-meter-container">
            <span class="sig-text">Sig:</span>
            <div class="s-grid" id="s-grid"></div>
          </div>
        </div>
      </div>
    </div>
    <div class="bezel-knob"><div id="knob"></div></div>
    <div class="controls-container">
      <div class="grid group-margin">
        <div class="btn b-band" onclick="setBand(531000, 'MW')">MW</div>
        <div class="btn b-band" onclick="setBand(1810000, '160M')">160</div>
        <div class="btn b-band" onclick="setBand(3500000, '80M')">80</div>
        <div class="btn b-band" onclick="setBand(7000000, '40M')">40</div>
        <div class="btn b-band" onclick="setBand(14000000, '20M')">20</div>
        <div class="btn b-band" onclick="setBand(18068000, '17M')">17</div>
        <div class="btn b-band" onclick="setBand(21000000, '15M')">15</div>
        <div class="btn b-band" onclick="setBand(24890000, '12M')">12</div>
      </div>
      <div class="grid group-margin">
        <div class="btn b-step" id="step-btn" onclick="nextStep()">STEP: 100Hz</div>
      </div>
      <div class="grid">
        <div class="btn b-mode" onclick="setMode('AM')">AM</div>
        <div class="btn b-mode" onclick="setMode('USB')">USB</div>
        <div class="btn b-mode" onclick="setMode('LSB')">LSB</div>
        <div class="btn b-mode" onclick="setMode('FM')">FM</div>
        <div class="btn b-mem" id="m1" onclick="handleMem(1)">M1</div>
        <div class="btn b-mem" id="m2" onclick="handleMem(2)">M2</div>
        <div class="btn b-mem" id="m3" onclick="handleMem(3)">M3</div>
        <div class="btn b-mem" id="m4" onclick="handleMem(4)">M4</div>
      </div>
    </div>
    <div class="signature">Si5351 VFO by mircemk</div>
  </div>

  <script>
    var freq = 7000000;
    var curMode = "USB"; var lastAngle = 0; var curRot = 0; var isDrag = false; var lastSent = 0;
    var steps = [10, 100, 1000, 5000, 10000, 100000];
    var stepLabels = ["10Hz", "100Hz", "1KHz", "5KHz", "10KHz", "100KHz"];
    var stepIdx = 1;
    var isMemMode = false;

    function loadSavedMem() {
      for(let i=1; i<=4; i++){
        let saved = localStorage.getItem('vfo_m'+i);
        if(saved) document.getElementById('m'+i).innerText = (saved/1000000).toFixed(3);
        else document.getElementById('m'+i).innerText = "M"+i;
      }
    }

    function clearAllMem() {
      for(let i=1; i<=4; i++) localStorage.removeItem('vfo_m'+i);
      loadSavedMem();
      let d = document.getElementById('main-display');
      d.classList.add('reset-flash');
      setTimeout(() => d.classList.remove('reset-flash'), 300);
    }

    function startMem() {
      isMemMode = true;
      document.getElementById('main-display').classList.add('mem-active');
      document.querySelectorAll('.b-mem').forEach(b => b.classList.add('save-ready'));
    }

    function handleMem(id) {
      if(isMemMode) {
        localStorage.setItem('vfo_m'+id, freq);
        document.getElementById('m'+id).innerText = (freq/1000000).toFixed(3);
        isMemMode = false;
        document.getElementById('main-display').classList.remove('mem-active');
        document.querySelectorAll('.b-mem').forEach(b => b.classList.remove('save-ready'));
      } else {
        let saved = localStorage.getItem('vfo_m'+id);
        if(saved) { freq = parseInt(saved); updateUI(); sendFreq(); }
      }
    }

    function toggleFS() {
      var d = document.documentElement;
      if(!document.fullscreenElement) d.requestFullscreen().catch(e=>{});
      else document.exitFullscreen();
    }

    function updateBandLabel() {
      let b = document.getElementById('band-label');
      // ПРЕЗЕМЕНИ ОПСЕЗИ ОД V1.9
      if (freq >= 1810000 && freq <= 2000000) b.innerText = "160M HAM";
      else if (freq >= 3500000 && freq <= 3800000) b.innerText = "80M HAM";
      else if (freq >= 7000000 && freq <= 7200000) b.innerText = "40M HAM";
      else if (freq >= 14000000 && freq <= 14350000) b.innerText = "20M HAM";
      else if (freq >= 18068000 && freq <= 18168000) b.innerText = "17M HAM";
      else if (freq >= 21000000 && freq <= 21450000) b.innerText = "15M HAM";
      else if (freq >= 24890000 && freq <= 24990000) b.innerText = "12M HAM";
      else if (freq >= 28000000 && freq <= 29700000) b.innerText = "10M HAM";
      else if (freq >= 531000 && freq <= 1602000) b.innerText = "MW BROADCAST";
      else if (freq >= 5900000 && freq <= 6200000) b.innerText = "49M BROADCAST";
      else if (freq >= 7200001 && freq <= 7450000) b.innerText = "41M BROADCAST";
      else if (freq >= 9400000 && freq <= 9900000) b.innerText = "31M BROADCAST";
      else if (freq >= 11600000 && freq <= 12100000) b.innerText = "25M BROADCAST";
      else if (freq >= 15100000 && freq <= 15830000) b.innerText = "19M BROADCAST";
      else b.innerText = "GEN";
    }

    function updateUI() {
      document.getElementById('f-display').innerText = Number(freq).toLocaleString('de-DE').replace(/,/g, '.');
      document.getElementById('mode-label').innerText = curMode;
      updateBandLabel();
    }

    function setBand(f, n) { freq = f; updateUI(); sendFreq(); }
    function setMode(m) { curMode = m; updateUI(); }
    function nextStep() {
      stepIdx = (stepIdx + 1) % steps.length;
      document.getElementById('step-btn').innerText = "STEP: " + stepLabels[stepIdx];
      document.getElementById('step-label').innerText = stepLabels[stepIdx];
    }
    function sendFreq() {
      let now = Date.now();
      if (now - lastSent > 50) { fetch('/set?f=' + freq); lastSent = now; }
    }
    function getAngle(x, y) {
      let r = document.getElementById('knob').getBoundingClientRect();
      return Math.atan2(y - (r.top + r.height/2), x - (r.left + r.width/2)) * 180 / Math.PI;
    }
    function move(e) {
      if (!isDrag) return;
      let ev = e.touches ? e.touches[0] : e;
      let ang = getAngle(ev.clientX, ev.clientY);
      let d = ang - lastAngle;
      if (d > 180) d -= 360; if (d < -180) d += 360;
      curRot += d;
      freq += Math.round(d) * (steps[stepIdx] / 10);
      if (freq < 100000) freq = 100000;
      updateUI();
      document.getElementById('knob').style.transform = 'rotate(' + curRot + 'deg)';
      sendFreq();
      lastAngle = ang;
    }
    let knob = document.getElementById('knob');
    knob.addEventListener('mousedown', function(e) { isDrag = true; lastAngle = getAngle(e.clientX, e.clientY); });
    knob.addEventListener('touchstart', function(e) { isDrag = true; lastAngle = getAngle(e.touches[0].clientX, e.touches[0].clientY); e.preventDefault(); }, {passive: false});
    window.addEventListener('mouseup', () => isDrag = false);
    window.addEventListener('touchend', () => isDrag = false);
    window.addEventListener('mousemove', move);
    window.addEventListener('touchmove', move, {passive: false});
    
    setInterval(() => {
      fetch('/getS').then(r => r.text()).then(v => {
        let segs = document.querySelectorAll('.s-seg');
        let act = Math.floor((v/100)*20);
        segs.forEach((s,i) => { if(i<act) s.classList.add('s-on'); else s.classList.remove('s-on'); });
      });
    }, 250);

    loadSavedMem();
    updateUI();
  </script>
</body></html>
)rawliteral";

void updateFrequency(uint32_t f) {
  // Si5351 library expects frequency in units of 0.01 Hz.
  si5351.set_freq(static_cast<uint64_t>(f) * SI5351_FREQ_MULT, SI5351_CLK0);
}

void handleRoot() {
  server.send_P(200, "text/html", VFO_HTML);
}

void handleSetFrequency() {
  if (!server.hasArg("f")) {
    server.send(400, "text/plain", "Missing frequency parameter");
    return;
  }

  String value = server.arg("f");

  // Convert the HTTP parameter to an unsigned long value.
  uint32_t newFrequency = strtoul(value.c_str(), nullptr, 10);

  if (newFrequency == 0 || newFrequency > SI5351_MULTISYNTH_MAX_FREQ) {
    server.send(400, "text/plain", "Invalid frequency");
    return;
  }

  frequency = newFrequency;
  updateFrequency(frequency);

  server.send(200, "text/plain", "OK");
}

void handleSignalLevel() {
  int percent = 0;
  server.send(200, "text/plain", String(percent));
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // Initialize I2C.
  Wire.begin(I2C_SDA, I2C_SCL);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.println();
  Serial.println("Access point started");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  bool initialized = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);

  if (!initialized) {
    Serial.println("Si5351 initialization failed");
  } else {
    Serial.println("Si5351 initialized");
  }

  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA);
  updateFrequency(frequency);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/set", HTTP_GET, handleSetFrequency);
  server.on("/getS", HTTP_GET, handleSignalLevel);

  server.begin();

  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}

#include "web_server.h"
#include <WiFi.h>
#include <WebServer.h>
#include "config.h"
#include "sync.h"
#include "nvs_manager.h"

namespace LFRWeb {

    static WebServer server(80);

    // ---------------- Embedded Web UI (Responsive Dark Theme) ----------------
    static const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html><head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Line Follower</title>
<style>
body{font-family:-apple-system,BlinkMacSystemFont,Segoe UI,Roboto,Arial,sans-serif;background:#111;color:#eee;margin:0;padding:16px;text-align:center}
h1{font-size:22px;margin:4px 0 10px;font-weight:700;letter-spacing:0.5px}
#st{display:inline-block;font-size:16px;font-weight:700;padding:7px 18px;border-radius:20px;background:#333;margin-bottom:6px;transition:background 0.2s}
#st.READY{background:#2563eb;color:#fff}#st.RUNNING{background:#16a34a;color:#fff}#st.CALIBRATING{background:#ca8a04;color:#fff}#st.IDLE{background:#475569;color:#e2e8f0}
#msg{font-size:13px;color:#cbd5e1;margin:10px auto;max-width:440px;min-height:38px;line-height:1.4;background:#1e1e24;padding:8px 12px;border-radius:8px;border:1px solid #2d2d38}
.bars{display:flex;gap:3px;height:75px;align-items:flex-end;max-width:440px;margin:12px auto 0;background:#18181b;padding:6px;border-radius:8px}
.bar{flex:1;background:#3b82f6;border-radius:2px 2px 0 0;min-height:2px;transition:height 0.05s}
.bar.on{background:#22c55e}.bar.bad{background:#4b5563}
.nums{display:flex;gap:3px;max-width:440px;margin:4px auto 0;padding:0 6px}.n{flex:1;font-size:9px;color:#94a3b8}

.pid{max-width:440px;margin:18px auto 0;padding:14px 18px;background:#18181b;border-radius:12px;text-align:left;border:1px solid #27272a}
.pid h2{font-size:14px;margin:0 0 10px;color:#93c5fd;font-weight:600;text-align:center;letter-spacing:0.5px}
.row{display:flex;align-items:center;gap:12px;margin:8px 0}
.row label{width:46px;font-weight:700;font-size:15px;color:#e2e8f0}
.row input[type=range]{flex:1;height:30px;accent-color:#3b82f6;cursor:pointer}
.row span{width:64px;text-align:right;font-size:14px;font-variant-numeric:tabular-nums;color:#38bdf8;font-family:monospace}

.action-btns{max-width:440px;margin:16px auto 0;display:flex;flex-direction:column;gap:10px}
button{display:block;width:100%;padding:18px;font-size:20px;font-weight:700;border:none;border-radius:10px;cursor:pointer;transition:opacity 0.2s,transform 0.1s}
button:active{transform:scale(0.98)}
button:disabled{opacity:0.35;cursor:not-allowed}
#cal{background:#2563eb;color:#fff}
#go{background:#16a34a;color:#fff}
#go.stop{background:#dc2626;color:#fff}

.cfg-grid{max-width:440px;margin:14px auto 0;display:grid;grid-template-columns:1fr 1fr;gap:8px}
.btn-sec{padding:12px 10px;font-size:13px;font-weight:600;border-radius:8px;background:#27272a;color:#e4e4e7;border:1px solid #3f3f46}
.btn-sec:hover{background:#3f3f46}
.btn-warn{grid-column:span 2;background:#271c1c;color:#f87171;border:1px solid #7f1d1d}
.btn-warn:hover{background:#451a1a}
</style></head><body>
<h1>Line Follower</h1>
<div id="st">IDLE</div>
<div id="msg">Initializing...</div>
<div class="bars" id="bars"></div><div class="nums" id="nums"></div>

<div class="pid">
  <h2>PID &amp; Speed Tuning</h2>
  <div class="row"><label>Kp</label><input type="range" id="kp" min="0" max="5" step="0.01" value="0.1"><span id="kpv">0.10</span></div>
  <div class="row"><label>Ki</label><input type="range" id="ki" min="0" max="1" step="0.0005" value="0"><span id="kiv">0.0000</span></div>
  <div class="row"><label>Kd</label><input type="range" id="kd" min="0" max="20" step="0.05" value="1"><span id="kdv">1.00</span></div>
  <div class="row"><label>Speed</label><input type="range" id="sp" min="0" max="255" step="1" value="230"><span id="spv">230</span></div>
  <div class="row"><label>Turn</label><input type="range" id="tn" min="0" max="255" step="1" value="80"><span id="tnv">80</span></div>
  <div class="row"><label>Start</label><input type="range" id="ss" min="0" max="255" step="1" value="180"><span id="ssv">180</span></div>
</div>

<div class="action-btns">
  <button id="cal" onclick="act('cal')">Calibrate</button>
  <button id="go" onclick="act('start')">Start</button>
</div>

<div class="cfg-grid">
  <button class="btn-sec" onclick="act('save')">&#128190; Save Settings</button>
  <button class="btn-sec" onclick="act('defaults')">&#8634; Load Defaults</button>
  <button class="btn-sec btn-warn" onclick="if(confirm('Reset calibration tables to uncalibrated?'))act('reset_cal')">&#9888; Reset Calibration</button>
</div>

<script>
const $=id=>document.getElementById(id);
let bh='',nh='';for(let i=0;i<14;i++){bh+='<div class="bar" id="b'+i+'"></div>';nh+='<div class="n">'+i+'</div>'}
$('bars').innerHTML=bh;$('nums').innerHTML=nh;

function act(c){
  fetch('/'+c,{method:'POST'}).then(r=>r.text()).then(t=>{
    if(c==='save'||c==='defaults'||c==='reset_cal') $('msg').textContent=t;
  }).catch(()=>{});
}

// ---- PID sliders ----
const KEYS=['kp','ki','kd','sp','tn','ss'];
const dec={kp:2,ki:4,kd:2,sp:0,tn:0,ss:0};
let hold=0,sendT=0;

function showPid(){
  for(const k of KEYS)$(k+'v').textContent=(+$(k).value).toFixed(dec[k]);
}

function sendPid(){
  const url='/pid?kp='+$('kp').value+'&ki='+$('ki').value+'&kd='+$('kd').value+'&sp='+$('sp').value+'&tn='+$('tn').value+'&ss='+$('ss').value;
  fetch(url,{method:'POST'}).catch(()=>{});
}

for(const k of KEYS){
  $(k).addEventListener('input',()=>{
    hold=Date.now()+1500; // Freeze polling updates while dragging
    showPid();
    const now=Date.now();
    if(now-sendT>100){sendT=now;sendPid();} // Throttled updates during drag
  });
  $(k).addEventListener('change',()=>{
    hold=Date.now()+1500;
    sendPid(); // Final value on release
  });
}
showPid();

function upd(){
  fetch('/status').then(r=>r.json()).then(d=>{
    const st=$('st');
    st.textContent=d.state+(d.state==='CALIBRATING'?' '+d.cal+'%':'');
    st.className=d.state;
    $('msg').textContent=d.msg;

    const run=d.state==='RUNNING', busy=d.state==='CALIBRATING';
    const g=$('go');
    g.textContent=run?'Stop':'Start';
    g.className=run?'stop':'';
    g.disabled=(d.state==='IDLE'||busy);
    $('cal').disabled=(run||busy);

    d.s.forEach((v,i)=>{
      const b=$('b'+i);
      b.style.height=Math.max(2,v/10)+'%';
      b.className='bar'+(!d.ok[i]?' bad':(v>500?' on':''));
    });

    if(Date.now()>hold){
      $('kp').value=d.kp; $('ki').value=d.ki; $('kd').value=d.kd;
      $('sp').value=d.sp; $('tn').value=d.tn; $('ss').value=d.ss;
      showPid();
    }
  }).catch(()=>{});
}
setInterval(upd,300);
upd();
</script></body></html>
)HTML";

    // ---------------- HTTP Route Handlers ----------------
    static void handleRoot() {
        server.send_P(200, "text/html", INDEX_HTML);
    }

    static void handleStatus() {
        LFRTelemetry telem;
        Sync::getTelemetrySnapshot(telem);

        char msg[160];
        Sync::getStatusMessage(msg, sizeof(msg));

        String o;
        o.reserve(750);
        o = "{\"state\":\"";
        o += robotStateToString(telem.state);
        o += "\",\"cal\":";
        o += String(telem.calPct);
        o += ",\"msg\":\"";
        for (const char* p = msg; *p; p++) {
            if (*p == '"' || *p == '\\') o += '\\';
            o += *p;
        }
        o += "\",\"s\":[";
        for (int i = 0; i < NUM_SENSORS; i++) {
            if (i) o += ",";
            o += String(telem.sensorValue[i]);
        }
        o += "],\"ok\":[";
        for (int i = 0; i < NUM_SENSORS; i++) {
            if (i) o += ",";
            o += (telem.valid[i] ? "1" : "0");
        }
        o += "],\"kp\":";
        o += String(telem.Kp, 4);
        o += ",\"ki\":";
        o += String(telem.Ki, 4);
        o += ",\"kd\":";
        o += String(telem.Kd, 4);
        o += ",\"sp\":";
        o += String(telem.lfSpeed);
        o += ",\"tn\":";
        o += String(telem.turnSpeed);
        o += ",\"ss\":";
        o += String(telem.startSpeed);
        o += ",\"err\":";
        o += String(telem.error, 1);
        o += ",\"pid\":";
        o += String(telem.PIDvalue);
        o += ",\"lsp\":";
        o += String(telem.lsp);
        o += ",\"rsp\":";
        o += String(telem.rsp);
        o += "}";

        server.send(200, "application/json", o);
    }

    static void handleCal() {
        Sync::requestCalibrate();
        server.send(200, "text/plain", "ok");
    }

    static void handleStart() {
        Sync::requestStartStopToggle();
        server.send(200, "text/plain", "ok");
    }

    static void handlePid() {
        LFRConfig currentCfg;
        Sync::getConfigSnapshot(currentCfg);

        float kp = currentCfg.Kp;
        float ki = currentCfg.Ki;
        float kd = currentCfg.Kd;
        int   sp = currentCfg.lfSpeed;
        int   tn = currentCfg.turnSpeed;
        int   ss = currentCfg.startSpeed;

        if (server.hasArg("kp")) kp = constrain(server.arg("kp").toFloat(), KP_MIN, KP_MAX);
        if (server.hasArg("ki")) ki = constrain(server.arg("ki").toFloat(), KI_MIN, KI_MAX);
        if (server.hasArg("kd")) kd = constrain(server.arg("kd").toFloat(), KD_MIN, KD_MAX);
        if (server.hasArg("sp")) sp = constrain(server.arg("sp").toInt(), SPEED_MIN, SPEED_MAX);
        if (server.hasArg("tn")) tn = constrain(server.arg("tn").toInt(), SPEED_MIN, SPEED_MAX);
        if (server.hasArg("ss")) ss = constrain(server.arg("ss").toInt(), SPEED_MIN, SPEED_MAX);

        Sync::updateTuningConfig(kp, ki, kd, sp, tn, ss);
        server.send(200, "text/plain", "ok");
    }

    static void handleSave() {
        LFRConfig cfgToSave;
        Sync::getConfigSnapshot(cfgToSave);

        if (NVSManager::saveConfig(cfgToSave)) {
            Sync::setStatusMessage("Configuration saved to flash memory.");
            server.send(200, "text/plain", "Settings saved to flash!");
        } else {
            Sync::setStatusMessage("Error saving settings to flash.");
            server.send(500, "text/plain", "Failed to save settings.");
        }
    }

    static void handleDefaults() {
        LFRConfig currentCfg;
        Sync::getConfigSnapshot(currentCfg);
        NVSManager::loadDefaults(currentCfg);
        Sync::setFullConfig(currentCfg);

        Sync::setStatusMessage("Factory defaults loaded into RAM. Click Save to persist.");
        server.send(200, "text/plain", "Defaults loaded into RAM.");
    }

    static void handleResetCal() {
        LFRConfig currentCfg;
        Sync::getConfigSnapshot(currentCfg);
        NVSManager::resetCalibration(currentCfg);
        Sync::requestResetCalibration();

        Sync::setStatusMessage("Calibration reset. Please recalibrate.");
        server.send(200, "text/plain", "Calibration cleared.");
    }

    // ---------------- Wi-Fi & Server Init ----------------
    void init() {
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, 0, AP_MAX_CONN);
        WiFi.setSleep(false);
        WiFi.setTxPower(WIFI_POWER_8_5dBm); // Reduce TX power to mitigate ADC noise on pin 34

        server.on("/", HTTP_GET, handleRoot);
        server.on("/status", HTTP_GET, handleStatus);
        server.on("/cal", HTTP_POST, handleCal);
        server.on("/start", HTTP_POST, handleStart);
        server.on("/pid", HTTP_POST, handlePid);
        server.on("/save", HTTP_POST, handleSave);
        server.on("/defaults", HTTP_POST, handleDefaults);
        server.on("/reset_cal", HTTP_POST, handleResetCal);

        server.begin();
        Serial.printf("[Core 0] WebServer active at http://%s/\n", WiFi.softAPIP().toString().c_str());
    }

    void taskEntry(void* parameter) {
        init();
        Serial.println("[Core 0] Wi-Fi / Web task started on Core 0.");

        for (;;) {
            server.handleClient();
            vTaskDelay(pdMS_TO_TICKS(1)); // Yield to FreeRTOS IDLE task
        }
    }

} // namespace LFRWeb

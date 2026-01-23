#ifndef WEB_UI_H
#define WEB_UI_H

#include <Arduino.h>

const char LOGIN_HTML[] PROGMEM = R"HTML(
<!doctype html><html><head>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Industrial AQM Login</title>
<style>
*{box-sizing:border-box;margin:0;padding:0;}
body{font-family:Arial,sans-serif;background:#f5f5f5;display:flex;justify-content:center;align-items:center;min-height:100vh;}
.login-container{background:#fff;padding:40px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.1);width:100%;max-width:400px;}
.login-container h1{color:#1976d2;margin-bottom:30px;text-align:center;font-size:24px;}
.login-container input{width:100%;padding:12px;margin:10px 0;border:1px solid #ddd;border-radius:4px;font-size:14px;}
.login-container button{width:100%;padding:12px;background:#17a2b8;color:#fff;border:0;border-radius:4px;font-size:16px;cursor:pointer;margin-top:10px;}
.login-container button:hover{background:#138496;}
.login-container .copyright{text-align:center;margin-top:20px;color:#666;font-size:12px;}
</style>
</head><body>
<div class="login-container">
  <h1>Industrial AQM Login</h1>
  <input type="text" id="username" placeholder="Username" value="admin">
  <input type="password" id="password" placeholder="Password" value="admin">
  <button onclick="login()">Login</button>
  <div class="copyright">Copyright © Unio Labs</div>
</div>
<script>
function login(){
  const u=document.getElementById('username').value;
  const p=document.getElementById('password').value;
  if(u==='admin' && p==='admin'){
    sessionStorage.setItem('authenticated','true');
    window.location.href='/index.html';
  }else{
    alert('Invalid credentials');
  }
}
document.getElementById('password').addEventListener('keypress',function(e){if(e.key==='Enter')login();});
</script>
</body></html>
)HTML";

const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html><html><head>
<meta charset="utf-8"/>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Industrial AQM Dashboard</title>
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700;800&display=swap" rel="stylesheet">
<style>
:root {
  --primary: #2563eb;
  --primary-hover: #1d4ed8;
  --bg-main: #f8fafc;
  --bg-sidebar: #0f172a;
  --sidebar-text: #94a3b8;
  --card-bg: rgba(255, 255, 255, 0.95);
  --text-main: #1e293b;
  --text-muted: #64748b;
  --border: rgba(226, 232, 240, 0.8);
  --shadow: 0 4px 6px -1px rgb(0 0 0 / 0.1);
  --success: #10b981;
  --warning: #f59e0b;
  --danger: #ef4444;
}

*{box-sizing:border-box;margin:0;padding:0;}
body{font-family:'Inter', sans-serif; background:var(--bg-main); color:var(--text-main); display:flex; min-height:100vh;}

/* Sidebar */
.sidebar { width:260px; background:var(--bg-sidebar); display:flex; flex-direction:column; position:fixed; top:0; bottom:0; left:0; z-index:100; transition:0.3s; }
.sidebar-header { padding:24px; color:#fff; font-size:20px; font-weight:800; border-bottom:1px solid rgba(255,255,255,0.1); }
.nav-link { padding:14px 24px; color:var(--sidebar-text); text-decoration:none; display:flex; align-items:center; gap:12px; transition:0.2s; font-size:14px; font-weight:500; }
.nav-link:hover, .nav-link.active { background:rgba(255,255,255,0.05); color:#fff; border-left:4px solid var(--primary); }
.sidebar-footer { margin-top:auto; padding:20px; color:var(--sidebar-text); font-size:12px; border-top:1px solid rgba(255,255,255,0.1); }

/* Main Content */
.main-content { margin-left:260px; flex:1; display:flex; flex-direction:column; }
.main-header { background:#fff; padding:16px 32px; border-bottom:1px solid var(--border); display:flex; justify-content:space-between; align-items:center; }
.page-title { font-size:20px; font-weight:700; }
.status-badge { background:#10b98115; color:#10b981; padding:6px 12px; border-radius:30px; font-size:12px; font-weight:600; display:flex; align-items:center; gap:6px; }
.status-dot { width:8px; height:8px; background:currentColor; border-radius:50%; }

.content-area { padding:32px; flex:1; }
.page { display:none; animation:fadeIn 0.3s ease-out; }
.page.active { display:block; }
@keyframes fadeIn { from{opacity:0;transform:translateY(10px);} to{opacity:1;transform:translateY(0);} }

/* Cards & Grid */
.grid { display:grid; grid-template-columns:repeat(auto-fit, minmax(300px, 1fr)); gap:24px; }
.card { background:#fff; padding:24px; border-radius:16px; box-shadow:var(--shadow); border:1px solid var(--border); }
.card h3 { font-size:16px; color:var(--text-muted); text-transform:uppercase; letter-spacing:0.5px; margin-bottom:16px; font-weight:700; }

/* Metrics */
.metric-value { font-size:36px; font-weight:800; color:var(--text-main); line-height:1.2; }
.metric-unit { font-size:14px; color:var(--text-muted); font-weight:500; margin-left:4px; }
.metric-sub { font-size:13px; margin-top:8px; }
.trend-up { color:var(--success); }
.trend-down { color:var(--danger); }

/* Tables */
table { width:100%; border-collapse:collapse; }
td, th { padding:12px; text-align:left; border-bottom:1px solid #f1f5f9; font-size:14px; }
th { font-weight:600; color:var(--text-muted); font-size:12px; text-transform:uppercase; }

/* Buttons & Inputs */
.btn { padding:10px 20px; border-radius:8px; border:none; background:var(--primary); color:#fff; font-weight:600; cursor:pointer; font-size:14px; transition:0.2s; }
.btn:hover { background:var(--primary-hover); transform:translateY(-1px); }
.btn-sm { padding:6px 12px; font-size:12px; }
input, select { padding:10px; border:1px solid var(--border); border-radius:8px; width:100%; outline:none; }
input:focus { border-color:var(--primary); }

/* Logs Console */
.log-console { background:#0f172a; color:#fff; padding:16px; border-radius:8px; height:300px; overflow-y:auto; font-family:monospace; font-size:12px; }
.log-entry { margin-bottom:4px; border-bottom:1px solid rgba(255,255,255,0.1); padding-bottom:4px; }
.log-INFO { color:#60a5fa; }
.log-WARN { color:#f59e0b; }
.log-ERROR { color:#ef4444; }

/* Responsive */
@media(max-width:768px){
  .sidebar{transform:translateX(-100%);}
  .sidebar.open{transform:translateX(0);}
  .main-content{margin-left:0;}
  .menu-toggle{display:block;}
}
</style>
</head><body>

<!-- Sidebar -->
<div class="sidebar" id="sidebar">
  <div class="sidebar-header">UnioLabs AQM</div>
  <nav style="flex:1; padding-top:20px;">
    <a href="#" class="nav-link active" onclick="nav('index')">Overview</a>
    <a href="#" class="nav-link" onclick="nav('sensors')">Sensor Health</a>
    <a href="#" class="nav-link" onclick="nav('network')">GSM Network</a>
    <a href="#" class="nav-link" onclick="nav('health')">Device Health</a>
    <a href="#" class="nav-link" onclick="nav('mag')">Mag Switch</a>
    <a href="#" class="nav-link" onclick="nav('logs')">Logs & Alerts</a>
  </nav>
  <div class="sidebar-footer">Firmware v1.0.0</div>
</div>

<!-- Main Content -->
<div class="main-content">
  <div class="main-header">
    <div style="display:flex; align-items:center; gap:16px;">
        <button class="btn btn-sm" onclick="document.getElementById('sidebar').classList.toggle('open')" style="display:none; @media(max-width:768px){display:block;}">☰</button>
        <div class="page-title" id="page-title">Overview</div>
    </div>
    <div class="status-badge"><span class="status-dot"></span> Online</div>
  </div>

  <div class="content-area">
    
    <!-- OVERVIEW PAGE -->
    <div id="page-index" class="page active">
        <div class="grid">
            <div class="card">
                <h3>CO2 Level</h3>
                <div class="metric-value" id="val-co2">--</div><span class="metric-unit">ppm</span>
                <div class="metric-sub" id="stat-co2">Loading...</div>
            </div>
            <div class="card">
                <h3>Temperature</h3>
                <div class="metric-value" id="val-temp">--</div><span class="metric-unit">°C</span>
            </div>
            <div class="card">
                <h3>Humidity</h3>
                <div class="metric-value" id="val-hum">--</div><span class="metric-unit">%</span>
            </div>
            <div class="card">
                <h3>Network Signal</h3>
                <div class="metric-value" id="val-signal">--</div><span class="metric-unit">dBm</span>
                <div class="metric-sub" id="val-operator">--</div>
            </div>
        </div>
        
        <div class="card" style="margin-top:24px;">
            <h3>Quick Actions</h3>
            <div style="display:flex; gap:12px;">
                <button class="btn" onclick="nav('network')">Manage Network</button>
                <button class="btn" onclick="nav('logs')">View Logs</button>
            </div>
        </div>
    </div>

    <!-- SENSOR HEALTH -->
    <div id="page-sensors" class="page">
        <div class="card">
            <h3>SCD30 Status</h3>
            <table>
                <tr><td>Status</td><td id="sensor-status"><span style="color:green">OK</span></td></tr>
                <tr><td>Last Update</td><td id="sensor-last">--</td></tr>
                <tr><td>Error Count</td><td id="sensor-errors">0</td></tr>
                <tr><td>Auto-Calibration</td><td>Enabled (ASC)</td></tr>
            </table>
        </div>
    </div>

    <!-- GSM NETWORK (INDIA) -->
    <div id="page-network" class="page">
        <div class="grid">
            <div class="card">
                <h3>Current Link</h3>
                <div class="metric-value" id="net-operator">Airtel</div>
                <div class="metric-sub">LTE Cat-M | -82 dBm</div>
                <div class="status-badge" style="width:fit-content; margin-top:10px;">Connected</div>
            </div>
            <div class="card">
                <h3>Selection Mode</h3>
                <div style="margin-bottom:16px;">
                    <label>Network Mode</label>
                    <select id="net-mode-select">
                        <option value="auto">Automatic (Recommended)</option>
                        <option value="manual">Manual Selection</option>
                    </select>
                </div>
                <button class="btn" onclick="saveNetMode()">Apply Mode</button>
            </div>
        </div>

        <div class="card" style="margin-top:24px;">
            <h3>Available Networks (India)</h3>
            <div style="display:flex; gap:12px; margin-bottom:16px;">
                <button class="btn" onclick="scanNetworks()">Scan Networks</button>
                <span id="scan-status" style="color:var(--text-muted); font-size:13px; align-self:center;"></span>
            </div>
            <table id="net-table">
                <thead><tr><th>Operator</th><th>MCC/MNC</th><th>Signal</th><th>Action</th></tr></thead>
                <tbody>
                   <!-- Injected by JS -->
                </tbody>
            </table>
        </div>
    </div>

    <!-- DEVICE HEALTH -->
    <div id="page-health" class="page">
         <div class="grid">
             <div class="card">
                 <h3>System Uptime</h3>
                 <div class="metric-value" id="sys-uptime">--</div>
                 <div class="metric-sub">Hours</div>
             </div>
             <div class="card">
                 <h3>Memory (Heap)</h3>
                 <div class="metric-value" id="sys-heap">--</div>
                 <div class="metric-sub">Bytes Free</div>
             </div>
             <div class="card">
                 <h3>Maintenance</h3>
                 <button class="btn" style="background:var(--danger);" onclick="rebootDevice()">Reboot Device</button>
             </div>
         </div>
    </div>

    <!-- LOGS -->
    <div id="page-logs" class="page">
       <div class="card">
           <h3>System Event Log</h3>
           <div class="log-console" id="log-console">
               <div class="log-entry log-INFO">[INFO] System Booted</div>
               <div class="log-entry log-INFO">[INFO] GSM Connected to Airtel</div>
               <div class="log-entry log-WARN">[WARN] High CO2 Detected: 1100ppm</div>
           </div>
       </div>
    </div>

  </div>
</div>

<script>
// Navigation
function nav(pageId) {
    document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
    document.getElementById('page-'+pageId).classList.add('active');
    document.getElementById('page-title').innerText = pageId.charAt(0).toUpperCase() + pageId.slice(1);
    
    document.querySelectorAll('.nav-link').forEach(l => l.classList.remove('active'));
    event.target.classList.add('active');
}

// Fetch Logic
async function fetchData() {
    try {
        // Sensors
        const res1 = await fetch('/api/sensors');
        const s = await res1.json();
        document.getElementById('val-co2').innerText = s.co2.toFixed(0);
        document.getElementById('val-temp').innerText = s.temp.toFixed(1);
        document.getElementById('val-hum').innerText = s.hum.toFixed(1);
        
        // GSM (Simulated for Demo if API missing)
        const res2 = await fetch('/api/gsm');
        const g = await res2.json();
        document.getElementById('val-signal').innerText = g.signal_dbm;
        document.getElementById('val-operator').innerText = g.operator;
        document.getElementById('net-operator').innerText = g.operator;
        
    } catch(e) { console.error(e); }
}

async function scanNetworks() {
    document.getElementById('scan-status').innerText = 'Scanning... (Approx 60s)';
    try {
        await fetch('/api/gsm/scan', {method:'POST'});
        // In real impl, poll for results or wait
    } catch(e) { alert('Scan failed'); }
}

async function rebootDevice() {
    if(confirm('Reboot device?')) {
        await fetch('/api/system/reboot', {method:'POST'});
        alert('Rebooting...');
    }
}

// Loop
setInterval(fetchData, 2000);
fetchData();

</script>
</body></html>
)HTML";

#endif

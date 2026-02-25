#pragma once

// =============================================================================
// Embedded Web Dashboard HTML — served at "/"
// Dark-mode responsive dashboard for aquarium automation control
// =============================================================================

const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Aquarium Controller</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
:root{--bg:#0f1117;--card:#1a1d28;--accent:#3b82f6;--accent2:#22c55e;--warn:#f59e0b;--danger:#ef4444;--text:#e2e8f0;--muted:#64748b;--border:#2d3348}
body{font-family:'Segoe UI',system-ui,sans-serif;background:var(--bg);color:var(--text);min-height:100vh}
.header{background:linear-gradient(135deg,#1e293b,#0f172a);padding:16px 20px;border-bottom:1px solid var(--border);display:flex;align-items:center;justify-content:space-between}
.header h1{font-size:1.2rem;font-weight:600}
.header .dot{width:10px;height:10px;border-radius:50%;display:inline-block;margin-right:8px}
.dot.on{background:var(--accent2);box-shadow:0 0 8px var(--accent2)}
.dot.off{background:var(--danger);box-shadow:0 0 8px var(--danger)}
.grid{display:grid;grid-template-columns:1fr;gap:12px;padding:12px;max-width:800px;margin:0 auto}
@media(min-width:600px){.grid{grid-template-columns:1fr 1fr}}
.card{background:var(--card);border-radius:12px;padding:16px;border:1px solid var(--border)}
.card h2{font-size:.85rem;text-transform:uppercase;color:var(--muted);letter-spacing:1px;margin-bottom:12px}
.val{font-size:1.8rem;font-weight:700;color:var(--accent)}
.val.warn{color:var(--warn)}
.val.danger{color:var(--danger)}
.val.ok{color:var(--accent2)}
.row{display:flex;justify-content:space-between;align-items:center;padding:4px 0;font-size:.9rem}
.row .label{color:var(--muted)}
.badge{padding:2px 8px;border-radius:6px;font-size:.75rem;font-weight:600}
.badge.on{background:rgba(34,197,94,.15);color:var(--accent2)}
.badge.off{background:rgba(100,116,139,.15);color:var(--muted)}
.badge.alert{background:rgba(239,68,68,.15);color:var(--danger)}
.btn{width:100%;padding:10px;border:none;border-radius:8px;font-weight:600;font-size:.9rem;cursor:pointer;margin-top:6px;transition:all .2s}
.btn-primary{background:var(--accent);color:#fff}
.btn-primary:hover{background:#2563eb}
.btn-danger{background:var(--danger);color:#fff}
.btn-danger:hover{background:#dc2626}
.btn-warn{background:var(--warn);color:#000}
.btn-warn:hover{background:#d97706}
.btn-ok{background:var(--accent2);color:#000}
.btn-ok:hover{background:#16a34a}
.btn:disabled{opacity:.5;cursor:not-allowed}
.input-row{display:flex;gap:8px;align-items:center;margin-top:6px}
.input-row input,.input-row select{flex:1;padding:8px;border-radius:6px;border:1px solid var(--border);background:#131620;color:var(--text);font-size:.9rem}
.input-row input:focus,.input-row select:focus{outline:none;border-color:var(--accent)}
.stock-bar{height:6px;border-radius:3px;background:#2d3348;margin-top:4px;overflow:hidden}
.stock-fill{height:100%;border-radius:3px;background:var(--accent2);transition:width .5s}
.stock-fill.low{background:var(--warn)}
.stock-fill.empty{background:var(--danger)}
.time-display{font-size:1rem;color:var(--accent);font-family:monospace}
.emergency-banner{background:rgba(239,68,68,.1);border:2px solid var(--danger);border-radius:12px;padding:16px;text-align:center;display:none}
.emergency-banner.active{display:block;animation:pulse 2s infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.7}}
</style>
</head>
<body>
<div class="header">
  <div><span class="dot off" id="wifiDot"></span><h1 style="display:inline">🐠 Aquarium Controller</h1></div>
  <span class="time-display" id="sysTime">--:--:--</span>
</div>

<div class="grid">
  <!-- Emergency Banner -->
  <div class="emergency-banner" id="emergencyBanner" style="grid-column:1/-1">
    ⚠️ <strong>EMERGÊNCIA</strong> — Sensor detectou risco!
  </div>

  <!-- Water Level -->
  <div class="card">
    <h2>💧 Nível da Água</h2>
    <div class="val" id="waterLevel">--</div>
    <div style="font-size:.8rem;color:var(--muted)">cm (distância ultrassônico)</div>
    <div class="row" style="margin-top:8px">
      <span class="label">Óptico</span>
      <span class="badge off" id="optical">--</span>
    </div>
    <div class="row">
      <span class="label">Boia</span>
      <span class="badge off" id="float">--</span>
    </div>
  </div>

  <!-- TPA Control -->
  <div class="card">
    <h2>🔄 TPA (Troca Parcial)</h2>
    <div class="row">
      <span class="label">Estado</span>
      <span class="badge off" id="tpaState">IDLE</span>
    </div>
    <div class="row">
      <span class="label">Canister</span>
      <span class="badge off" id="canister">OFF</span>
    </div>
    <button class="btn btn-primary" onclick="api('POST','/api/tpa/start')">▶ Iniciar TPA</button>
    <button class="btn btn-danger" onclick="api('POST','/api/tpa/abort')">⏹ Abortar TPA</button>
  </div>

  <!-- Maintenance -->
  <div class="card">
    <h2>🔧 Manutenção</h2>
    <div class="row">
      <span class="label">Modo</span>
      <span class="badge off" id="maintenance">OFF</span>
    </div>
    <button class="btn btn-warn" onclick="api('POST','/api/maintenance/toggle')">🔧 Alternar Manutenção</button>
    <button class="btn btn-danger" style="margin-top:8px" onclick="api('POST','/api/emergency/stop')">🚨 Parada de Emergência</button>
  </div>

  <!-- WiFi Config -->
  <div class="card">
    <h2>📶 Configurar WiFi</h2>
    <form onsubmit="saveWiFi(event)">
      <div class="input-row" style="margin-bottom:8px">
        <span class="label" style="width:50px">SSID</span>
        <input type="text" id="wifiSsid" required style="flex-grow:1">
      </div>
      <div class="input-row" style="margin-bottom:8px">
        <span class="label" style="width:50px">Senha</span>
        <input type="password" id="wifiPass" required style="flex-grow:1">
      </div>
      <button type="submit" class="btn btn-primary" style="margin-top:8px">💾 Salvar e Reiniciar</button>
    </form>
  </div>

  <!-- Schedule -->
  <div class="card">
    <h2>📅 Agendamento</h2>
    <div class="row"><span class="label">Fertilização</span><span id="fertSched">--:--</span></div>
    <div class="input-row">
      <input type="number" id="fertH" min="0" max="23" placeholder="HH" style="width:60px">
      <span>:</span>
      <input type="number" id="fertM" min="0" max="59" placeholder="MM" style="width:60px">
      <button class="btn btn-primary" style="width:auto;margin:0;padding:8px 16px" onclick="setSchedule('fert')">✓</button>
    </div>
    <div class="row" style="margin-top:12px"><span class="label">TPA</span><span id="tpaSched">D- --:--</span></div>
    <div class="input-row">
      <select id="tpaD" style="width:50px"><option value="0">Dom</option><option value="1">Seg</option><option value="2">Ter</option><option value="3">Qua</option><option value="4">Qui</option><option value="5">Sex</option><option value="6">Sáb</option></select>
      <input type="number" id="tpaH" min="0" max="23" placeholder="HH" style="width:55px">
      <span>:</span>
      <input type="number" id="tpaM" min="0" max="59" placeholder="MM" style="width:55px">
      <button class="btn btn-primary" style="width:auto;margin:0;padding:8px 16px" onclick="setSchedule('tpa')">✓</button>
    </div>
  </div>

  <!-- Fertilizer Doses -->
  <div class="card" style="grid-column:1/-1">
    <h2>🧪 Fertilizantes</h2>
    <div id="fertCards" style="display:grid;grid-template-columns:repeat(auto-fit,minmax(130px,1fr));gap:8px">
    </div>
  </div>
</div>

<script>
const $ = id => document.getElementById(id);
const channels = ['CH1','CH2','CH3','CH4','Prime'];

// Build fertilizer cards
channels.forEach((ch, i) => {
  $('fertCards').innerHTML += `
    <div style="background:var(--bg);border-radius:8px;padding:10px">
      <div style="font-size:.8rem;color:var(--muted)">${ch}</div>
      <div style="font-size:1.2rem;font-weight:700" id="stock${i}">--</div>
      <div class="stock-bar"><div class="stock-fill" id="bar${i}" style="width:0%"></div></div>
      <div class="input-row" style="margin-top:6px">
        <input type="number" id="dose${i}" step="0.5" min="0" max="50" placeholder="mL" style="width:55px;font-size:.8rem;padding:4px">
        <button class="btn btn-primary" style="width:auto;margin:0;padding:4px 8px;font-size:.75rem" onclick="setDose(${i})">Dose</button>
      </div>
      <div class="input-row">
        <input type="number" id="rst${i}" min="0" max="1000" placeholder="mL" style="width:55px;font-size:.8rem;padding:4px">
        <button class="btn btn-ok" style="width:auto;margin:0;padding:4px 8px;font-size:.75rem" onclick="resetStock(${i})">Reset</button>
      </div>
    </div>`;
});

function api(method, url, body) {
  fetch(url, {method, headers:{'Content-Type':'application/json'}, body: body ? JSON.stringify(body) : undefined})
    .then(r => r.json()).then(d => { if(d.error) alert(d.error); })
    .catch(e => console.error(e));
}

async function saveWiFi(e) {
  e.preventDefault();
  const fd = new URLSearchParams();
  fd.append('ssid', $('wifiSsid').value);
  fd.append('pass', $('wifiPass').value);
  try {
    const r = await fetch('/api/wifi', { method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:fd });
    if(r.ok) alert('WiFi configurado! O sistema reiniciará para conectar.');
    else alert('Erro ao salvar WiFi.');
  } catch(err) { alert('Erro de comunicação.'); }
}

function setSchedule(type) {
  if (type === 'fert') {
    api('POST', '/api/schedule', {fertHour: +$('fertH').value, fertMinute: +$('fertM').value});
  } else {
    api('POST', '/api/schedule', {tpaDay: +$('tpaD').value, tpaHour: +$('tpaH').value, tpaMinute: +$('tpaM').value});
  }
}

function setDose(ch) {
  const ml = +$('dose'+ch).value;
  if (ml > 0) api('POST', '/api/dose', {channel: ch, ml: ml});
}

function resetStock(ch) {
  const ml = +$('rst'+ch).value;
  if (ml > 0) api('POST', '/api/stock/reset', {channel: ch, ml: ml});
}

function updateUI(d) {
  // Time
  if(d.time) $('sysTime').textContent = d.time;

  // Water
  if(d.waterLevel !== undefined) {
    $('waterLevel').textContent = d.waterLevel.toFixed(1);
    $('waterLevel').className = 'val' + (d.waterLevel < 5 ? ' danger' : d.waterLevel < 10 ? ' warn' : '');
  }

  // Sensors
  setBadge('optical', d.optical, 'HIGH', 'low');
  setBadge('float', d.float, 'FULL', 'empty');
  setBadge('canister', d.canister, 'ON', 'OFF');
  setBadge('tpaState', d.tpaState !== 'IDLE', d.tpaState, d.tpaState);
  setBadge('maintenance', d.maintenance, 'ON', 'OFF');

  // Emergency
  const eb = $('emergencyBanner');
  if(d.emergency) { eb.classList.add('active'); } else { eb.classList.remove('active'); }

  // Schedule
  if(d.fertHour !== undefined) $('fertSched').textContent = pad(d.fertHour)+':'+pad(d.fertMinute);
  if(d.tpaDay !== undefined) {
    const days = ['Dom','Seg','Ter','Qua','Qui','Sex','Sáb'];
    $('tpaSched').textContent = days[d.tpaDay]+' '+pad(d.tpaHour)+':'+pad(d.tpaMinute);
  }

  // Stocks
  if(d.stocks) d.stocks.forEach((s, i) => {
    $('stock'+i).textContent = s.stock.toFixed(0) + ' mL';
    const pct = Math.min(100, (s.stock / 500) * 100);
    const bar = $('bar'+i);
    bar.style.width = pct + '%';
    bar.className = 'stock-fill' + (pct < 10 ? ' empty' : pct < 20 ? ' low' : '');
  });

  // WiFi indicator
  $('wifiDot').className = 'dot on';
}

function setBadge(id, cond, onText, offText) {
  const el = $(id);
  if(el && cond !== undefined) {
    el.textContent = cond ? onText : offText;
    el.className = 'badge ' + (cond ? 'on' : 'off');
  }
}

function pad(n) { return String(n).padStart(2, '0'); }

// SSE for real-time updates
const evtSource = new EventSource('/events');
evtSource.addEventListener('status', e => {
  try { updateUI(JSON.parse(e.data)); } catch(err) { console.error(err); }
});
evtSource.onerror = () => { $('wifiDot').className = 'dot off'; };

// Initial fetch
fetch('/api/status').then(r=>r.json()).then(updateUI).catch(()=>{});
</script>
</body>
</html>
)rawliteral";

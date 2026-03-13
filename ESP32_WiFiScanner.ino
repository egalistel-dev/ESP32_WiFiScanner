/*
 * ESP32 WiFi Scanner (Web Interface)
 * Version universelle — aucun SSID codé en dur
 * 
 * 1. Flasher ce sketch sur l'ESP32
 * 2. Connecter votre téléphone au WiFi "ESP32-WiFiScanner"
 *    Mot de passe : wifiscan32
 * 3. Ouvrir http://192.168.4.1 dans le navigateur
 * 4. Scanner, cliquer sur un réseau pour le détail
 * 5. Tester antenne 1 → comparer antenne 2
 * 
 * egamaker.be — MIT License
 */

#include <WiFi.h>
#include <WebServer.h>

#define AP_SSID  "ESP32-WiFiScanner"
#define AP_PASS  "wifiscan32"

WebServer server(80);

// ── Scan JSON ─────────────────────────────────────────────
String doScan() {
  int n = WiFi.scanNetworks();
  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i > 0) json += ",";
    String ssid = WiFi.SSID(i);
    ssid.replace("\\", "\\\\");
    ssid.replace("\"", "\\\"");
    json += "{";
    json += "\"ssid\":\"" + ssid + "\",";
    json += "\"rssi\":"  + String(WiFi.RSSI(i))    + ",";
    json += "\"ch\":"    + String(WiFi.channel(i)) + ",";
    json += "\"enc\":"   + String(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? 0 : 1);
    json += "}";
  }
  json += "]";
  WiFi.scanDelete();
  return json;
}

// ── HTML ──────────────────────────────────────────────────
void handleRoot() {
  String html = R"rawhtml(<!DOCTYPE html>
<html lang="fr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>ESP32 WiFi Scanner</title>
<style>
@import url('https://fonts.googleapis.com/css2?family=Space+Mono:wght@400;700&family=DM+Sans:wght@300;400;500;600&display=swap');
:root{--bg:#0d0f14;--surface:#161920;--surface2:#1e2029;--border:#252830;--accent:#f0a500;--accent2:#e05c00;--text:#e8eaf0;--muted:#6b7080;--good:#30d68a;--mid:#f0a500;--warn:#f07030;--bad:#f04060}
*{box-sizing:border-box;margin:0;padding:0}
body{background:var(--bg);color:var(--text);font-family:'DM Sans',sans-serif;min-height:100vh;padding:0 0 60px}

/* Header */
.header{text-align:center;padding:28px 16px 20px;border-bottom:1px solid var(--border)}
.logo{font-family:'Space Mono',monospace;font-size:20px;font-weight:700;background:linear-gradient(135deg,#f0a500,#e05c00);-webkit-background-clip:text;-webkit-text-fill-color:transparent}
.subtitle{font-size:10px;color:var(--muted);letter-spacing:2px;text-transform:uppercase;margin-top:3px}

/* Controls */
.controls{padding:16px;display:flex;flex-direction:column;gap:10px}
.btn-scan{width:100%;padding:14px;border-radius:12px;border:none;background:linear-gradient(135deg,var(--accent),var(--accent2));color:#000;font-family:'DM Sans',sans-serif;font-size:15px;font-weight:700;cursor:pointer;display:flex;align-items:center;justify-content:center;gap:8px;transition:opacity .2s}
.btn-scan:active{opacity:.8}
.btn-scan:disabled{opacity:.5}

.autoscan-row{display:flex;align-items:center;justify-content:space-between;background:var(--surface);border:1px solid var(--border);border-radius:12px;padding:12px 16px}
.autoscan-label{font-size:14px;font-weight:500}
.autoscan-hint{font-size:11px;color:var(--muted);margin-top:2px}
.toggle{width:44px;height:24px;background:var(--border);border-radius:12px;position:relative;cursor:pointer;transition:background .3s;flex-shrink:0}
.toggle.on{background:var(--accent)}
.toggle::after{content:'';position:absolute;width:18px;height:18px;background:#fff;border-radius:50%;top:3px;left:3px;transition:left .3s;box-shadow:0 1px 3px #0006}
.toggle.on::after{left:23px}

.status{text-align:center;font-size:11px;color:var(--muted);font-family:'Space Mono',monospace;padding:4px 0}

/* List */
.list-header{padding:0 16px 8px;display:flex;align-items:center;justify-content:space-between}
.list-title{font-family:'Space Mono',monospace;font-size:10px;letter-spacing:2px;text-transform:uppercase;color:var(--muted)}
.list-count{font-family:'Space Mono',monospace;font-size:10px;color:var(--accent)}

.net-list{display:flex;flex-direction:column;gap:6px;padding:0 16px}

.net-card{background:var(--surface);border:1px solid var(--border);border-radius:12px;padding:13px 14px;display:flex;align-items:center;gap:12px;cursor:pointer;transition:border-color .2s,background .2s;-webkit-tap-highlight-color:transparent}
.net-card:active,.net-card:hover{border-color:rgba(240,165,0,.35);background:rgba(240,165,0,.03)}

.signal-bars{display:flex;align-items:flex-end;gap:2px;height:18px;flex-shrink:0}
.bar{width:4px;border-radius:2px;background:var(--border)}
.bar-1{height:4px}.bar-2{height:7px}.bar-3{height:11px}.bar-4{height:15px}.bar-5{height:18px}
.bar.on{background:var(--good)}
.bar.on-mid{background:var(--mid)}
.bar.on-warn{background:var(--warn)}
.bar.on-bad{background:var(--bad)}

.net-info{flex:1;min-width:0}
.net-ssid{font-size:14px;font-weight:500;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.net-meta{font-size:11px;color:var(--muted);margin-top:2px;font-family:'Space Mono',monospace}

.net-right{text-align:right;flex-shrink:0}
.net-rssi{font-family:'Space Mono',monospace;font-size:13px;font-weight:700}
.net-pct{font-size:10px;color:var(--muted);font-family:'Space Mono',monospace;margin-top:1px}

.empty{text-align:center;padding:40px 16px;color:var(--muted);font-size:13px}

/* Spinner */
.spin{display:inline-block;animation:spin .9s linear infinite}
@keyframes spin{to{transform:rotate(360deg)}}

/* ── Detail panel (bottom sheet) ── */
.overlay{position:fixed;inset:0;background:#0009;z-index:10;opacity:0;pointer-events:none;transition:opacity .25s}
.overlay.show{opacity:1;pointer-events:all}

.detail{position:fixed;bottom:0;left:0;right:0;background:var(--surface2);border-top:1px solid var(--border);border-radius:20px 20px 0 0;z-index:11;padding:0 20px 40px;transform:translateY(100%);transition:transform .3s cubic-bezier(.32,.72,0,1);max-height:85vh;overflow-y:auto}
.detail.show{transform:translateY(0)}

.detail-handle{width:36px;height:4px;background:var(--border);border-radius:2px;margin:12px auto 20px}
.detail-ssid{font-family:'Space Mono',monospace;font-size:18px;font-weight:700;text-align:center;margin-bottom:4px;word-break:break-all}
.detail-enc{font-size:11px;color:var(--muted);text-align:center;margin-bottom:24px;font-family:'Space Mono',monospace}

.big-rssi{font-family:'Space Mono',monospace;font-size:64px;font-weight:700;text-align:center;line-height:1;transition:color .4s}
.big-unit{font-size:14px;color:var(--muted);text-align:center;margin-bottom:8px}
.big-quality{font-size:16px;font-weight:600;text-align:center;margin-bottom:20px}

.bar-wrap{background:var(--bg);border-radius:8px;height:12px;overflow:hidden;margin-bottom:20px}
.bar-fill{height:100%;border-radius:8px;transition:width .5s,background .5s}

.detail-stats{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:20px}
.stat-box{background:var(--bg);border:1px solid var(--border);border-radius:10px;padding:12px;text-align:center}
.stat-label{font-size:10px;color:var(--muted);letter-spacing:1px;text-transform:uppercase;margin-bottom:4px;font-family:'Space Mono',monospace}
.stat-val{font-family:'Space Mono',monospace;font-size:18px;font-weight:700}

.history-title{font-size:10px;color:var(--muted);letter-spacing:2px;text-transform:uppercase;font-family:'Space Mono',monospace;margin-bottom:8px}
.history-dots{display:flex;gap:5px;flex-wrap:wrap;margin-bottom:20px}
.hist-dot{width:12px;height:12px;border-radius:50%;cursor:default}

.btn-close{width:100%;padding:13px;border-radius:12px;border:1px solid var(--border);background:transparent;color:var(--muted);font-family:'DM Sans',sans-serif;font-size:14px;cursor:pointer}
.btn-close:active{background:var(--border)}

/* Scan btn inside detail */
.btn-rescan{width:100%;padding:12px;border-radius:12px;border:none;background:rgba(240,165,0,.1);border:1px solid rgba(240,165,0,.3);color:var(--accent);font-family:'DM Sans',sans-serif;font-size:14px;font-weight:600;cursor:pointer;margin-bottom:10px}
</style>
</head>
<body>

<div class="header">
  <div class="logo">WiFi Scanner</div>
  <div class="subtitle">ESP32 — Antenna Test Tool</div>
</div>

<div class="controls">
  <button class="btn-scan" id="btnScan" onclick="scan()">📡 Scanner les réseaux WiFi</button>
  <div class="autoscan-row">
    <div>
      <div class="autoscan-label">Scan automatique</div>
      <div class="autoscan-hint">Rafraîchit toutes les 4 secondes</div>
    </div>
    <div class="toggle" id="toggleAuto" onclick="toggleAuto()"></div>
  </div>
  <div class="status" id="status">Appuyez sur Scanner pour démarrer</div>
</div>

<div class="list-header">
  <div class="list-title">Réseaux détectés</div>
  <div class="list-count" id="listCount"></div>
</div>
<div class="net-list" id="netList">
  <div class="empty">📡 Aucun scan effectué</div>
</div>

<!-- Overlay + detail panel -->
<div class="overlay" id="overlay" onclick="closeDetail()"></div>
<div class="detail" id="detail">
  <div class="detail-handle"></div>
  <div class="detail-ssid" id="dSsid"></div>
  <div class="detail-enc" id="dEnc"></div>
  <div class="big-rssi" id="dRssi"></div>
  <div class="big-unit">dBm</div>
  <div class="big-quality" id="dQuality"></div>
  <div class="bar-wrap"><div class="bar-fill" id="dBar"></div></div>
  <div class="detail-stats">
    <div class="stat-box">
      <div class="stat-label">Canal</div>
      <div class="stat-val" id="dCh">—</div>
    </div>
    <div class="stat-box">
      <div class="stat-label">Qualité</div>
      <div class="stat-val" id="dPct">—</div>
    </div>
  </div>
  <div class="history-title">Historique des mesures</div>
  <div class="history-dots" id="dHistory"></div>
  <button class="btn-rescan" onclick="rescanDetail()">🔄 Rescanner ce réseau</button>
  <button class="btn-close" onclick="closeDetail()">✕ Fermer</button>
</div>

<script>
let autoInterval = null;
let autoOn = false;
let lastNets = [];
let selectedSsid = null;
let selectedHistory = [];

// ── Colors & helpers ──────────────────────────────────────
function col(r) {
  return r >= -60 ? '#30d68a' : r >= -70 ? '#f0a500' : r >= -80 ? '#f07030' : '#f04060';
}
function pct(r) {
  return Math.min(100, Math.max(0, 2 * (r + 100)));
}
function qual(r) {
  return r >= -50 ? '🟢 Excellent' : r >= -60 ? '🟢 Bon' : r >= -70 ? '🟡 Moyen' : r >= -80 ? '🟠 Faible' : '🔴 Mauvais';
}
function qualShort(r) {
  return r >= -50 ? 'Excellent' : r >= -60 ? 'Bon' : r >= -70 ? 'Moyen' : r >= -80 ? 'Faible' : 'Mauvais';
}

function barsHtml(r) {
  const lit = r >= -50 ? 5 : r >= -60 ? 4 : r >= -70 ? 3 : r >= -80 ? 2 : 1;
  const c = r >= -60 ? 'on' : r >= -70 ? 'on-mid' : r >= -80 ? 'on-warn' : 'on-bad';
  let h = '';
  for (let i = 1; i <= 5; i++) h += `<div class="bar bar-${i} ${i <= lit ? c : ''}"></div>`;
  return h;
}

// ── Toggle auto ───────────────────────────────────────────
function toggleAuto() {
  autoOn = !autoOn;
  document.getElementById('toggleAuto').classList.toggle('on', autoOn);
  if (autoOn) { scan(); autoInterval = setInterval(scan, 4000); }
  else clearInterval(autoInterval);
}

// ── Scan ──────────────────────────────────────────────────
async function scan() {
  const btn = document.getElementById('btnScan');
  btn.disabled = true;
  btn.innerHTML = '<span class="spin">⟳</span> Scan en cours...';
  document.getElementById('status').textContent = 'Scan en cours...';
  try {
    const res = await fetch('/scan');
    lastNets = await res.json();
    render(lastNets);
    // Refresh detail panel if open
    if (selectedSsid) {
      const found = lastNets.find(n => n.ssid === selectedSsid);
      if (found) updateDetail(found, false);
    }
  } catch(e) {
    document.getElementById('status').textContent = '❌ Erreur de connexion';
  }
  btn.disabled = false;
  btn.innerHTML = '📡 Scanner les réseaux WiFi';
}

// ── Render list ───────────────────────────────────────────
function render(nets) {
  const now = new Date().toLocaleTimeString('fr-FR');
  document.getElementById('status').textContent = `Màj : ${now}`;
  document.getElementById('listCount').textContent = nets.length + ' réseau(x)';

  if (!nets.length) {
    document.getElementById('netList').innerHTML = '<div class="empty">Aucun réseau détecté</div>';
    return;
  }

  nets.sort((a, b) => b.rssi - a.rssi);

  document.getElementById('netList').innerHTML = nets.map(n => `
    <div class="net-card" onclick="openDetail('${n.ssid.replace(/'/g,"\\'")}')">
      <div class="signal-bars">${barsHtml(n.rssi)}</div>
      <div class="net-info">
        <div class="net-ssid">${n.ssid || '<span style="color:var(--muted);font-style:italic">Réseau masqué</span>'}</div>
        <div class="net-meta">Canal ${n.ch} &nbsp;·&nbsp; ${n.enc ? '🔒 Sécurisé' : '🔓 Ouvert'}</div>
      </div>
      <div class="net-right">
        <div class="net-rssi" style="color:${col(n.rssi)}">${n.rssi}</div>
        <div class="net-pct">${pct(n.rssi)}%</div>
      </div>
    </div>
  `).join('');
}

// ── Detail panel ──────────────────────────────────────────
function openDetail(ssid) {
  const net = lastNets.find(n => n.ssid === ssid);
  if (!net) return;
  selectedSsid = ssid;
  selectedHistory = [net.rssi];
  updateDetail(net, true);
  document.getElementById('overlay').classList.add('show');
  document.getElementById('detail').classList.add('show');
}

function updateDetail(net, reset) {
  if (reset) selectedHistory = [];
  selectedHistory.push(net.rssi);
  if (selectedHistory.length > 15) selectedHistory.shift();

  const c = col(net.rssi);
  const p = pct(net.rssi);

  document.getElementById('dSsid').textContent    = net.ssid || '(Réseau masqué)';
  document.getElementById('dEnc').textContent     = net.enc ? '🔒 Réseau sécurisé' : '🔓 Réseau ouvert';
  document.getElementById('dRssi').textContent    = net.rssi;
  document.getElementById('dRssi').style.color    = c;
  document.getElementById('dQuality').textContent = qual(net.rssi);
  document.getElementById('dQuality').style.color = c;
  document.getElementById('dBar').style.width     = p + '%';
  document.getElementById('dBar').style.background = c;
  document.getElementById('dCh').textContent      = net.ch;
  document.getElementById('dPct').textContent     = p + '%';
  document.getElementById('dPct').style.color     = c;

  document.getElementById('dHistory').innerHTML =
    selectedHistory.map(r =>
      `<div class="hist-dot" style="background:${col(r)}" title="${r} dBm (${qualShort(r)})"></div>`
    ).join('');
}

function closeDetail() {
  selectedSsid = null;
  selectedHistory = [];
  document.getElementById('overlay').classList.remove('show');
  document.getElementById('detail').classList.remove('show');
}

async function rescanDetail() {
  await scan();
  const found = lastNets.find(n => n.ssid === selectedSsid);
  if (found) updateDetail(found, false);
}
</script>
</body>
</html>)rawhtml";

  server.send(200, "text/html; charset=utf-8", html);
}

void handleScan() {
  server.send(200, "application/json", doScan());
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nESP32 WiFi Scanner");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("AP  : %s\n", AP_SSID);
  Serial.printf("PWD : %s\n", AP_PASS);
  Serial.printf("IP  : %s\n", WiFi.softAPIP().toString().c_str());
  server.on("/",     handleRoot);
  server.on("/scan", handleScan);
  server.begin();
  Serial.println("Serveur démarré !");
}

void loop() {
  server.handleClient();
}

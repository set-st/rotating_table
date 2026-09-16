#pragma once
#include <Arduino.h>

const char PAGE_INDEX[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="ru">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Поворотный стол ESP32</title>
  <style>
    :root {
      --bg: #0d1117;
      --card-bg: rgba(22, 27, 34, 0.85);
      --border: #30363d;
      --text: #e6edf3;
      --text-muted: #8b949e;
      --primary: #58a6ff;
      --primary-hover: #388bfd;
      --success: #238636;
      --success-hover: #2ea043;
      --danger: #da3633;
      --danger-hover: #f85149;
      --warning: #d29922;
      --accent: #8957e5;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      background: var(--bg);
      background-image: radial-gradient(circle at 50% 0%, #1f2430 0%, #0d1117 75%);
      color: var(--text);
      min-height: 100vh;
      display: flex;
      justify-content: center;
      padding: 24px 16px;
    }
    .container {
      width: 100%;
      max-width: 680px;
      display: flex;
      flex-direction: column;
      gap: 20px;
    }
    header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding-bottom: 12px;
      border-bottom: 1px solid var(--border);
    }
    header h1 {
      font-size: 1.5rem;
      font-weight: 600;
      background: linear-gradient(90deg, #58a6ff, #bc8cff);
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
    }
    .badge {
      padding: 4px 12px;
      border-radius: 20px;
      font-size: 0.75rem;
      font-weight: 600;
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }
    .badge-idle { background: #23863633; color: #3fb950; border: 1px solid #238636; }
    .badge-moving { background: #1f6feb33; color: #58a6ff; border: 1px solid #1f6feb; animation: pulse 1.5s infinite; }
    .badge-homing { background: #d2992233; color: #e3b341; border: 1px solid #d29922; animation: pulse 1.5s infinite; }
    .badge-stopped, .badge-error { background: #da363333; color: #f85149; border: 1px solid #da3633; }
    @keyframes pulse {
      0%, 100% { opacity: 1; }
      50% { opacity: 0.6; }
    }
    .card {
      background: var(--card-bg);
      border: 1px solid var(--border);
      border-radius: 12px;
      padding: 20px;
      box-shadow: 0 8px 24px rgba(0,0,0,0.4);
      backdrop-filter: blur(10px);
    }
    .card-title {
      font-size: 1rem;
      font-weight: 600;
      margin-bottom: 16px;
      color: var(--text-muted);
      text-transform: uppercase;
      letter-spacing: 0.5px;
      display: flex;
      align-items: center;
      justify-content: space-between;
    }
    .status-grid {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 16px;
    }
    .status-item {
      background: rgba(13, 17, 23, 0.6);
      padding: 14px;
      border-radius: 8px;
      border: 1px solid rgba(48, 54, 61, 0.5);
    }
    .status-item .label {
      font-size: 0.8rem;
      color: var(--text-muted);
      margin-bottom: 4px;
    }
    .status-item .value {
      font-size: 1.6rem;
      font-weight: 700;
      color: var(--text);
    }
    .status-item .sub {
      font-size: 0.75rem;
      color: var(--text-muted);
      margin-top: 2px;
    }
    .btn-row {
      display: flex;
      gap: 10px;
      margin-top: 14px;
      flex-wrap: wrap;
    }
    button {
      background: var(--primary);
      color: #fff;
      border: none;
      border-radius: 6px;
      padding: 10px 18px;
      font-size: 0.9rem;
      font-weight: 600;
      cursor: pointer;
      transition: all 0.2s;
      display: inline-flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
    }
    button:hover { background: var(--primary-hover); transform: translateY(-1px); }
    button:active { transform: translateY(0); }
    button.btn-success { background: var(--success); }
    button.btn-success:hover { background: var(--success-hover); }
    button.btn-danger { background: var(--danger); }
    button.btn-danger:hover { background: var(--danger-hover); }
    button.btn-secondary { background: #21262d; border: 1px solid var(--border); color: var(--text); }
    button.btn-secondary:hover { background: #30363d; }
    button:disabled { opacity: 0.4; cursor: not-allowed; transform: none; }
    .form-group {
      margin-bottom: 14px;
    }
    .form-group label {
      display: block;
      font-size: 0.85rem;
      color: var(--text-muted);
      margin-bottom: 6px;
    }
    .input-row {
      display: flex;
      gap: 10px;
      align-items: center;
    }
    input[type="number"], input[type="range"] {
      background: #0d1117;
      border: 1px solid var(--border);
      border-radius: 6px;
      color: var(--text);
      padding: 10px 14px;
      font-size: 1rem;
      width: 100%;
    }
    input[type="range"] {
      padding: 0;
      height: 6px;
      cursor: pointer;
    }
    .presets {
      display: flex;
      gap: 6px;
      flex-wrap: wrap;
      margin-top: 8px;
    }
    .presets button {
      padding: 6px 10px;
      font-size: 0.8rem;
    }
    .toggle-group {
      display: flex;
      background: #0d1117;
      border: 1px solid var(--border);
      border-radius: 6px;
      overflow: hidden;
      margin-bottom: 14px;
    }
    .toggle-btn {
      flex: 1;
      padding: 8px;
      text-align: center;
      font-size: 0.85rem;
      font-weight: 500;
      color: var(--text-muted);
      cursor: pointer;
      transition: all 0.2s;
    }
    .toggle-btn.active {
      background: #21262d;
      color: var(--primary);
      font-weight: 600;
    }
    .indicator-dot {
      display: inline-block;
      width: 8px;
      height: 8px;
      border-radius: 50%;
      margin-right: 6px;
    }
    .dot-on { background: #3fb950; box-shadow: 0 0 6px #3fb950; }
    .dot-off { background: #8b949e; }
    details {
      background: rgba(13, 17, 23, 0.4);
      border: 1px solid var(--border);
      border-radius: 8px;
      padding: 12px;
      font-size: 0.85rem;
    }
    summary {
      cursor: pointer;
      font-weight: 600;
      color: var(--primary);
    }
    pre {
      background: #0d1117;
      border: 1px solid var(--border);
      border-radius: 6px;
      padding: 10px;
      margin-top: 10px;
      overflow-x: auto;
      font-size: 0.8rem;
      color: #79c0ff;
    }
    #toast {
      position: fixed;
      bottom: 20px;
      right: 20px;
      padding: 10px 18px;
      border-radius: 6px;
      background: #21262d;
      border: 1px solid var(--border);
      color: var(--text);
      font-size: 0.85rem;
      display: none;
      box-shadow: 0 4px 12px rgba(0,0,0,0.5);
      z-index: 100;
    }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>Вращающийся стол ESP32</h1>
      <span id="badgeState" class="badge badge-idle">IDLE</span>
    </header>

    <!-- LIVE STATUS CARD -->
    <div class="card">
      <div class="card-title">
        <span>Текущее состояние</span>
        <span id="txtError" style="color: var(--danger); font-size: 0.8rem; font-weight: normal;"></span>
      </div>
      <div class="status-grid">
        <div class="status-item">
          <div class="label">Текущий угол</div>
          <div class="value"><span id="valAngle">0.0</span>°</div>
          <div class="sub">Цель: <span id="valTarget">0.0</span>°</div>
        </div>
        <div class="status-item">
          <div class="label">Скорость</div>
          <div class="value"><span id="valSpeed">0</span> <small style="font-size:0.9rem">°/с</small></div>
          <div class="sub">
            <span id="dotHomed" class="indicator-dot dot-off"></span><span id="txtHomed">Не откалиброван</span>
          </div>
        </div>
      </div>
      <div style="margin-top: 12px; font-size: 0.85rem; color: var(--text-muted);">
        Концевик: <span id="dotEndstop" class="indicator-dot dot-off"></span>
        <span id="txtEndstop" style="color: var(--text);">Разомкнут</span>
      </div>
    </div>

    <!-- HOMING & ZERO CARD -->
    <div class="card">
      <div class="card-title">Калибровка и начало координат</div>
      <p style="font-size: 0.85rem; color: var(--text-muted); margin-bottom: 12px;">
        Поиск концевика поворотом влево до упора для выставления нуля (0.0°).
      </p>
      <div class="btn-row">
        <button id="btnHome" class="btn-success" onclick="startHoming()">
          <svg width="16" height="16" fill="currentColor" viewBox="0 0 16 16">
            <path d="M8.707 1.5a1 1 0 0 0-1.414 0L.646 8.146a.5.5 0 0 0 .708.708L2 8.207V13.5A1.5 1.5 0 0 0 3.5 15h9a1.5 1.5 0 0 0 1.5-1.5V8.207l.646.647a.5.5 0 0 0 .708-.708L13 5.793V2.5a.5.5 0 0 0-.5-.5h-1a.5.5 0 0 0-.5.5v1.293L8.707 1.5Z"/>
          </svg>
          Найти концевик (Home)
        </button>
        <button class="btn-secondary" onclick="setZero()">Сбросить угол в 0°</button>
        <button class="btn-danger" onclick="stopMotor()">СТОП</button>
      </div>
    </div>

    <!-- MOVE CONTROL CARD -->
    <div class="card">
      <div class="card-title">Управление поворотом</div>
      
      <div class="toggle-group">
        <div id="btnModeAbs" class="toggle-btn active" onclick="setRelativeMode(false)">Абсолютный угол</div>
        <div id="btnModeRel" class="toggle-btn" onclick="setRelativeMode(true)">Относительный шаг (Δ)</div>
      </div>

      <div class="form-group">
        <label for="inputAngle">Угол поворота (градусы):</label>
        <div class="input-row">
          <input type="number" id="inputAngle" value="90" step="1">
          <span style="font-size:1.1rem; color:var(--text-muted)">°</span>
        </div>
        <div class="presets">
          <button class="btn-secondary" onclick="setAnglePreset(-90)">-90°</button>
          <button class="btn-secondary" onclick="setAnglePreset(-45)">-45°</button>
          <button class="btn-secondary" onclick="setAnglePreset(0)">0°</button>
          <button class="btn-secondary" onclick="setAnglePreset(45)">+45°</button>
          <button class="btn-secondary" onclick="setAnglePreset(90)">+90°</button>
          <button class="btn-secondary" onclick="setAnglePreset(180)">+180°</button>
          <button class="btn-secondary" onclick="setAnglePreset(360)">+360°</button>
        </div>
      </div>

      <div class="form-group" style="margin-top: 16px;">
        <div style="display:flex; justify-content:space-between; margin-bottom: 6px;">
          <label for="rangeSpeed">Скорость вращения:</label>
          <span id="txtSpeedLabel" style="font-size: 0.9rem; font-weight:600; color:var(--primary)">45 °/с</span>
        </div>
        <input type="range" id="rangeSpeed" min="5" max="180" value="45" oninput="onSpeedChange(this.value)">
      </div>

      <div class="btn-row" style="margin-top: 20px;">
        <button id="btnMove" style="flex: 1; padding: 12px;" onclick="sendMove()">
          Повернуть стол
        </button>
      </div>
    </div>

    <!-- API DOCUMENTATION ACCORDION -->
    <details>
      <summary>Справка по REST JSON API</summary>
      <div style="margin-top:10px;">
        <p><strong>Поиск концевика (Homing):</strong></p>
        <pre>POST /api/home</pre>
        
        <p style="margin-top:8px;"><strong>Поворот стола:</strong></p>
        <pre>POST /api/move
Content-Type: application/json

{
  "angle": 90.0,
  "speed": 45.0,
  "relative": false
}</pre>

        <p style="margin-top:8px;"><strong>Остановка:</strong></p>
        <pre>POST /api/stop</pre>

        <p style="margin-top:8px;"><strong>Опрос состояния:</strong></p>
        <pre>GET /api/status</pre>
      </div>
    </details>
  </div>

  <div id="toast"></div>

  <script>
    let isRelative = false;

    function toast(msg) {
      const t = document.getElementById('toast');
      t.textContent = msg;
      t.style.display = 'block';
      setTimeout(() => { t.style.display = 'none'; }, 3000);
    }

    function setRelativeMode(rel) {
      isRelative = rel;
      document.getElementById('btnModeAbs').classList.toggle('active', !rel);
      document.getElementById('btnModeRel').classList.toggle('active', rel);
    }

    function setAnglePreset(val) {
      document.getElementById('inputAngle').value = val;
    }

    function onSpeedChange(val) {
      document.getElementById('txtSpeedLabel').textContent = val + ' °/с';
    }

    async function apiCall(endpoint, data = null) {
      try {
        const opts = {
          method: data ? 'POST' : 'GET',
          headers: { 'Content-Type': 'application/json' }
        };
        if (data) opts.body = JSON.stringify(data);
        const res = await fetch(endpoint, opts);
        return await res.json();
      } catch (err) {
        console.error('API Error:', err);
        return null;
      }
    }

    async function startHoming() {
      toast('Запуск поиска концевика...');
      const res = await apiCall('/api/home', {});
      if (res && res.error) toast('Ошибка: ' + res.error);
    }

    async function stopMotor() {
      toast('Остановка двигателя...');
      await apiCall('/api/stop', {});
    }

    async function setZero() {
      const res = await apiCall('/api/zero', {});
      if (res && res.status === 'ok') toast('Установлен 0°');
    }

    async function sendMove() {
      const angle = parseFloat(document.getElementById('inputAngle').value) || 0;
      const speed = parseFloat(document.getElementById('rangeSpeed').value) || 30;
      toast(`Поворот на ${angle}° со скоростью ${speed}°/с...`);
      const res = await apiCall('/api/move', {
        angle: angle,
        speed: speed,
        relative: isRelative
      });
      if (res && res.error) toast('Ошибка: ' + res.error);
    }

    async function updateStatus() {
      const st = await apiCall('/api/status');
      if (!st) return;

      const badge = document.getElementById('badgeState');
      badge.textContent = st.state;
      badge.className = 'badge badge-' + st.state.toLowerCase();

      document.getElementById('valAngle').textContent = st.current_angle.toFixed(1);
      document.getElementById('valTarget').textContent = st.target_angle.toFixed(1);
      document.getElementById('valSpeed').textContent = Math.round(st.speed);

      // Homed indicator
      const dotHomed = document.getElementById('dotHomed');
      const txtHomed = document.getElementById('txtHomed');
      if (st.is_homed) {
        dotHomed.className = 'indicator-dot dot-on';
        txtHomed.textContent = 'Откалиброван (0° OK)';
      } else {
        dotHomed.className = 'indicator-dot dot-off';
        txtHomed.textContent = 'Не откалиброван';
      }

      // Endstop indicator
      const dotEndstop = document.getElementById('dotEndstop');
      const txtEndstop = document.getElementById('txtEndstop');
      if (st.endstop_triggered) {
        dotEndstop.className = 'indicator-dot dot-on';
        txtEndstop.textContent = 'НАЖАТ';
      } else {
        dotEndstop.className = 'indicator-dot dot-off';
        txtEndstop.textContent = 'Разомкнут';
      }

      document.getElementById('txtError').textContent = st.error || '';

      const isBusy = (st.state === 'HOMING');
      document.getElementById('btnHome').disabled = isBusy;
      document.getElementById('btnMove').disabled = isBusy;
    }

    setInterval(updateStatus, 400);
    updateStatus();
  </script>
</body>
</html>
)rawliteral";

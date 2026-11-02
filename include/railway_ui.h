#pragma once
#include <Arduino.h>

const char PAGE_RAILWAY[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="uk">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>RailWay ESP32</title>
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
    .badge-holding { background: #a371f733; color: #d2a8ff; border: 1px solid #8957e5; animation: pulse 1.5s infinite; }
    .badge-connected { background: #23863633; color: #3fb950; border: 1px solid #238636; }
    .badge-disconnected { background: #da363333; color: #f85149; border: 1px solid #da3633; }
    .badge-warning { background: #d2992233; color: #e3b341; border: 1px solid #9e6a03; }
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
    details.card summary {
      cursor: pointer;
      list-style: none;
      user-select: none;
      margin-bottom: 0;
    }
    details.card summary::-webkit-details-marker {
      display: none;
    }
    details.card[open] summary {
      margin-bottom: 16px;
      padding-bottom: 10px;
      border-bottom: 1px solid rgba(48, 54, 61, 0.5);
    }
    .summary-arrow {
      font-size: 0.9rem;
      color: var(--text-muted);
      transition: transform 0.2s ease;
      margin-left: 8px;
    }
    details.card[open] .summary-arrow {
      transform: rotate(180deg);
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
    .grid-2 {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 12px;
    }
    .input-row {
      display: flex;
      gap: 10px;
      align-items: center;
    }
    input[type="number"], input[type="text"], input[type="password"], select {
      background: #0d1117;
      border: 1px solid var(--border);
      border-radius: 6px;
      color: var(--text);
      padding: 10px 14px;
      font-size: 1rem;
      width: 100%;
    }
    input:focus, select:focus {
      outline: none;
      border-color: var(--primary);
      box-shadow: 0 0 0 2px rgba(88, 166, 255, 0.2);
    }
    input[type="range"] {
      background: #0d1117;
      border: 1px solid var(--border);
      border-radius: 6px;
      color: var(--text);
      width: 100%;
      padding: 0;
      height: 6px;
      cursor: pointer;
    }
    .input-with-btn {
      position: relative;
      display: flex;
      align-items: center;
    }
    .input-with-btn input {
      padding-right: 44px;
    }
    .input-with-btn .btn-inside {
      position: absolute;
      right: 6px;
      background: transparent;
      border: none;
      color: var(--text-muted);
      padding: 6px;
      font-size: 1rem;
      cursor: pointer;
    }
    .input-with-btn .btn-inside:hover {
      color: var(--text);
      transform: none;
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
    .info-box {
      background: rgba(13, 17, 23, 0.6);
      padding: 12px 14px;
      border-radius: 8px;
      border: 1px solid rgba(48, 54, 61, 0.5);
      margin-bottom: 14px;
      font-size: 0.85rem;
      display: flex;
      flex-direction: column;
      gap: 4px;
    }
    .section-title {
      font-size: 0.9rem;
      color: var(--text);
      margin-bottom: 8px;
      font-weight: 600;
      border-bottom: 1px solid rgba(48, 54, 61, 0.3);
      padding-bottom: 4px;
    }
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
    .wall-corner { position: absolute; font-size: 0.7rem; color: var(--text-muted); background: rgba(13,17,23,0.75); padding: 1px 4px; border-radius: 4px; pointer-events: none; line-height: 1.3; }
    #toast {
      position: fixed;
      bottom: 20px;
      right: 20px;
      padding: 12px 20px;
      border-radius: 8px;
      background: #21262d;
      border: 1px solid var(--border);
      color: var(--text);
      font-size: 0.9rem;
      display: none;
      box-shadow: 0 8px 24px rgba(0,0,0,0.6);
      z-index: 1000;
      max-width: 90vw;
    }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>RailWay — Каретка X/Z</h1>
      <div style="display: flex; gap: 6px; flex-wrap: wrap;">
        <span id="badgeState" class="badge badge-idle">IDLE</span>
        <span id="badgeHomed" class="badge badge-warning">БЕЗ HOMING</span>
      </div>
    </header>

    <div class="card">
      <div class="card-title">
        <span>Каретка (Live)</span>
        <span id="txtError" style="color: var(--danger); font-size: 0.8rem; font-weight: normal;"></span>
      </div>
      <div class="status-grid">
        <div class="status-item">
          <div class="label">Позиція X (від кінцевика)</div>
          <div class="value"><span id="valX">0.0</span> мм</div>
          <div class="sub">ціль: <span id="valTargetX">0.0</span> мм</div>
        </div>
        <div class="status-item">
          <div class="label">Позиція Z (від кінцевика)</div>
          <div class="value"><span id="valZ">0.0</span> мм</div>
          <div class="sub">ціль: <span id="valTargetZ">0.0</span> мм</div>
        </div>
        <div class="status-item">
          <div class="label">Швидкість</div>
          <div class="value"><span id="valSpeed">0.0</span> мм/с</div>
        </div>
        <div class="status-item">
          <div class="label">Кінцевики</div>
          <div class="sub" style="font-size: 0.85rem; line-height: 1.6;">
            <div><span id="dotLimitX" class="indicator-dot dot-off"></span>X (початок)</div>
            <div><span id="dotLimitXFar" class="indicator-dot dot-off"></span>X (кінець)</div>
            <div><span id="dotLimitZ" class="indicator-dot dot-off"></span>Z</div>
          </div>
        </div>
      </div>
      <div class="btn-row">
        <button class="btn-success" onclick="startHoming()">Homing (X + вимірювання, потім Z)</button>
        <button class="btn-danger" onclick="stopMotion()">СТОП</button>
      </div>
    </div>

    <details class="card" id="detailsWall" open>
      <summary class="card-title">
        <span>Стіна — клік для переміщення</span>
        <span class="summary-arrow">▼</span>
      </summary>
      <div class="info-box">
        <div>Курсор: <strong id="lblHover">—</strong></div>
        <div>Остання ціль: <strong id="lblLastClick">—</strong></div>
        <div id="lblWallSize" style="color: var(--text-muted);"></div>
      </div>
      <div id="wallWrap" style="width: 100%;">
        <div id="wall" style="position: relative; width: 100%; aspect-ratio: 2 / 1; background: #0d1117; border: 1px solid var(--border); border-radius: 8px; overflow: hidden; cursor: crosshair; touch-action: none;">
          <div id="wallGrid" style="position: absolute; inset: 0; pointer-events: none;"></div>
          <div id="wallEndstopZ" style="position: absolute; left: 0; right: 0; height: 3px; background: var(--warning); pointer-events: none;"></div>
          <div id="wallEndstopX" style="position: absolute; top: 0; bottom: 0; left: 0; width: 3px; background: var(--warning); pointer-events: none;"></div>
          <div id="wallTarget" style="position: absolute; width: 16px; height: 16px; margin: -8px 0 0 -8px; border: 2px solid var(--accent); border-radius: 50%; display: none; pointer-events: none;"></div>
          <div id="wallCrossH" style="position: absolute; left: 0; right: 0; height: 0; border-top: 1px dashed rgba(88,166,255,0.8); display: none; pointer-events: none;"></div>
          <div id="wallCrossV" style="position: absolute; top: 0; bottom: 0; width: 0; border-left: 1px dashed rgba(88,166,255,0.8); display: none; pointer-events: none;"></div>
          <span id="cornerTL" class="wall-corner" style="left: 6px; top: 4px;"></span>
          <span id="cornerTR" class="wall-corner" style="right: 6px; top: 4px; text-align: right;"></span>
          <span id="cornerBL" class="wall-corner" style="left: 6px; bottom: 4px;"></span>
          <span id="cornerBR" class="wall-corner" style="right: 6px; bottom: 4px; text-align: right;"></span>
          <div id="wallCarriage" style="position: absolute; width: 14px; height: 14px; margin: -7px 0 0 -7px; background: var(--primary); border-radius: 3px; box-shadow: 0 0 10px var(--primary); pointer-events: none; left: 0; top: 0;"></div>
        </div>
        <div style="display: flex; justify-content: space-between; font-size: 0.75rem; color: var(--text-muted); margin-top: 4px;">
          <span>X = 0 (кінцевик)</span>
          <span id="lblWallXMax">X макс.</span>
        </div>
      </div>
      <div class="form-group" style="margin-top: 12px;">
        <label for="rangeSpeed">Швидкість переміщення: <strong id="txtSpeedLabel">50 мм/с</strong></label>
        <input type="range" id="rangeSpeed" min="1" max="100" step="1" value="50" oninput="onSpeedChange(this.value)">
      </div>
    </details>

    <details class="card" id="detailsManual">
      <summary class="card-title">
        <span>Ручне керування</span>
        <span class="summary-arrow">▼</span>
      </summary>
      <div class="grid-2">
        <div class="form-group">
          <label for="inputX">X (мм):</label>
          <input type="number" id="inputX" value="0" step="1" min="0">
        </div>
        <div class="form-group">
          <label for="inputZ">Z (мм):</label>
          <input type="number" id="inputZ" value="0" step="1" min="0">
        </div>
      </div>
      <div class="btn-row">
        <button onclick="moveToInputs()">Їхати в точку</button>
        <button class="btn-secondary" onclick="zeroHere()">Поточна позиція = (0, 0)</button>
      </div>
      <div class="section-title" style="margin-top: 16px;">Ручні кроки (працюють і без homing)</div>
      <div class="section-title" style="font-weight: normal; color: var(--text-muted);">Вісь X</div>
      <div class="presets">
        <button class="btn-secondary" onclick="jog('x', -50)">−50</button>
        <button class="btn-secondary" onclick="jog('x', -10)">−10</button>
        <button class="btn-secondary" onclick="jog('x', -1)">−1</button>
        <button class="btn-secondary" onclick="jog('x', 1)">+1</button>
        <button class="btn-secondary" onclick="jog('x', 10)">+10</button>
        <button class="btn-secondary" onclick="jog('x', 50)">+50</button>
      </div>
      <div class="section-title" style="font-weight: normal; color: var(--text-muted); margin-top: 12px;">Вісь Z</div>
      <div class="presets">
        <button class="btn-secondary" onclick="jog('z', -50)">−50</button>
        <button class="btn-secondary" onclick="jog('z', -10)">−10</button>
        <button class="btn-secondary" onclick="jog('z', -1)">−1</button>
        <button class="btn-secondary" onclick="jog('z', 1)">+1</button>
        <button class="btn-secondary" onclick="jog('z', 10)">+10</button>
        <button class="btn-secondary" onclick="jog('z', 50)">+50</button>
      </div>
      <div class="btn-row">
        <button class="btn-secondary" id="btnDrivers" onclick="toggleDrivers()">Вимкнути драйвери</button>
      </div>
    </details>

    <details class="card" id="detailsSettings">
      <summary class="card-title">
        <span>Налаштування рейки та осей</span>
        <span class="summary-arrow">▼</span>
      </summary>
      <div class="section-title">Розміри стіни (від кінцевиків)</div>
      <div class="grid-2">
        <div class="form-group">
          <label for="setXLength">Довжина X, мм (вимірюється при homing): <span id="lblXMeasured" style="color: var(--warning);"></span></label>
          <input type="number" id="setXLength" step="1" min="10">
        </div>
        <div class="form-group">
          <label for="setZHeight">Висота Z вниз від верхнього кінцевика, мм (обмежує хід):</label>
          <input type="number" id="setZHeight" step="1" min="10">
        </div>
      </div>
      <div class="section-title">Двигуни</div>
      <div class="grid-2">
        <div class="form-group">
          <label for="setStepsX">Кроків/мм, X:</label>
          <input type="number" id="setStepsX" step="0.01" min="1">
        </div>
        <div class="form-group">
          <label for="setStepsZ">Кроків/мм, Z:</label>
          <input type="number" id="setStepsZ" step="0.01" min="1">
        </div>
      </div>
      <div class="grid-2">
        <div class="form-group">
          <label><input type="checkbox" id="setXInvert"> Інвертувати DIR, X</label>
        </div>
        <div class="form-group">
          <label><input type="checkbox" id="setZInvert"> Інвертувати DIR, Z</label>
        </div>
      </div>
      <div class="section-title">Рух</div>
      <div class="grid-2">
        <div class="form-group">
          <label for="setMaxSpeed">Макс. швидкість, мм/с:</label>
          <input type="number" id="setMaxSpeed" step="1" min="1" max="100">
        </div>
        <div class="form-group">
          <label for="setAccel">Прискорення, мм/с²:</label>
          <input type="number" id="setAccel" step="10" min="10">
        </div>
        <div class="form-group">
          <label for="setHomeSpeed">Швидкість homing, мм/с:</label>
          <input type="number" id="setHomeSpeed" step="1" min="1" max="100">
        </div>
        <div class="form-group">
          <label for="setBackoff">Відкат від кінцевика, мм:</label>
          <input type="number" id="setBackoff" step="0.5" min="1" max="50">
        </div>
      </div>
      <div class="section-title">Кінцевики</div>
      <div class="form-group">
        <label><input type="checkbox" id="setLimitLow"> Кінцевик спрацьовує низьким рівнем</label>
      </div>
      <div class="form-group">
        <label><input type="checkbox" id="setZTop"> Кінцевик Z розташований зверху (лише для карти)</label>
      </div>
      <div class="form-group">
        <label><input type="checkbox" id="setAutoHome"> Homing при увімкненні</label>
       </div>
       <div class="form-group">
         <label><input type="checkbox" id="setIgnoreHoming"> Ігнорувати homing (налагодження)</label>
      </div>
      <p style="font-size: 0.75rem; color: var(--text-muted); margin-bottom: 10px;">
        Зміна кроків/мм або інверсії DIR скидає homing. Налаштування не можна змінювати під час руху.
      </p>
      <div class="btn-row">
        <button class="btn-success" style="flex: 1;" onclick="saveSettings()">💾 Зберегти налаштування</button>
      </div>
    </details>


    <!-- СЕКЦІЯ НАЛАШТУВАНЬ WI-FI ТА ТОЧКИ ДОСТУПУ (ЗГОРТАНА) -->
    <details class="card" id="detailsWifi">
      <summary class="card-title">
        <span style="display:flex; align-items:center; gap:8px;">
          <span>📶</span>
          <span>Налаштування Wi-Fi та Точки Доступу</span>
        </span>
        <span style="display:flex; align-items:center; gap:8px;">
          <span id="badgeWifiMode" class="badge badge-idle">STA</span>
          <span class="summary-arrow">▼</span>
        </span>
      </summary>

      <div style="margin-top: 10px;">
        <div class="info-box">
          <div><strong>Поточний статус:</strong> <span id="lblWifiStatus" style="color: var(--primary);">Завантаження...</span></div>
          <div><strong>IP-адреса:</strong> <code id="lblWifiIP" style="color: #79c0ff; font-family: monospace;">-</code></div>
        </div>

        <div style="margin-bottom: 18px;">
          <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px;">
            <h3 style="font-size: 0.9rem; color: var(--text);">Підключення до роутера (Wi-Fi Клієнт)</h3>
            <button type="button" id="btnScanWifi" class="btn-secondary" style="padding: 4px 10px; font-size: 0.75rem;" onclick="scanWiFiNetworks()">
              🔍 Сканувати мережі
            </button>
          </div>

          <div id="scanContainer" style="display: none; margin-bottom: 12px;">
            <label style="font-size: 0.8rem; color: var(--text-muted); display: block; margin-bottom: 4px;">Оберіть знайдену мережу:</label>
            <select id="selectWifiScan" onchange="onSelectNetwork(this.value)">
              <option value="">-- Список мереж --</option>
            </select>
          </div>

          <div class="form-group">
            <label for="inputStaSSID">Назва мережі Wi-Fi (SSID):</label>
            <input type="text" id="inputStaSSID" placeholder="Назва вашої Wi-Fi мережі">
          </div>

          <div class="form-group">
            <label for="inputStaPass">Пароль від Wi-Fi мережі:</label>
            <div class="input-with-btn">
              <input type="password" id="inputStaPass" placeholder="Пароль до роутера (якщо є)">
              <button type="button" class="btn-inside" onclick="togglePassVisibility('inputStaPass')" title="Показати/приховати">👁</button>
            </div>
          </div>
        </div>

        <div style="border-top: 1px solid var(--border); padding-top: 16px; margin-bottom: 18px;">
          <h3 style="font-size: 0.9rem; color: var(--text); margin-bottom: 4px;">Власна точка доступу столу (SoftAP)</h3>
          <p style="font-size: 0.75rem; color: var(--text-muted); margin-bottom: 12px;">
            ESP32 активує цю мережу за відсутності зв'язку з основним роутером.
          </p>

          <div class="form-group">
            <label for="inputApSSID">Ім'я точки столу (AP SSID):</label>
            <input type="text" id="inputApSSID" placeholder="RotatingTable-ESP32">
          </div>

          <div class="form-group">
            <label for="inputApPass">Пароль точки столу (AP Password):</label>
            <div class="input-with-btn">
              <input type="password" id="inputApPass" placeholder="Мінімум 8 символів (або порожньо для відкритої)">
              <button type="button" class="btn-inside" onclick="togglePassVisibility('inputApPass')" title="Показати/приховати">👁</button>
            </div>
          </div>
        </div>

        <div class="btn-row">
          <button id="btnSaveWifi" class="btn-success" style="flex: 1;" onclick="saveWiFiSettings()">
            💾 Зберегти та перезавантажити
          </button>
          <button class="btn-secondary" onclick="resetWiFiSettings()">
            Скинути до заводських
          </button>
        </div>
      </div>
    </details>


    <details class="card" id="detailsOta">
      <summary class="card-title">
        <span style="display:flex; align-items:center; gap:8px;">
          <span>⬆️</span>
          <span>Оновлення прошивки з GitHub</span>
        </span>
        <span class="summary-arrow">▼</span>
      </summary>
      <div style="margin-top: 10px;">
        <p style="font-size: 0.8rem; color: var(--text-muted);">
          Буде використано railway.bin з останнього релізу репозиторію
          set-st/rotating_table. Потрібне підключення до роутера з Інтернетом.
          Не вимикайте живлення під час оновлення.
        </p>
        <div>Версія: <strong id="txtCurrentVersion">невідома</strong></div>
        <div id="txtOtaRelease" class="info-box">
          Натисніть «Перевірити реліз», щоб дізнатися про оновлення.
        </div>
        <div class="btn-row">
          <button type="button" class="btn-secondary" onclick="checkOtaRelease()">Перевірити реліз</button>
          <button type="button" id="btnOtaUpdate" class="btn-success" onclick="updateFromGithub()">Оновити прошивку</button>
        </div>
      </div>
    </details>

    <details class="card" id="detailsApi">
      <summary class="card-title">
        <span style="display:flex; align-items:center; gap:8px;">
          <span>📖</span>
          <span>Довідка по REST JSON API</span>
        </span>
        <span class="summary-arrow">▼</span>
      </summary>
      <div style="margin-top: 10px;">
        <p><strong>Стан у реальному часі:</strong></p>
        <pre>GET /api/status</pre>
        <p style="margin-top:8px;"><strong>Homing: X до кінцевика 1, до кінцевика 2 (вимірювання довжини), потім Z вгору до кінцевика:</strong></p>
        <pre>POST /api/home</pre>
        <p style="margin-top:8px;"><strong>Переміщення в точку стіни (мм від кінцевиків):</strong></p>
        <pre>POST /api/move
{"x": 400.0, "z": 150.0, "speed": 40.0}</pre>
        <p style="margin-top:8px;"><strong>Відносний рух однієї осі:</strong></p>
        <pre>POST /api/jog
{"axis": "x", "distance": -10.0, "speed": 20.0}</pre>
        <p style="margin-top:8px;"><strong>Зупинка, нуль, драйвери:</strong></p>
        <pre>POST /api/stop
POST /api/zero
POST /api/enable   {"enabled": true}</pre>
        <p style="margin-top:8px;"><strong>Налаштування:</strong></p>
        <pre>GET  /api/settings
POST /api/settings
     {"x_length_mm": 1200, "z_height_mm": 600, "x_dir_invert": true,
      "steps_per_mm_x": 80, "steps_per_mm_z": 80, "limit_active_low": true}</pre>
        <p style="margin-top:8px;"><strong>Оновлення прошивки:</strong></p>
        <pre>GET  /api/ota/latest
POST /api/ota/update</pre>
        <p style="margin-top:8px;"><strong>Керування мережею Wi-Fi:</strong></p>
        <pre>GET  /api/wifi/config
GET  /api/wifi/scan
POST /api/wifi/save
POST /api/wifi/reset</pre>
      </div>
    </details>
  </div>

  <div id="toast"></div>

  <script>
    let wallCfg = { x: 1000, z: 500, zTop: true };
    let lastStatus = null;
    let settingsLoaded = false;

    function toast(msg, duration = 3000) {
      const t = document.getElementById('toast');
      t.innerHTML = msg;
      t.style.display = 'block';
      if (duration > 0) {
        setTimeout(() => { t.style.display = 'none'; }, duration);
      }
    }

    function togglePassVisibility(id) {
      const input = document.getElementById(id);
      input.type = (input.type === 'password') ? 'text' : 'password';
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
        return null;
      }
    }

    function onSpeedChange(val) {
      document.getElementById('txtSpeedLabel').textContent = val + ' мм/с';
    }

    function fmt(v) { return Number(v).toFixed(1); }

    // Перетворення між мм і відсотками на карті (Z відлічується від кінцевика)
    function mmToPct(x, z) {
      const px = x / wallCfg.x * 100;
      const pz = z / wallCfg.z * 100;
      return { left: px, top: wallCfg.zTop ? pz : 100 - pz };
    }

    function eventToMm(ev) {
      const r = document.getElementById('wall').getBoundingClientRect();
      const fx = Math.min(1, Math.max(0, (ev.clientX - r.left) / r.width));
      const fy = Math.min(1, Math.max(0, (ev.clientY - r.top) / r.height));
      return { x: fx * wallCfg.x, z: (wallCfg.zTop ? fy : 1 - fy) * wallCfg.z };
    }

    function placeMarker(el, x, z) {
      const p = mmToPct(x, z);
      el.style.left = p.left + '%';
      el.style.top = p.top + '%';
    }

    function drawGrid() {
      const wall = document.getElementById('wall');
      wall.style.aspectRatio = wallCfg.x + ' / ' + wallCfg.z;
      const grid = document.getElementById('wallGrid');
      const stepMm = [10, 20, 50, 100, 200, 500, 1000].find(s => wallCfg.x / s <= 12) || 1000;
      let html = '';
      for (let x = stepMm; x < wallCfg.x; x += stepMm) {
        html += `<div style="position:absolute;top:0;bottom:0;left:${x / wallCfg.x * 100}%;border-left:1px solid #21262d;"></div>`;
      }
      for (let z = stepMm; z < wallCfg.z; z += stepMm) {
        const top = wallCfg.zTop ? z / wallCfg.z * 100 : 100 - z / wallCfg.z * 100;
        html += `<div style="position:absolute;left:0;right:0;top:${top}%;border-top:1px solid #21262d;"></div>`;
      }
      grid.innerHTML = html;
      const ez = document.getElementById('wallEndstopZ');
      ez.style.top = wallCfg.zTop ? '0' : 'auto';
      ez.style.bottom = wallCfg.zTop ? 'auto' : '0';
      document.getElementById('lblWallXMax').textContent = 'X = ' + wallCfg.x + ' мм';
      document.getElementById('lblWallSize').textContent =
        `Стіна: ${wallCfg.x} × ${wallCfg.z} мм. Z відлічується від кінцевика (${wallCfg.zTop ? 'зверху' : 'знизу'}).`;
      const zNear = wallCfg.zTop ? 0 : wallCfg.z, zFar = wallCfg.zTop ? wallCfg.z : 0;
      const corner = (id, x, z) => { document.getElementById(id).textContent = 'X' + x + ' Z' + z; };
      corner('cornerTL', 0, zNear); corner('cornerTR', wallCfg.x, zNear);
      corner('cornerBL', 0, zFar); corner('cornerBR', wallCfg.x, zFar);
      document.getElementById('inputX').max = wallCfg.x;
      document.getElementById('inputZ').max = wallCfg.z;
    }

    function setDot(id, on) {
      document.getElementById(id).className = 'indicator-dot ' + (on ? 'dot-on' : 'dot-off');
    }

    async function updateStatus() {
      const st = await apiCall('/api/status');
      const badge = document.getElementById('badgeState');
      if (!st || st.status !== 'ok') {
        badge.className = 'badge badge-disconnected';
        badge.textContent = 'НЕМАЄ ЗВ\'ЯЗКУ';
        return;
      }
      lastStatus = st;

      if (st.x_length_mm !== wallCfg.x || st.z_height_mm !== wallCfg.z || st.z_endstop_top !== wallCfg.zTop) {
        wallCfg = { x: st.x_length_mm, z: st.z_height_mm, zTop: st.z_endstop_top };
        drawGrid();
      }

      badge.textContent = st.state;
      const cls = { IDLE: 'idle', MOVING: 'moving', HOMING: 'homing' }[st.state] || 'error';
      badge.className = 'badge badge-' + cls;

      const homed = document.getElementById('badgeHomed');
      homed.className = 'badge ' + (st.is_homed ? 'badge-connected' : 'badge-warning');
      homed.textContent = st.is_homed ? 'HOMED' : (st.ignore_homing ? 'HOMING ІГНОРУЄТЬСЯ' : 'БЕЗ HOMING');

      document.getElementById('valX').textContent = fmt(st.x_mm);
      document.getElementById('valZ').textContent = fmt(st.z_mm);
      document.getElementById('valTargetX').textContent = fmt(st.target_x_mm);
      document.getElementById('valTargetZ').textContent = fmt(st.target_z_mm);
      document.getElementById('valSpeed').textContent = fmt(st.speed_mm_s);
      document.getElementById('txtError').textContent = st.error || '';
      setDot('dotLimitX', st.limit_x);
      setDot('dotLimitXFar', st.limit_x_far);
      setDot('dotLimitZ', st.limit_z);

      placeMarker(document.getElementById('wallCarriage'), st.x_mm, st.z_mm);
      document.getElementById('btnDrivers').textContent =
        st.drivers_enabled ? 'Вимкнути драйвери' : 'Увімкнути драйвери';

      const speed = document.getElementById('rangeSpeed');
      if (Number(speed.max) !== 100) speed.max = 100;
    }

    async function sendMove(x, z) {
      const speed = Number(document.getElementById('rangeSpeed').value);
      const res = await apiCall('/api/move', { x: x, z: z, speed: speed });
      if (!res || res.status !== 'ok') {
        toast('Помилка: ' + (res ? res.error : 'немає відповіді'));
        return false;
      }
      const t = document.getElementById('wallTarget');
      placeMarker(t, x, z);
      t.style.display = 'block';
      document.getElementById('lblLastClick').textContent = `X ${fmt(x)} мм, Z ${fmt(z)} мм`;
      return true;
    }

    function initWall() {
      const wall = document.getElementById('wall');
      const crossH = document.getElementById('wallCrossH');
      const crossV = document.getElementById('wallCrossV');
      wall.addEventListener('pointermove', ev => {
        const p = eventToMm(ev);
        document.getElementById('lblHover').textContent = `X ${fmt(p.x)} мм, Z ${fmt(p.z)} мм`;
        const r = wall.getBoundingClientRect();
        crossH.style.top = (ev.clientY - r.top) + 'px';
        crossV.style.left = (ev.clientX - r.left) + 'px';
        crossH.style.display = crossV.style.display = 'block';
      });
      wall.addEventListener('pointerleave', () => {
        document.getElementById('lblHover').textContent = '—';
        crossH.style.display = crossV.style.display = 'none';
      });
      wall.addEventListener('click', ev => {
        const p = eventToMm(ev);
        sendMove(Number(p.x.toFixed(1)), Number(p.z.toFixed(1)));
      });
      drawGrid();
    }

    async function startHoming() {
      toast('Запуск пошуку кінцевиків...');
      const res = await apiCall('/api/home', {});
      if (res && res.status !== 'ok') toast('Помилка: ' + res.error);
    }

    async function stopMotion() {
      await apiCall('/api/stop', {});
      toast('Зупинено');
    }

    async function moveToInputs() {
      const x = parseFloat(document.getElementById('inputX').value);
      const z = parseFloat(document.getElementById('inputZ').value);
      if (isNaN(x) || isNaN(z)) { toast('Введіть координати X та Z'); return; }
      await sendMove(x, z);
    }

    async function jog(axis, distance) {
      const speed = Number(document.getElementById('rangeSpeed').value);
      const res = await apiCall('/api/jog', { axis: axis, distance: distance, speed: speed });
      if (!res || res.status !== 'ok') toast('Помилка: ' + (res ? res.error : 'немає відповіді'));
    }

    async function zeroHere() {
      if (!confirm('Прийняти поточну позицію за X=0, Z=0? Перевірте, що каретка біля кінцевиків.')) return;
      const res = await apiCall('/api/zero', {});
      toast(res && res.status === 'ok' ? res.message : 'Помилка: ' + (res ? res.error : 'немає відповіді'));
    }

    async function toggleDrivers() {
      const enable = lastStatus ? !lastStatus.drivers_enabled : true;
      if (!enable && !confirm('Вимкнути драйвери? Вертикальна вісь може опуститися, homing буде скинуто.')) return;
      await apiCall('/api/enable', { enabled: enable });
    }

    async function checkOtaRelease() {
      const output = document.getElementById('txtOtaRelease');
      output.textContent = 'Перевірка останнього релізу GitHub...';
      const res = await apiCall('/api/ota/latest');
      if (res && res.current_version) {
        document.getElementById('txtCurrentVersion').textContent = res.current_version;
      }
      if (!res || res.status !== 'ok') {
        output.textContent = 'Помилка перевірки: ' + (res ? (res.error || 'невідома помилка') : 'немає відповіді');
        return;
      }
      output.textContent = `Встановлена: ${res.current_version}; останній реліз: ${res.tag}. ` +
        `Файл: ${res.asset} (${Math.round(res.size / 1024)} КБ)`;
    }

    async function updateFromGithub() {
      if (!confirm('Запустити оновлення прошивки з останнього релізу GitHub? Рух буде зупинено.')) return;
      const btn = document.getElementById('btnOtaUpdate');
      btn.disabled = true;
      document.getElementById('txtOtaRelease').textContent =
        'Завантаження та запис прошивки. Не вимикайте живлення...';
      const res = await apiCall('/api/ota/update', {});
      if (res && res.status === 'ok') {
        startRebootCountdown('Прошивку оновлено. Контролер перезавантажується...');
      } else {
        btn.disabled = false;
        document.getElementById('txtOtaRelease').textContent =
          'Помилка оновлення: ' + (res ? (res.error || res.message) : 'немає відповіді');
      }
    }

    async function loadSettings() {
      const s = await apiCall('/api/settings');
      if (!s || s.status !== 'ok') return;
      document.getElementById('setXLength').value = s.x_length_mm;
      document.getElementById('lblXMeasured').textContent = s.x_length_measured ? '✔ виміряно' : '(ще не виміряно)';
      document.getElementById('setZHeight').value = s.z_height_mm;
      document.getElementById('setStepsX').value = s.steps_per_mm_x;
      document.getElementById('setStepsZ').value = s.steps_per_mm_z;
      document.getElementById('setXInvert').checked = s.x_dir_invert;
      document.getElementById('setZInvert').checked = s.z_dir_invert;
      document.getElementById('setMaxSpeed').value = s.max_speed_mm_s;
      document.getElementById('setAccel').value = s.accel_mm_s2;
      document.getElementById('setHomeSpeed').value = s.homing_speed_mm_s;
      document.getElementById('setBackoff').value = s.homing_backoff_mm;
      document.getElementById('setLimitLow').checked = s.limit_active_low;
      document.getElementById('setZTop').checked = s.z_endstop_top;
      document.getElementById('setAutoHome').checked = s.auto_home_on_boot;
      document.getElementById('setIgnoreHoming').checked = !!s.ignore_homing;
      document.getElementById('rangeSpeed').value = Math.min(100, s.max_speed_mm_s);
      onSpeedChange(document.getElementById('rangeSpeed').value);
    }

    async function saveSettings() {
      const num = id => parseFloat(document.getElementById(id).value);
      const chk = id => document.getElementById(id).checked;
      const res = await apiCall('/api/settings', {
        x_length_mm: num('setXLength'),
        z_height_mm: num('setZHeight'),
        steps_per_mm_x: num('setStepsX'),
        steps_per_mm_z: num('setStepsZ'),
        x_dir_invert: chk('setXInvert'),
        z_dir_invert: chk('setZInvert'),
        max_speed_mm_s: num('setMaxSpeed'),
        accel_mm_s2: num('setAccel'),
        homing_speed_mm_s: num('setHomeSpeed'),
        homing_backoff_mm: num('setBackoff'),
        limit_active_low: chk('setLimitLow'),
        z_endstop_top: chk('setZTop'),
        auto_home_on_boot: chk('setAutoHome'),
        ignore_homing: chk('setIgnoreHoming')
      });
      if (res && res.status === 'ok') {
        toast('Налаштування збережено');
        loadSettings();
      } else {
        toast('Помилка: ' + (res ? res.error : 'немає відповіді'));
      }
    }

    // --- НАЛАШТУВАННЯ WI-FI ---
    async function loadWiFiConfig() {
      const cfg = await apiCall('/api/wifi/config');
      if (!cfg || cfg.status !== 'ok') return;

      document.getElementById('badgeWifiMode').textContent = cfg.mode;
      document.getElementById('lblWifiIP').textContent = 'http://' + cfg.ip;

      if (cfg.connected) {
        document.getElementById('lblWifiStatus').innerHTML = `Підключено до роутера <strong>"${cfg.current_ssid}"</strong>`;
      } else {
        document.getElementById('lblWifiStatus').innerHTML = `Режим власної точки доступу <strong>"${cfg.current_ssid}"</strong>`;
      }

      if (cfg.sta_ssid) document.getElementById('inputStaSSID').value = cfg.sta_ssid;
      if (cfg.ap_ssid) document.getElementById('inputApSSID').value = cfg.ap_ssid;
    }

    async function scanWiFiNetworks() {
      const btn = document.getElementById('btnScanWifi');
      btn.disabled = true;
      btn.textContent = '⏳ Пошук...';
      toast('Сканування ефіру 2.4 ГГц...');

      const res = await apiCall('/api/wifi/scan');
      btn.disabled = false;
      btn.textContent = '🔍 Сканувати мережі';

      if (!res || !res.networks) {
        toast('Не вдалося отримати список мереж.');
        return;
      }

      const select = document.getElementById('selectWifiScan');
      select.innerHTML = '<option value="">-- Оберіть знайдену мережу --</option>';

      if (res.networks.length === 0) {
        toast('Мереж не виявлено.');
        return;
      }

      res.networks.sort((a, b) => b.rssi - a.rssi);
      res.networks.forEach(net => {
        if (!net.ssid) return;
        const opt = document.createElement('option');
        opt.value = net.ssid;
        opt.textContent = `${net.ssid} (${net.rssi} dBm)${net.secure ? ' 🔒' : ''}`;
        select.appendChild(opt);
      });

      document.getElementById('scanContainer').style.display = 'block';
      toast(`Знайдено ${res.networks.length} мереж`);
    }

    function onSelectNetwork(ssid) {
      if (!ssid) return;
      document.getElementById('inputStaSSID').value = ssid;
      document.getElementById('inputStaPass').focus();
    }

    function startRebootCountdown(msg) {
      let seconds = 12;
      const interval = setInterval(() => {
        toast(`<strong>${msg}</strong><br>Оновлення сторінки через ${seconds} сек...`, 0);
        seconds--;
        if (seconds < 0) {
          clearInterval(interval);
          location.reload();
        }
      }, 1000);
    }

    async function saveWiFiSettings() {
      const staSSID = document.getElementById('inputStaSSID').value.trim();
      const staPass = document.getElementById('inputStaPass').value;
      const apSSID = document.getElementById('inputApSSID').value.trim();
      const apPass = document.getElementById('inputApPass').value;

      if (apPass.length > 0 && apPass.length < 8) {
        alert('Пароль точки доступу (SoftAP) має містити щонайменше 8 символів або бути порожнім.');
        return;
      }

      if (!confirm('Застосувати та зберегти налаштування Wi-Fi? Контролер буде перезавантажено.')) {
        return;
      }

      const btn = document.getElementById('btnSaveWifi');
      btn.disabled = true;
      toast('Надсилання налаштувань...');

      const res = await apiCall('/api/wifi/save', {
        sta_ssid: staSSID,
        sta_pass: staPass,
        ap_ssid: apSSID,
        ap_pass: apPass
      });

      if (res && res.status === 'ok') {
        startRebootCountdown('Налаштування збережено!');
      } else {
        btn.disabled = false;
        toast('Помилка: ' + (res ? res.error : 'немає зв\'язку'));
      }
    }

    async function resetWiFiSettings() {
      if (!confirm('Скинути налаштування Wi-Fi до заводських?')) return;
      toast('Скидання налаштувань...');
      const res = await apiCall('/api/wifi/reset', {});
      if (res && res.status === 'ok') {
        startRebootCountdown('Налаштування мережі скинуто.');
      } else {
        toast('Помилка скидання.');
      }
    }


    initWall();
    loadSettings();
    loadWiFiConfig();
    updateStatus();
    apiCall('/api/status').then(s => { if (s && s.firmware_version) document.getElementById('txtCurrentVersion').textContent = s.firmware_version; });
    setInterval(updateStatus, 300);
  </script>
</body>
</html>
)rawliteral";

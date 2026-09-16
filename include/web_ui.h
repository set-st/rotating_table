#pragma once
#include <Arduino.h>

const char PAGE_INDEX[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="uk">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Поворотний стіл ESP32</title>
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
      <h1>Поворотний стіл ESP32</h1>
      <span id="badgeState" class="badge badge-idle">IDLE</span>
    </header>

    <!-- КАРТКА ПОТОЧНОГО СТАНУ (РЕАЛЬНИЙ ЧАС) -->
    <div class="card">
      <div class="card-title">
        <span>Поточний стан (Live)</span>
        <span id="txtError" style="color: var(--danger); font-size: 0.8rem; font-weight: normal;"></span>
      </div>
      <div class="status-grid">
        <div class="status-item">
          <div class="label">Поточний кут</div>
          <div class="value"><span id="valAngle">0.0</span>°</div>
          <div class="sub">Ціль: <span id="valTarget">0.0</span>°</div>
        </div>
        <div class="status-item">
          <div class="label">Швидкість</div>
          <div class="value"><span id="valSpeed">0</span> <small style="font-size:0.9rem">°/с</small></div>
          <div class="sub">
            <span id="dotHomed" class="indicator-dot dot-off"></span><span id="txtHomed">Не відкалібрований</span>
          </div>
        </div>
      </div>
      <div style="margin-top: 12px; font-size: 0.85rem; color: var(--text-muted); display: flex; justify-content: space-between;">
        <div>Кінцевик: <span id="dotEndstop" class="indicator-dot dot-off"></span><span id="txtEndstop" style="color: var(--text);">Розімкнений</span></div>
        <div id="lblLiveRatio" style="font-size: 0.8rem; color: var(--text-muted);">Редукція: 3.00:1</div>
      </div>
    </div>

    <!-- КАРТКА КАЛІБРУВАННЯ ТА ОБНУЛЕННЯ -->
    <div class="card">
      <div class="card-title">Калібрування та нульова точка</div>
      <p style="font-size: 0.85rem; color: var(--text-muted); margin-bottom: 12px;">
        Пошук кінцевика до упору для виставлення бази нуля (0.0°).
      </p>
      <div class="btn-row">
        <button id="btnHome" class="btn-success" onclick="startHoming()">
          <svg width="16" height="16" fill="currentColor" viewBox="0 0 16 16">
            <path d="M8.707 1.5a1 1 0 0 0-1.414 0L.646 8.146a.5.5 0 0 0 .708.708L2 8.207V13.5A1.5 1.5 0 0 0 3.5 15h9a1.5 1.5 0 0 0 1.5-1.5V8.207l.646.647a.5.5 0 0 0 .708-.708L13 5.793V2.5a.5.5 0 0 0-.5-.5h-1a.5.5 0 0 0-.5.5v1.293L8.707 1.5Z"/>
          </svg>
          Знайти кінцевик (Home)
        </button>
        <button class="btn-secondary" onclick="setZero()">Скинути кут в 0°</button>
        <button class="btn-danger" onclick="stopMotor()">СТОП</button>
      </div>
    </div>

    <!-- КАРТКА КЕРУВАННЯ ПОВОРОТОМ -->
    <div class="card">
      <div class="card-title">Керування поворотом</div>
      
      <div class="toggle-group">
        <div id="btnModeAbs" class="toggle-btn active" onclick="setRelativeMode(false)">Абсолютний кут</div>
        <div id="btnModeRel" class="toggle-btn" onclick="setRelativeMode(true)">Відносний кут (Δ)</div>
      </div>

      <div class="form-group">
        <label for="inputAngle">Кут повороту (градуси):</label>
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
          <label for="rangeSpeed">Швидкість обертання:</label>
          <span id="txtSpeedLabel" style="font-size: 0.9rem; font-weight:600; color:var(--primary)">45 °/с</span>
        </div>
        <input type="range" id="rangeSpeed" min="5" max="180" value="45" oninput="onSpeedChange(this.value)">
      </div>

      <div class="btn-row" style="margin-top: 20px;">
        <button id="btnMove" style="flex: 1; padding: 12px;" onclick="sendMove()">
          Повернути стіл
        </button>
      </div>
    </div>

    <!-- СЕКЦІЯ НАЛАШТУВАНЬ СТОЛУ ТА КІНЕМАТИКИ (ЗГОРТАНА) -->
    <details class="card" id="detailsSettings">
      <summary class="card-title">
        <span style="display:flex; align-items:center; gap:8px;">
          <span>⚙️</span>
          <span>Налаштування кінематики, шестерень та пінів</span>
        </span>
        <span class="summary-arrow">▼</span>
      </summary>

      <div style="margin-top: 10px;">
        <!-- 1. Механіка та шестерні -->
        <div class="section-title">Зубчаста передача та редукція</div>
        <div class="grid-2">
          <div class="form-group">
            <label for="inputMotorTeeth">Шестерня мотора (зубів):</label>
            <input type="number" id="inputMotorTeeth" value="20" min="1" max="200" oninput="recalcKinematics()">
          </div>
          <div class="form-group">
            <label for="inputTableTeeth">Шестерня столу (зубів):</label>
            <input type="number" id="inputTableTeeth" value="60" min="1" max="500" oninput="recalcKinematics()">
          </div>
        </div>

        <div class="info-box">
          <div><strong>Передатне число:</strong> <span id="lblGearRatio" style="color:var(--primary); font-weight:700;">3.00 (1:3)</span></div>
          <div><strong>Розраховано імпульсів:</strong> <span id="lblStepsPerDeg" style="color:#79c0ff; font-weight:700;">26.67 кроків/град</span></div>
        </div>

        <div class="grid-2">
          <div class="form-group">
            <label for="selectMotorSteps">Кроків мотора на 360°:</label>
            <select id="selectMotorSteps" onchange="recalcKinematics()">
              <option value="200">200 кроків (1.8° - стандарт)</option>
              <option value="400">400 кроків (0.9° - точний)</option>
            </select>
          </div>
          <div class="form-group">
            <label for="selectMicrosteps">Мікрокрок драйвера:</label>
            <select id="selectMicrosteps" onchange="recalcKinematics()">
              <option value="1">1 (Повний крок)</option>
              <option value="2">2 (1/2)</option>
              <option value="4">4 (1/4)</option>
              <option value="8">8 (1/8)</option>
              <option value="16" selected>16 (1/16 - стандарт)</option>
              <option value="32">32 (1/32)</option>
            </select>
          </div>
        </div>

        <!-- 2. Напрямок та обмеження швидкості -->
        <div class="section-title" style="margin-top: 14px;">Керування двигуном та напрямок</div>
        <div class="form-group" style="display: flex; align-items: center; gap: 10px; margin-bottom: 12px;">
          <input type="checkbox" id="chkInvertDir" style="width: 20px; height: 20px; cursor: pointer;">
          <label for="chkInvertDir" style="margin-bottom: 0; cursor: pointer; font-size: 0.95rem; color: var(--text);">
            Інвертувати напрямок обертання двигуна (DIR)
          </label>
        </div>

        <div class="grid-2">
          <div class="form-group">
            <label for="inputDefSpeed">Базова швидкість (°/с):</label>
            <input type="number" id="inputDefSpeed" value="30" min="1" max="180">
          </div>
          <div class="form-group">
            <label for="inputMaxSpeed">Макс. швидкість (°/с):</label>
            <input type="number" id="inputMaxSpeed" value="180" min="5" max="360">
          </div>
        </div>
        <div class="form-group">
          <label for="inputAccel">Прискорення (°/с²):</label>
          <input type="number" id="inputAccel" value="90" min="10" max="1000">
        </div>

        <!-- 3. Кінцевик та калібрування -->
        <div class="section-title" style="margin-top: 14px;">Кінцевик та калібрування (Homing)</div>
        <div class="form-group" style="display: flex; align-items: center; gap: 10px; margin-bottom: 10px;">
          <input type="checkbox" id="chkEndstopInvert" style="width: 20px; height: 20px; cursor: pointer;">
          <label for="chkEndstopInvert" style="margin-bottom: 0; cursor: pointer; font-size: 0.95rem; color: var(--text);">
            Кінцевик інвертований (активний HIGH замість LOW)
          </label>
        </div>
        <div class="form-group" style="display: flex; align-items: center; gap: 10px; margin-bottom: 12px;">
          <input type="checkbox" id="chkBootHome" style="width: 20px; height: 20px; cursor: pointer;">
          <label for="chkBootHome" style="margin-bottom: 0; cursor: pointer; font-size: 0.95rem; color: var(--text);">
            Автоматичний пошук нуля при увімкненні (Auto-Home on Boot)
          </label>
        </div>

        <div class="grid-2">
          <div class="form-group">
            <label for="inputDebounceMs">Фільтр брязкоту (мс):</label>
            <input type="number" id="inputDebounceMs" value="10" min="0" max="500">
          </div>
          <div class="form-group">
            <label for="selectHomeDir">Напрямок калібрування:</label>
            <select id="selectHomeDir">
              <option value="-1">Проти годинникової (Вліво, -1)</option>
              <option value="1">За годинниковою (Вправо, +1)</option>
            </select>
          </div>
        </div>

        <!-- 4. Піни GPIO -->
        <div class="section-title" style="margin-top: 14px;">Призначення пінів ESP32 (GPIO)</div>
        <div class="grid-2">
          <div class="form-group">
            <label for="inputPinStep">Пін STEP:</label>
            <input type="number" id="inputPinStep" value="18">
          </div>
          <div class="form-group">
            <label for="inputPinDir">Пін DIR:</label>
            <input type="number" id="inputPinDir" value="19">
          </div>
        </div>
        <div class="grid-2">
          <div class="form-group">
            <label for="inputPinEnable">Пін ENABLE (-1 якщо ні):</label>
            <input type="number" id="inputPinEnable" value="5">
          </div>
          <div class="form-group">
            <label for="inputPinEndstop">Пін ENDSTOP:</label>
            <input type="number" id="inputPinEndstop" value="4">
          </div>
        </div>

        <div class="btn-row" style="margin-top: 16px;">
          <button id="btnSaveSettings" class="btn-success" style="flex: 1;" onclick="saveHardwareSettings()">
            💾 Зберегти конфігурацію столу
          </button>
        </div>
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

    <!-- ДОВІДКА ПО API (ЗГОРТАНА) -->
    <details class="card" id="detailsApi">
      <summary class="card-title">
        <span style="display:flex; align-items:center; gap:8px;">
          <span>📖</span>
          <span>Довідка по REST JSON API</span>
        </span>
        <span class="summary-arrow">▼</span>
      </summary>

      <div style="margin-top: 10px;">
        <p><strong>Пошук кінцевика (Homing):</strong></p>
        <pre>POST /api/home</pre>
        
        <p style="margin-top:8px;"><strong>Поворот столу:</strong></p>
        <pre>POST /api/move
Content-Type: application/json

{
  "angle": 90.0,
  "speed": 45.0,
  "relative": false
}</pre>

        <p style="margin-top:8px;"><strong>Зупинка:</strong></p>
        <pre>POST /api/stop</pre>

        <p style="margin-top:8px;"><strong>Опитування стану в реальному часі:</strong></p>
        <pre>GET /api/status</pre>

        <p style="margin-top:8px;"><strong>Налаштування конфігурації столу:</strong></p>
        <pre>GET  /api/settings
POST /api/settings
     {"motor_teeth": 20, "table_teeth": 60, "invert_dir": false, "endstop_debounce_ms": 10}</pre>

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
    let isRelative = false;
    let isStatusUpdating = false;

    function toast(msg, duration = 3000) {
      const t = document.getElementById('toast');
      t.innerHTML = msg;
      t.style.display = 'block';
      if (duration > 0) {
        setTimeout(() => { t.style.display = 'none'; }, duration);
      }
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

    async function startHoming() {
      toast('Запуск пошуку кінцевика...');
      const res = await apiCall('/api/home', {});
      if (res && res.error) toast('Помилка: ' + res.error);
    }

    async function stopMotor() {
      toast('Зупинка двигуна...');
      await apiCall('/api/stop', {});
    }

    async function setZero() {
      const res = await apiCall('/api/zero', {});
      if (res && res.status === 'ok') toast('Встановлено 0°');
    }

    async function sendMove() {
      const angle = parseFloat(document.getElementById('inputAngle').value) || 0;
      const speed = parseFloat(document.getElementById('rangeSpeed').value) || 30;
      toast(`Поворот на ${angle}° зі швидкістю ${speed}°/с...`);
      const res = await apiCall('/api/move', {
        angle: angle,
        speed: speed,
        relative: isRelative
      });
      if (res && res.error) toast('Помилка: ' + res.error);
    }

    // --- РОЗРАХУНОК КІНЕМАТИКИ ТА ШЕСТЕРЕНЬ ---
    function recalcKinematics() {
      const mTeeth = parseFloat(document.getElementById('inputMotorTeeth').value) || 1;
      const tTeeth = parseFloat(document.getElementById('inputTableTeeth').value) || 1;
      const mSteps = parseFloat(document.getElementById('selectMotorSteps').value) || 200;
      const micro = parseFloat(document.getElementById('selectMicrosteps').value) || 16;

      const ratio = tTeeth / mTeeth;
      const stepsDeg = (mSteps * micro * ratio) / 360.0;

      document.getElementById('lblGearRatio').textContent = `${ratio.toFixed(2)} (${mTeeth}:${tTeeth})`;
      document.getElementById('lblStepsPerDeg').textContent = `${stepsDeg.toFixed(2)} кроків/град`;
      document.getElementById('lblLiveRatio').textContent = `Редукція: ${ratio.toFixed(2)}:1`;
    }

    // --- НАЛАШТУВАННЯ АПАРАТУРИ ---
    async function loadHardwareSettings() {
      const res = await apiCall('/api/settings');
      if (!res || res.status !== 'ok') return;

      document.getElementById('inputMotorTeeth').value = res.motor_teeth ?? 20;
      document.getElementById('inputTableTeeth').value = res.table_teeth ?? 60;
      document.getElementById('selectMotorSteps').value = res.steps_per_rev ?? 200;
      document.getElementById('selectMicrosteps').value = res.microsteps ?? 16;

      document.getElementById('chkInvertDir').checked = !!res.invert_dir;
      document.getElementById('inputDefSpeed').value = res.default_speed ?? 30;
      document.getElementById('inputMaxSpeed').value = res.max_speed ?? 180;
      document.getElementById('inputAccel').value = res.acceleration ?? 90;

      document.getElementById('chkEndstopInvert').checked = !!res.endstop_inverted;
      document.getElementById('chkBootHome').checked = !!res.auto_home_on_boot;
      document.getElementById('inputDebounceMs').value = res.endstop_debounce_ms ?? 10;
      document.getElementById('selectHomeDir').value = res.homing_direction ?? -1;

      document.getElementById('inputPinStep').value = res.pin_step ?? 18;
      document.getElementById('inputPinDir').value = res.pin_dir ?? 19;
      document.getElementById('inputPinEnable').value = res.pin_enable ?? 5;
      document.getElementById('inputPinEndstop').value = res.pin_endstop ?? 4;

      recalcKinematics();
    }

    async function saveHardwareSettings() {
      const payload = {
        motor_teeth: parseFloat(document.getElementById('inputMotorTeeth').value),
        table_teeth: parseFloat(document.getElementById('inputTableTeeth').value),
        steps_per_rev: parseFloat(document.getElementById('selectMotorSteps').value),
        microsteps: parseFloat(document.getElementById('selectMicrosteps').value),

        invert_dir: document.getElementById('chkInvertDir').checked,
        default_speed: parseFloat(document.getElementById('inputDefSpeed').value),
        max_speed: parseFloat(document.getElementById('inputMaxSpeed').value),
        acceleration: parseFloat(document.getElementById('inputAccel').value),

        endstop_inverted: document.getElementById('chkEndstopInvert').checked,
        auto_home_on_boot: document.getElementById('chkBootHome').checked,
        endstop_debounce_ms: parseInt(document.getElementById('inputDebounceMs').value),
        homing_direction: parseInt(document.getElementById('selectHomeDir').value),

        pin_step: parseInt(document.getElementById('inputPinStep').value),
        pin_dir: parseInt(document.getElementById('inputPinDir').value),
        pin_enable: parseInt(document.getElementById('inputPinEnable').value),
        pin_endstop: parseInt(document.getElementById('inputPinEndstop').value)
      };

      const btn = document.getElementById('btnSaveSettings');
      btn.disabled = true;
      toast('Збереження налаштувань столу...');

      const res = await apiCall('/api/settings', payload);
      btn.disabled = false;

      if (res && res.status === 'ok') {
        if (res.reboot_required) {
          startRebootCountdown('Піни змінено. Контролер перезавантажується...');
        } else {
          toast('Налаштування столу успішно застосовано!');
          recalcKinematics();
        }
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

    // --- ЖИВЕ ОНОВЛЕННЯ СТАНУ (150 мс) ---
    async function updateStatus() {
      if (isStatusUpdating) return;
      isStatusUpdating = true;

      try {
        const st = await apiCall('/api/status');
        if (!st) return;

        const badge = document.getElementById('badgeState');
        badge.textContent = st.state;
        badge.className = 'badge badge-' + st.state.toLowerCase();

        // Оновлення градусів у реальному часі
        document.getElementById('valAngle').textContent = st.current_angle.toFixed(1);
        document.getElementById('valTarget').textContent = st.target_angle.toFixed(1);
        document.getElementById('valSpeed').textContent = Math.round(st.speed);

        // Індикатор калібрування
        const dotHomed = document.getElementById('dotHomed');
        const txtHomed = document.getElementById('txtHomed');
        if (st.is_homed) {
          dotHomed.className = 'indicator-dot dot-on';
          txtHomed.textContent = 'Відкалібрований (0° OK)';
        } else {
          dotHomed.className = 'indicator-dot dot-off';
          txtHomed.textContent = 'Не відкалібрований';
        }

        // Індикатор кінцевика
        const dotEndstop = document.getElementById('dotEndstop');
        const txtEndstop = document.getElementById('txtEndstop');
        if (st.endstop_triggered) {
          dotEndstop.className = 'indicator-dot dot-on';
          txtEndstop.textContent = 'Натиснутий';
        } else {
          dotEndstop.className = 'indicator-dot dot-off';
          txtEndstop.textContent = 'Розімкнений';
        }

        document.getElementById('txtError').textContent = st.error || '';

        const isBusy = (st.state === 'HOMING');
        document.getElementById('btnHome').disabled = isBusy;
        document.getElementById('btnMove').disabled = isBusy;
      } finally {
        isStatusUpdating = false;
      }
    }

    // Запуск таймера опитування (150 мс для плавної анімації цифр)
    setInterval(updateStatus, 150);
    updateStatus();
    loadHardwareSettings();
    loadWiFiConfig();
  </script>
</body>
</html>
)rawliteral";

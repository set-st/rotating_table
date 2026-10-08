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
    .badge-holding { background: #a371f733; color: #d2a8ff; border: 1px solid #8957e5; animation: pulse 1.5s infinite; }
    .badge-connected { background: #23863633; color: #3fb950; border: 1px solid #238636; }
    .badge-disconnected { background: #da363333; color: #f85149; border: 1px solid #da3633; }
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
      <h1>Поворотний та Нахильний Стіл ESP32</h1>
      <div style="display: flex; gap: 6px; flex-wrap: wrap;">
        <span id="badgeState" class="badge badge-idle">ПОВОРОТ: IDLE</span>
        <span id="badgeTiltState" class="badge badge-disconnected">НАХИЛ: BLE DISCONNECTED</span>
      </div>
    </header>

    <!-- КАРТКА ПОТОЧНОГО СТАНУ ПОВОРОТНОГО СТОЛУ -->
    <div class="card">
      <div class="card-title">
        <span>Поворотний стіл (Live)</span>
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

    <!-- КАРТКА ПОТОЧНОГО СТАНУ ПЛАТФОРМИ НАХИЛУ (BLE LIVE) -->
    <div class="card" id="cardTiltLive">
      <div class="card-title">
        <span>Платформа нахилу (Live Bluetooth)</span>
        <div style="display: flex; align-items: center; gap: 8px;">
          <span id="lblTiltRssi" style="font-size: 0.75rem; color: var(--text-muted); text-transform: none;">RSSI: - dBm</span>
          <span id="badgeTiltConnection" class="badge badge-disconnected">OFFLINE</span>
        </div>
      </div>
      <div class="status-grid">
        <div class="status-item">
          <div class="label">Кут нахилу (GY-521)</div>
          <div class="value"><span id="valTiltAngle">0.00</span>°</div>
          <div class="sub">Ціль: <span id="valTiltTarget">0.0</span>° | Мотор: <span id="valTiltMotorAngle">0.0</span>°</div>
        </div>
        <div class="status-item">
          <div class="label">Швидкість нахилу</div>
          <div class="value"><span id="valTiltSpeed">0</span> <small style="font-size:0.9rem">°/с</small></div>
          <div class="sub">
            <span id="dotTiltHold" class="indicator-dot dot-off"></span><span id="txtTiltHold">Утримання вимкнено</span>
          </div>
        </div>
      </div>
      <div style="margin-top: 12px; font-size: 0.85rem; color: var(--text-muted); display: flex; justify-content: space-between; flex-wrap: wrap; gap: 8px;">
        <div>Кінцевик крайньої точки: <span id="dotTiltEndstop" class="indicator-dot dot-off"></span><span id="txtTiltEndstop" style="color: var(--text);">Розімкнений</span></div>
        <div>База нуля: <span id="dotTiltHomed" class="indicator-dot dot-off"></span><span id="txtTiltHomed" style="color: var(--text);">Не відкалібрований</span></div>
      </div>
      <div class="info-box" style="margin-top: 10px; margin-bottom: 0;">
        <div>Датчик GY-521: Pitch: <span id="valTiltPitch" style="color:#79c0ff">0.00</span>°, Roll: <span id="valTiltRoll" style="color:#79c0ff">0.00</span>° | Стан: <span id="txtTiltGyroState">Очікування зв'язку</span></div>
        <div id="txtTiltError" style="color: var(--danger); font-size: 0.8rem;"></div>
      </div>
    </div>

    <details class="card" id="detailsImu">
      <summary class="card-title">
        <span>IMU GY-87 (Live)</span>
        <span id="badgeImu" class="badge badge-error">IMU OFFLINE</span>
      </summary>
      <div class="status-grid">
        <div class="status-item">
          <div class="label">Нахил Pitch</div>
          <div class="value"><span id="valImuPitch">0.00</span>°</div>
          <div class="sub">Уперед / назад</div>
        </div>
        <div class="status-item">
          <div class="label">Нахил Roll</div>
          <div class="value"><span id="valImuRoll">0.00</span>°</div>
          <div class="sub">Ліворуч / праворуч</div>
        </div>
      </div>
      <div class="info-box" style="margin-top: 12px; margin-bottom: 0;">
        <div>Гіроскоп X/Y/Z: <span id="valImuGyro">0.00 / 0.00 / 0.00 °/с</span></div>
        <div>Датчики: <span id="txtImuSensors">MPU6050 — очікування</span></div>
        <div id="txtImuError" style="color: var(--danger);"></div>
      </div>
      <p style="font-size: 0.85rem; color: var(--text-muted); margin: 12px 0;">
        Для калібрування гіроскопа покладіть IMU нерухомо. Для нульової точки
        спочатку виставте стіл по бульбашковому рівню.
      </p>
      <div class="btn-row">
        <button type="button" class="btn-secondary" id="btnCalibrateGyro" onclick="calibrateGyro()">
          Калібрувати гіроскоп
        </button>
        <button type="button" class="btn-secondary" id="btnZeroImu" onclick="zeroImu()">
          Зберегти поточне положення як 0°
        </button>
      </div>
    </details>

    <!-- КАРТКА КАЛІБРУВАННЯ ТА ОБНУЛЕННЯ -->
    <details class="card" id="detailsCalibration">
      <summary class="card-title">
        <span>Калібрування та нульова точка</span>
        <span class="summary-arrow">▼</span>
      </summary>
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
    </details>

    <!-- КАРТКА КЕРУВАННЯ ПОВОРОТОМ -->
    <details class="card" id="detailsRotation">
      <summary class="card-title">
        <span>Керування поворотом</span>
        <span class="summary-arrow">▼</span>
      </summary>
      
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
          <span id="txtSpeedLabel" style="font-size: 0.9rem; font-weight:600; color:var(--primary)">3 °/с</span>
        </div>
        <input type="range" id="rangeSpeed" min="1" max="180" value="3" oninput="onSpeedChange(this.value)">
      </div>

      <div class="btn-row" style="margin-top: 20px;">
        <button id="btnMove" style="flex: 1; padding: 12px;" onclick="sendMove()">
          Повернути стіл
        </button>
      </div>
    </details>

    <details class="card" id="detailsAutomaticRotation">
      <summary class="card-title">
        <span>Автоматичне обертання</span>
        <span class="summary-arrow">▼</span>
      </summary>
      <p style="font-size: 0.85rem; color: var(--text-muted); margin-bottom: 12px;">
        Повторює відносний поворот на заданий кут через вказаний інтервал.
        Кут може бути від'ємним для обертання вліво.
      </p>
      <div class="grid-2">
        <div class="form-group">
          <label for="inputAutoAngle">Кут одного кроку (°):</label>
          <input type="number" id="inputAutoAngle" value="10" step="0.1">
        </div>
        <div class="form-group">
          <label for="inputAutoSpeed">Швидкість (°/с):</label>
          <input type="number" id="inputAutoSpeed" value="3" min="1" step="0.1">
        </div>
      </div>
      <div class="form-group">
        <label for="inputAutoInterval">Інтервал між поворотами (с):</label>
        <input type="number" id="inputAutoInterval" value="5" min="0.1" step="0.1">
      </div>
      <div class="btn-row">
        <button id="btnAutoStart" class="btn-success" onclick="startAutomaticRotation()">Запустити автоматично</button>
        <button id="btnAutoStop" class="btn-danger" onclick="stopAutomaticRotation()">Зупинити автоматичний режим</button>
      </div>
      <div id="txtAutoStatus" class="info-box" style="margin-top: 10px; margin-bottom: 0;">
        Автоматичний режим вимкнено.
      </div>
    </details>

    <!-- КАРТКА КЕРУВАННЯ ПЛАТФОРМОЮ НАХИЛУ -->
    <details class="card" id="detailsTiltControl" open>
      <summary class="card-title">
        <span>Керування платформою нахилу</span>
        <span class="summary-arrow">▼</span>
      </summary>

      <p style="font-size: 0.85rem; color: var(--text-muted); margin-bottom: 12px;">
        Задайте кут нахилу платформи (додатній — нахил вперед/вгору, від'ємний — назад/вниз).
        Режим стабілізації автоматично підтримує задане положення за гіроскопом GY-521.
      </p>

      <div class="form-group">
        <label for="inputTiltAngle">Цільовий кут нахилу (градуси):</label>
        <div class="input-row">
          <input type="number" id="inputTiltAngle" value="0.0" step="0.5">
          <span style="font-size:1.1rem; color:var(--text-muted)">°</span>
        </div>
        <div class="presets">
          <button class="btn-secondary" onclick="setTiltAnglePreset(-30)">-30°</button>
          <button class="btn-secondary" onclick="setTiltAnglePreset(-15)">-15°</button>
          <button class="btn-secondary" onclick="setTiltAnglePreset(-5)">-5°</button>
          <button class="btn-secondary" onclick="setTiltAnglePreset(0)">0°</button>
          <button class="btn-secondary" onclick="setTiltAnglePreset(5)">+5°</button>
          <button class="btn-secondary" onclick="setTiltAnglePreset(15)">+15°</button>
          <button class="btn-secondary" onclick="setTiltAnglePreset(30)">+30°</button>
        </div>
      </div>

      <div class="form-group" style="margin-top: 16px;">
        <div style="display:flex; justify-content:space-between; margin-bottom: 6px;">
          <label for="rangeTiltSpeed">Швидкість нахилу:</label>
          <span id="txtTiltSpeedLabel" style="font-size: 0.9rem; font-weight:600; color:var(--primary)">5 °/с</span>
        </div>
        <input type="range" id="rangeTiltSpeed" min="1" max="30" value="5" oninput="onTiltSpeedChange(this.value)">
      </div>

      <div class="btn-row" style="margin-top: 16px;">
        <button id="btnTiltMove" class="btn-primary" style="flex: 2; padding: 12px;" onclick="sendTiltMove()">
          Нахилити платформу
        </button>
        <button id="btnTiltToggleHold" class="btn-secondary" style="flex: 2; padding: 12px;" onclick="toggleTiltHold()">
          🛡️ Підтримувати кут (Hold Mode)
        </button>
      </div>

      <div class="btn-row" style="margin-top: 12px; flex-wrap: wrap;">
        <button id="btnTiltHome" class="btn-success" onclick="startTiltHoming()" title="Рух до кінцевика крайньої точки">
          Знайти кінцевик (Home)
        </button>
        <button id="btnTiltZero" class="btn-secondary" onclick="setTiltZero()" title="Встановлює поточну позицію двигуна як 0°">
          Стіл в 0° вручну
        </button>
        <button id="btnTiltGyroZero" class="btn-secondary" onclick="zeroTiltGyro()" title="Обнуляє гіроскоп та зберігає нуль у flash-пам'ять NVS">
          Скинути гіроскоп (Запам'ятати 0°)
        </button>
        <button id="btnTiltCalGyro" class="btn-secondary" onclick="calibrateTiltGyro()" title="Калібрування нульового зміщення сенсора">
          Калібрувати гіроскоп
        </button>
        <button id="btnTiltStop" class="btn-danger" onclick="stopTiltMotor()">СТОП</button>
      </div>
    </details>

    <!-- НАЛАШТУВАННЯ ПЛАТФОРМИ НАХИЛУ (ПО BLUETOOTH) -->
    <details class="card" id="detailsTiltSettings">
      <summary class="card-title">
        <span style="display:flex; align-items:center; gap:8px;">
          <span>⚙️</span>
          <span>Налаштування плати нахилу (Bluetooth)</span>
        </span>
        <span class="summary-arrow">▼</span>
      </summary>

      <div style="margin-top: 10px;">
        <!-- 1. Зв'язок Bluetooth -->
        <div class="section-title">Прив'язка Bluetooth (BLE)</div>
        <p style="font-size: 0.8rem; color: var(--text-muted); margin-bottom: 10px;">
          Пошук та прив'язка другої плати ESP32 платформи нахилу.
        </p>

        <div style="display: flex; gap: 8px; margin-bottom: 12px;">
          <button type="button" id="btnScanTiltBt" class="btn-secondary" onclick="scanTiltBt()">
            🔍 Сканувати Bluetooth пристрої
          </button>
        </div>

        <div id="tiltBtScanContainer" style="display: none; margin-bottom: 12px;">
          <label style="font-size: 0.8rem; color: var(--text-muted); display: block; margin-bottom: 4px;">Знайдені пристрої поблизу:</label>
          <select id="selectTiltBtScan" onchange="onSelectTiltBtDevice(this.value)">
            <option value="">-- Оберіть пристрій --</option>
          </select>
        </div>

        <div class="grid-2">
          <div class="form-group">
            <label for="inputTiltMac">MAC-адреса або ім'я плати:</label>
            <input type="text" id="inputTiltMac" placeholder="XX:XX:XX:XX:XX:XX або TiltTable-ESP32">
          </div>
          <div class="form-group" style="display: flex; align-items: center; gap: 10px; margin-top: 24px;">
            <input type="checkbox" id="chkTiltAutoConnect" checked style="width: 20px; height: 20px; cursor: pointer;">
            <label for="chkTiltAutoConnect" style="margin-bottom: 0; cursor: pointer;">Автоматичне перепідключення</label>
          </div>
        </div>

        <div class="btn-row" style="margin-bottom: 16px;">
          <button type="button" class="btn-success" onclick="connectTiltBt()">Підключити по Bluetooth</button>
          <button type="button" class="btn-secondary" onclick="disconnectTiltBt()">Від'єднати</button>
        </div>

        <!-- 2. Драйвер крокового двигуна на опторозв'язці -->
        <div class="section-title">Драйвер крокового двигуна (Опторозв'язка)</div>
        <p style="font-size: 0.8rem; color: var(--text-muted); margin-bottom: 10px;">
          Драйвер з оптичною розв'язкою (спільний анод +5V/+3.3V на PUL+, DIR+, ENA-) керується низькими рівнями LOW.
        </p>
        <div class="grid-2">
          <div class="form-group">
            <label for="selectTiltStepActive">Активний рівень STEP:</label>
            <select id="selectTiltStepActive">
              <option value="1" selected>LOW (Опторозв'язка - стандарт)</option>
              <option value="0">HIGH</option>
            </select>
          </div>
          <div class="form-group">
            <label for="selectTiltDirPositive">Рівень DIR для додатного нахилу:</label>
            <select id="selectTiltDirPositive">
              <option value="0" selected>LOW</option>
              <option value="1">HIGH</option>
            </select>
          </div>
        </div>
        <div class="form-group">
          <label for="selectTiltEnableActive">Активний рівень ENABLE:</label>
          <select id="selectTiltEnableActive">
            <option value="0" selected>LOW (Опторозв'язка - стандарт)</option>
            <option value="1">HIGH</option>
          </select>
        </div>

        <div class="grid-2">
          <div class="form-group">
            <label for="inputTiltPinStep">Пін STEP (GPIO):</label>
            <input type="number" id="inputTiltPinStep" value="18">
          </div>
          <div class="form-group">
            <label for="inputTiltPinDir">Пін DIR (GPIO):</label>
            <input type="number" id="inputTiltPinDir" value="19">
          </div>
        </div>
        <div class="form-group">
          <label for="inputTiltPinEnable">Пін ENABLE (GPIO, -1 якщо ні):</label>
          <input type="number" id="inputTiltPinEnable" value="5">
        </div>

        <!-- 3. Кінцевик крайньої точки -->
        <div class="section-title" style="margin-top: 14px;">Кінцевик крайньої точки (Endstop)</div>
        <div class="grid-2">
          <div class="form-group">
            <label for="inputTiltPinEndstop">Пін ENDSTOP (GPIO):</label>
            <input type="number" id="inputTiltPinEndstop" value="4">
          </div>
          <div class="form-group">
            <label for="inputTiltDebounceMs">Фільтр брязкоту (мс):</label>
            <input type="number" id="inputTiltDebounceMs" value="10" min="0" max="500">
          </div>
        </div>
        <div class="form-group" style="display: flex; align-items: center; gap: 10px; margin-bottom: 10px;">
          <input type="checkbox" id="chkTiltEndstopInvert" style="width: 20px; height: 20px; cursor: pointer;">
          <label for="chkTiltEndstopInvert" style="margin-bottom: 0; cursor: pointer;">Кінцевик інвертований (активний HIGH замість LOW)</label>
        </div>
        <div class="form-group" style="display: flex; align-items: center; gap: 10px; margin-bottom: 10px;">
          <input type="checkbox" id="chkTiltBootHome" style="width: 20px; height: 20px; cursor: pointer;">
          <label for="chkTiltBootHome" style="margin-bottom: 0; cursor: pointer;">Автоматичний пошук нуля при запуску плати нахилу</label>
        </div>
        <div class="grid-2">
          <div class="form-group">
            <label for="selectTiltHomeDir">Напрямок калібрування:</label>
            <select id="selectTiltHomeDir">
              <option value="-1" selected>У напрямку мінімального кута (-1)</option>
              <option value="1">У напрямку максимального кута (+1)</option>
            </select>
          </div>
          <div class="form-group">
            <label for="inputTiltHomeFast">Швидкість підходу (°/с):</label>
            <input type="number" id="inputTiltHomeFast" value="8" min="0.1" step="0.1">
          </div>
        </div>
        <div class="grid-2">
          <div class="form-group">
            <label for="inputTiltHomeBackoff">Кут відкату (°):</label>
            <input type="number" id="inputTiltHomeBackoff" value="3" min="0.1" step="0.1">
          </div>
          <div class="form-group">
            <label for="inputTiltHomeSlow">Точний підхід (°/с):</label>
            <input type="number" id="inputTiltHomeSlow" value="2" min="0.1" step="0.1">
          </div>
        </div>

        <!-- 4. Гіроскоп GY-521 -->
        <div class="section-title" style="margin-top: 14px;">Гіроскоп GY-521 (MPU-6050)</div>
        <div class="grid-2">
          <div class="form-group">
            <label for="inputTiltPinSda">Пін I2C SDA:</label>
            <input type="number" id="inputTiltPinSda" value="21">
          </div>
          <div class="form-group">
            <label for="inputTiltPinScl">Пін I2C SCL:</label>
            <input type="number" id="inputTiltPinScl" value="22">
          </div>
        </div>
        <div class="grid-2">
          <div class="form-group">
            <label for="inputTiltMpuAddr">Адреса MPU6050 (hex):</label>
            <input type="text" id="inputTiltMpuAddr" value="0x68">
          </div>
          <div class="form-group">
            <label for="selectTiltAxis">Робоча вісь нахилу:</label>
            <select id="selectTiltAxis">
              <option value="0" selected>Pitch (тангаж / нахил вперед-назад)</option>
              <option value="1">Roll (крен / нахил вліво-вправо)</option>
            </select>
          </div>
        </div>
        <div class="grid-2">
          <div class="form-group">
            <label for="inputTiltHoldDeadband">Зона нечутливості утримання (°):</label>
            <input type="number" id="inputTiltHoldDeadband" value="0.2" min="0.05" max="2.0" step="0.05">
          </div>
          <div class="form-group">
            <label for="inputTiltHoldKp">Коефіцієнт P корекції (Kp):</label>
            <input type="number" id="inputTiltHoldKp" value="2.5" min="0.1" max="10.0" step="0.1">
          </div>
        </div>

        <!-- 5. Кінематика платформи нахилу -->
        <div class="section-title" style="margin-top: 14px;">Кінематика та межі нахилу</div>
        <div class="grid-2">
          <div class="form-group">
            <label for="selectTiltMotorSteps">Кроків двигуна на 360°:</label>
            <select id="selectTiltMotorSteps" onchange="recalcTiltKinematics()">
              <option value="200" selected>200 кроків (1.8° - стандарт)</option>
              <option value="400">400 кроків (0.9° - точний)</option>
            </select>
          </div>
          <div class="form-group">
            <label for="selectTiltMicrosteps">Мікрокрок драйвера:</label>
            <select id="selectTiltMicrosteps" onchange="recalcTiltKinematics()">
              <option value="1">1 (Повний крок)</option>
              <option value="2">2 (1/2)</option>
              <option value="4">4 (1/4)</option>
              <option value="8">8 (1/8)</option>
              <option value="16" selected>16 (1/16)</option>
              <option value="32">32 (1/32)</option>
            </select>
          </div>
        </div>
        <div class="section-title">Зубчаста передача механізму нахилу</div>
        <div class="grid-2">
          <div class="form-group">
            <label for="inputTiltMotorTeeth">Шестерня мотора (зубів):</label>
            <input type="number" id="inputTiltMotorTeeth" value="20" min="1" max="200" oninput="recalcTiltKinematics()">
          </div>
          <div class="form-group">
            <label for="inputTiltPlatformTeeth">Шестерня платформи (зубів):</label>
            <input type="number" id="inputTiltPlatformTeeth" value="60" min="1" max="500" oninput="recalcTiltKinematics()">
          </div>
        </div>
        <div class="info-box">
          <div><strong>Передатне число:</strong> <span id="lblTiltGearRatio" style="color:var(--primary); font-weight:700;">3.00 (20:60)</span></div>
          <div><strong>Розраховано імпульсів:</strong> <span id="lblTiltStepsPerDeg" style="color:#79c0ff; font-weight:700;">26.67 кроків/град</span></div>
        </div>
        <div class="grid-2">
          <div class="form-group">
            <label for="inputTiltMinAngle">Мінімальний кут нахилу (°):</label>
            <input type="number" id="inputTiltMinAngle" value="-45" max="0" step="1">
          </div>
          <div class="form-group">
            <label for="inputTiltMaxAngle">Максимальний кут нахилу (°):</label>
            <input type="number" id="inputTiltMaxAngle" value="45" min="0" step="1">
          </div>
        </div>
        <div class="grid-2">
          <div class="form-group">
            <label for="inputTiltDefSpeed">Базова швидкість (°/с):</label>
            <input type="number" id="inputTiltDefSpeed" value="5" min="1" max="60">
          </div>
          <div class="form-group">
            <label for="inputTiltMaxSpeed">Макс. швидкість (°/с):</label>
            <input type="number" id="inputTiltMaxSpeed" value="30" min="1" max="100">
          </div>
        </div>
        <div class="form-group">
          <label for="inputTiltAccel">Прискорення (°/с²):</label>
          <input type="number" id="inputTiltAccel" value="25" min="5" max="200">
        </div>

        <div class="btn-row" style="margin-top: 16px;">
          <button id="btnSaveTiltSettings" class="btn-success" style="flex: 1;" onclick="saveTiltSettings()">
            💾 Зберегти налаштування плати нахилу (по Bluetooth)
          </button>
        </div>
      </div>
    </details>

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
        <div class="section-title">I2C та GY-87</div>
        <p style="font-size: 0.8rem; color: var(--text-muted); margin-bottom: 12px;">
          Зміна I2C-пінів або адрес потребує перезавантаження контролера.
          Типові адреси: MPU6050 — 0x68, BMP180 — 0x77.
        </p>
        <div class="grid-2">
          <div class="form-group">
            <label for="inputI2cSda">Пін I2C SDA:</label>
            <input type="number" id="inputI2cSda" value="21" min="0">
          </div>
          <div class="form-group">
            <label for="inputI2cScl">Пін I2C SCL:</label>
            <input type="number" id="inputI2cScl" value="22" min="0">
          </div>
        </div>
        <div class="form-group">
          <label for="inputMpuAddress">Адреса MPU6050 (hex):</label>
          <input type="text" id="inputMpuAddress" value="0x68">
        </div>
        <div class="form-group">
          <label for="inputBaroAddress">Адреса BMP180 (hex):</label>
          <input type="text" id="inputBaroAddress" value="0x77">
        </div>
        <div class="btn-row">
          <button type="button" class="btn-secondary" onclick="scanI2c()">🔍 Сканувати I2C</button>
        </div>
        <div id="txtI2cScan" class="info-box" style="margin-top: 10px; margin-bottom: 14px;">
          Натисніть «Сканувати I2C», щоб знайти пристрої на шині.
        </div>

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
        <div class="grid-2">
          <div class="form-group">
            <label for="selectDirPositiveLevel">Рівень DIR для додатного напрямку:</label>
            <select id="selectDirPositiveLevel">
              <option value="1">HIGH</option>
              <option value="0">LOW</option>
            </select>
          </div>
          <div class="form-group">
            <label for="selectStepActiveLevel">Активний рівень STEP:</label>
            <select id="selectStepActiveLevel">
              <option value="0">HIGH</option>
              <option value="1">LOW</option>
            </select>
          </div>
        </div>
        <div class="form-group">
          <label for="selectEnableActiveLevel">Активний рівень ENABLE:</label>
          <select id="selectEnableActiveLevel">
            <option value="0">LOW</option>
            <option value="1">HIGH</option>
          </select>
        </div>

        <div class="grid-2">
          <div class="form-group">
            <label for="inputDefSpeed">Базова швидкість (°/с):</label>
            <input type="number" id="inputDefSpeed" value="30" min="1" max="180">
          </div>
          <div class="form-group">
            <label for="inputMaxSpeed">Макс. швидкість (°/с):</label>
            <input type="number" id="inputMaxSpeed" value="180" min="1" max="360" oninput="updateSpeedSliderLimit(this.value)">
          </div>
        </div>
        <div class="form-group">
          <label for="inputAccel">Прискорення (°/с²):</label>
          <input type="number" id="inputAccel" value="90" min="10" max="1000">
        </div>
        <div class="form-group">
          <label for="inputStopDecel">Пригальмовування перед стопом (°/с²):</label>
          <input type="number" id="inputStopDecel" value="60" min="1" max="1000">
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
        <div class="grid-2">
          <div class="form-group">
            <label for="inputHomingFastSpeed">Швидкий підхід (°/с):</label>
            <input type="number" id="inputHomingFastSpeed" value="25" min="0.1" step="0.1">
          </div>
          <div class="form-group">
            <label for="inputHomingBackoffSpeed">Швидкість відкату (°/с):</label>
            <input type="number" id="inputHomingBackoffSpeed" value="3" min="0.1" step="0.1">
          </div>
        </div>
        <div class="form-group">
          <label for="inputHomingSlowSpeed">Повторний точний підхід (°/с):</label>
          <input type="number" id="inputHomingSlowSpeed" value="5" min="0.1" step="0.1">
        </div>
        <div class="form-group">
          <label for="inputRotationLimit">Межа повороту від нуля в кожен бік (°):</label>
          <input type="number" id="inputRotationLimit" value="180" min="1" max="360" step="1">
          <small>Після Homing стіл рухатиметься в межах від −межі до +межі.</small>
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

        <div class="section-title" style="margin-top: 14px;">Апаратні кнопки</div>
        <p style="font-size: 0.8rem; color: var(--text-muted); margin-bottom: 12px;">
          Одне натискання виконує відносний поворот на заданий кут. Кнопка STOP
          негайно зупиняє двигун.
        </p>
        <div class="grid-2">
          <div class="form-group">
            <label for="inputPinButtonLeft">Пін кнопки «Вліво»:</label>
            <input type="number" id="inputPinButtonLeft" value="25">
          </div>
          <div class="form-group">
            <label for="inputPinButtonRight">Пін кнопки «Вправо»:</label>
            <input type="number" id="inputPinButtonRight" value="26">
          </div>
        </div>
        <div class="form-group">
          <label for="inputPinButtonStop">Пін кнопки «STOP»:</label>
          <input type="number" id="inputPinButtonStop" value="27">
        </div>
        <div class="form-group" style="display: flex; align-items: center; gap: 10px;">
          <input type="checkbox" id="chkButtonLeftInvert" style="width: 20px; height: 20px; cursor: pointer;">
          <label for="chkButtonLeftInvert" style="margin-bottom: 0; cursor: pointer; color: var(--text);">Інвертувати кнопку «Вліво»</label>
        </div>
        <div class="form-group" style="display: flex; align-items: center; gap: 10px;">
          <input type="checkbox" id="chkButtonRightInvert" style="width: 20px; height: 20px; cursor: pointer;">
          <label for="chkButtonRightInvert" style="margin-bottom: 0; cursor: pointer; color: var(--text);">Інвертувати кнопку «Вправо»</label>
        </div>
        <div class="form-group" style="display: flex; align-items: center; gap: 10px;">
          <input type="checkbox" id="chkButtonStopInvert" style="width: 20px; height: 20px; cursor: pointer;">
          <label for="chkButtonStopInvert" style="margin-bottom: 0; cursor: pointer; color: var(--text);">Інвертувати кнопку «STOP»</label>
        </div>
        <div class="grid-2">
          <div class="form-group">
            <label for="inputButtonSpeed">Швидкість кнопок (°/с):</label>
            <input type="number" id="inputButtonSpeed" value="3" min="1" step="0.1">
          </div>
          <div class="form-group">
            <label for="inputButtonAngle">Кут за натискання (°):</label>
            <input type="number" id="inputButtonAngle" value="1" min="0.1" step="0.1">
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
          Буде використано BIN-файл з останнього релізу репозиторію
          set-st/rotating_table. Не вимикайте живлення під час оновлення.
        </p>
        <div>Версія основної плати: <strong id="txtCurrentVersion">невідома</strong></div>
        <div id="txtOtaRelease" class="info-box">
          Натисніть «Перевірити реліз», щоб дізнатися про оновлення.
        </div>
        <div class="btn-row">
          <button type="button" class="btn-secondary" onclick="checkOtaRelease()">Перевірити реліз</button>
          <button type="button" id="btnOtaUpdate" class="btn-success" onclick="updateFromGithub()">Оновити прошивку</button>
        </div>
        <div class="section-title" style="margin-top: 16px;">Плата нахилу (Slave)</div>
        <div>Версія плати нахилу: <strong id="txtTiltCurrentVersion">невідома</strong></div>
        <div id="txtTiltOtaRelease" class="info-box">
          Для оновлення плата нахилу має бути підключена через BLE.
        </div>
        <div class="btn-row">
          <button type="button" class="btn-secondary" onclick="checkTiltOtaRelease()">Перевірити реліз Slave</button>
          <button type="button" id="btnTiltOtaUpdate" class="btn-success" onclick="updateTiltFromGithub()">Оновити Slave через BLE</button>
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

        <p style="margin-top:8px;"><strong>Оновлення прошивки:</strong></p>
        <pre>GET  /api/ota/latest
        POST /api/ota/update
        GET  /api/ota/tilt/latest
        POST /api/ota/tilt/update</pre>
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

    function updateSpeedSliderLimit(maxSpeed) {
      const slider = document.getElementById('rangeSpeed');
      const limit = Math.max(1, Number(maxSpeed) || 1);
      slider.max = limit;
      if (Number(slider.value) > limit) {
        slider.value = limit;
      }
      onSpeedChange(slider.value);
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

    async function scanI2c() {
      const output = document.getElementById('txtI2cScan');
      output.textContent = 'Сканування I2C...';
      const res = await apiCall('/api/imu/scan');
      if (!res || res.status !== 'ok') {
        output.textContent = 'Помилка сканування I2C.';
        return;
      }

      if (!res.addresses || res.addresses.length === 0) {
        output.textContent = 'Пристроїв I2C не знайдено.';
        return;
      }
      output.textContent = 'Знайдені адреси: ' +
        res.addresses.map(address => '0x' + address.toString(16).toUpperCase().padStart(2, '0')).join(', ');
    }

    async function calibrateGyro() {
      const btn = document.getElementById('btnCalibrateGyro');
      btn.disabled = true;
      toast('Калібрування гіроскопа: не рухайте IMU приблизно 1 секунду...');
      const res = await apiCall('/api/imu/calibrate', {});
      btn.disabled = false;
      if (res && res.status === 'ok') {
        toast('Гіроскоп успішно відкалібровано.');
      } else {
        toast('Помилка калібрування: ' + (res ? res.error : 'немає відповіді'));
      }
    }

    async function zeroImu() {
      const btn = document.getElementById('btnZeroImu');
      btn.disabled = true;
      const res = await apiCall('/api/imu/zero', {});
      btn.disabled = false;
      if (res && res.status === 'ok') {
        toast('Поточне положення збережено як нульове.');
      } else {
        toast('Помилка обнулення: ' + (res ? res.error : 'немає відповіді'));
      }

    }

    async function startAutomaticRotation() {
      const angle = parseFloat(document.getElementById('inputAutoAngle').value);
      const speed = parseFloat(document.getElementById('inputAutoSpeed').value);
      const interval = parseFloat(document.getElementById('inputAutoInterval').value);
      if (!Number.isFinite(angle) || !Number.isFinite(speed) || !Number.isFinite(interval) ||
          angle === 0 || speed <= 0 || interval < 0.1) {
        toast('Перевірте параметри автоматичного обертання.');
        return;
      }
      const res = await apiCall('/api/automatic/start', {
        angle: angle,
        speed: speed,
        interval_ms: Math.round(interval * 1000)
      });
      if (res && res.status === 'ok') {
        toast('Автоматичний режим запущено.');
      } else {
        toast('Помилка запуску: ' + (res ? res.error : 'немає відповіді'));
      }
    }

    async function stopAutomaticRotation() {
      const res = await apiCall('/api/automatic/stop', {});
      if (res && res.status === 'ok') {
        toast('Автоматичний режим зупинено.');
      }
    }

    // --- НАЛАШТУВАННЯ АПАРАТУРИ ---
    async function loadHardwareSettings() {
      const res = await apiCall('/api/settings');
      if (!res || res.status !== 'ok') return;

      document.getElementById('inputMotorTeeth').value = res.motor_teeth ?? 20;
      document.getElementById('inputTableTeeth').value = res.table_teeth ?? 60;
      document.getElementById('selectMotorSteps').value = res.steps_per_rev ?? 200;
      document.getElementById('selectMicrosteps').value = res.microsteps ?? 16;

      document.getElementById('selectDirPositiveLevel').value = (res.dir_positive_high ?? !res.invert_dir) ? '1' : '0';
      document.getElementById('selectStepActiveLevel').value = res.step_active_low ? '1' : '0';
      document.getElementById('selectEnableActiveLevel').value = res.enable_active_high ? '1' : '0';
      document.getElementById('inputDefSpeed').value = res.default_speed ?? 30;
      document.getElementById('inputMaxSpeed').value = res.max_speed ?? 180;
      updateSpeedSliderLimit(document.getElementById('inputMaxSpeed').value);
      document.getElementById('inputAccel').value = res.acceleration ?? 90;
      document.getElementById('inputStopDecel').value = res.stop_deceleration ?? 60;

      document.getElementById('chkEndstopInvert').checked = !!res.endstop_inverted;
      document.getElementById('chkBootHome').checked = !!res.auto_home_on_boot;
      document.getElementById('inputDebounceMs').value = res.endstop_debounce_ms ?? 10;
      document.getElementById('selectHomeDir').value = res.homing_direction ?? -1;
      document.getElementById('inputHomingFastSpeed').value = res.homing_fast_speed ?? 25;
      document.getElementById('inputHomingBackoffSpeed').value = res.homing_backoff_speed ?? 3;
      document.getElementById('inputHomingSlowSpeed').value = res.homing_slow_speed ?? 5;
      document.getElementById('inputRotationLimit').value = res.rotation_limit_deg ?? 180;

      document.getElementById('inputPinStep').value = res.pin_step ?? 18;
      document.getElementById('inputPinDir').value = res.pin_dir ?? 19;
      document.getElementById('inputPinEnable').value = res.pin_enable ?? 5;
      document.getElementById('inputPinEndstop').value = res.pin_endstop ?? 4;
      document.getElementById('inputPinButtonLeft').value = res.pin_button_left ?? 25;
      document.getElementById('inputPinButtonRight').value = res.pin_button_right ?? 26;
      document.getElementById('inputPinButtonStop').value = res.pin_button_stop ?? 27;
      document.getElementById('chkButtonLeftInvert').checked = !!res.button_left_inverted;
      document.getElementById('chkButtonRightInvert').checked = !!res.button_right_inverted;
      document.getElementById('chkButtonStopInvert').checked = !!res.button_stop_inverted;
      document.getElementById('inputButtonSpeed').value = res.button_move_speed ?? 3;
      document.getElementById('inputButtonAngle').value = res.button_move_angle ?? 1;
      document.getElementById('inputI2cSda').value = res.i2c_sda_pin ?? 21;
      document.getElementById('inputI2cScl').value = res.i2c_scl_pin ?? 22;
      document.getElementById('inputMpuAddress').value = '0x' + (res.mpu6050_address ?? 104).toString(16).toUpperCase();
      document.getElementById('inputBaroAddress').value = '0x' + (res.barometer_address ?? 119).toString(16).toUpperCase();

      recalcKinematics();
    }

    async function saveHardwareSettings() {
      const payload = {
        motor_teeth: parseFloat(document.getElementById('inputMotorTeeth').value),
        table_teeth: parseFloat(document.getElementById('inputTableTeeth').value),
        steps_per_rev: parseFloat(document.getElementById('selectMotorSteps').value),
        microsteps: parseFloat(document.getElementById('selectMicrosteps').value),

        dir_positive_high: document.getElementById('selectDirPositiveLevel').value === '1',
        step_active_low: document.getElementById('selectStepActiveLevel').value === '1',
        enable_active_high: document.getElementById('selectEnableActiveLevel').value === '1',
        default_speed: parseFloat(document.getElementById('inputDefSpeed').value),
        max_speed: parseFloat(document.getElementById('inputMaxSpeed').value),
        acceleration: parseFloat(document.getElementById('inputAccel').value),
        stop_deceleration: parseFloat(document.getElementById('inputStopDecel').value),

        endstop_inverted: document.getElementById('chkEndstopInvert').checked,
        auto_home_on_boot: document.getElementById('chkBootHome').checked,
        endstop_debounce_ms: parseInt(document.getElementById('inputDebounceMs').value),
        homing_direction: parseInt(document.getElementById('selectHomeDir').value),
        homing_fast_speed: parseFloat(document.getElementById('inputHomingFastSpeed').value),
        homing_backoff_speed: parseFloat(document.getElementById('inputHomingBackoffSpeed').value),
        homing_slow_speed: parseFloat(document.getElementById('inputHomingSlowSpeed').value),
        rotation_limit_deg: parseFloat(document.getElementById('inputRotationLimit').value),

        pin_step: parseInt(document.getElementById('inputPinStep').value),
        pin_dir: parseInt(document.getElementById('inputPinDir').value),
        pin_enable: parseInt(document.getElementById('inputPinEnable').value),
        pin_endstop: parseInt(document.getElementById('inputPinEndstop').value),
        pin_button_left: parseInt(document.getElementById('inputPinButtonLeft').value),
        pin_button_right: parseInt(document.getElementById('inputPinButtonRight').value),
        pin_button_stop: parseInt(document.getElementById('inputPinButtonStop').value),
        button_left_inverted: document.getElementById('chkButtonLeftInvert').checked,
        button_right_inverted: document.getElementById('chkButtonRightInvert').checked,
        button_stop_inverted: document.getElementById('chkButtonStopInvert').checked,
        button_move_speed: parseFloat(document.getElementById('inputButtonSpeed').value),
        button_move_angle: parseFloat(document.getElementById('inputButtonAngle').value),
        i2c_sda_pin: parseInt(document.getElementById('inputI2cSda').value),
        i2c_scl_pin: parseInt(document.getElementById('inputI2cScl').value),
        mpu6050_address: parseInt(document.getElementById('inputMpuAddress').value, 0),
        barometer_address: parseInt(document.getElementById('inputBaroAddress').value, 0)
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

    async function checkOtaRelease() {
      const output = document.getElementById('txtOtaRelease');
      output.textContent = 'Перевірка останнього релізу GitHub...';
      const res = await apiCall('/api/ota/latest');
      if (!res || res.status !== 'ok') {
        output.textContent = 'Встановлена версія: ' + (res && res.current_version ? res.current_version : 'невідома') +
          '. Помилка перевірки: ' + (res ? (res.error || 'невідома помилка') : 'немає відповіді');
        return;
      }
      document.getElementById('txtCurrentVersion').textContent = res.current_version || 'невідома';
      const newer = isReleaseNewer(res.current_version, res.tag);
      const status = newer ? 'Доступна нова версія.' : 'Встановлена версія актуальна.';
      output.textContent = `Встановлена: ${res.current_version || 'невідома'}; ` +
        `останній реліз: ${res.tag} (${status}) Файл: ${res.asset} (${Math.round(res.size / 1024)} КБ)`;
    }

    async function updateFromGithub() {
      if (!confirm('Запустити оновлення прошивки з останнього релізу GitHub?')) return;
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
          'Помилка оновлення: ' + (res ? res.message : 'немає відповіді');
      }
    }

    async function checkTiltOtaRelease() {
      const output = document.getElementById('txtTiltOtaRelease');
      output.textContent = 'Перевірка релізу плати нахилу...';
      const res = await apiCall('/api/ota/tilt/latest');
      if (res && res.current_version) {
        document.getElementById('txtTiltCurrentVersion').textContent = res.current_version;
      }
      if (!res || res.status !== 'ok') {
        output.textContent = 'Поточна версія: ' +
          (res && res.current_version ? res.current_version : 'невідома') +
          '. Помилка: ' + (res ? (res.error || 'невідома помилка') : 'немає відповіді');
        return;
      }
      const newer = isReleaseNewer(res.current_version, res.tag);
      output.textContent = `Slave: ${res.current_version || 'невідома'}; останній реліз: ${res.tag}` +
        ` (${newer ? 'доступне оновлення' : 'встановлена версія актуальна'}), файл ${res.asset} (${Math.round(res.size / 1024)} КБ).`;
    }

    async function updateTiltFromGithub() {
      if (!confirm('Передати прошивку останнього релізу на плату нахилу через BLE? Рух зупиниться, slave перезавантажиться; не вимикайте живлення.')) return;
      const btn = document.getElementById('btnTiltOtaUpdate');
      btn.disabled = true;
      const output = document.getElementById('txtTiltOtaRelease');
      output.textContent = 'Завантаження з GitHub і передавання на slave через BLE. Не вимикайте живлення...';
      const res = await apiCall('/api/ota/tilt/update', {});
      btn.disabled = false;
      if (res && res.status === 'ok') {
        output.textContent = res.message;
      } else {
        output.textContent = 'Помилка оновлення Slave: ' +
          (res ? res.message : 'немає відповіді');
      }
    }

    function isReleaseNewer(currentVersion, releaseTag) {
      const current = String(currentVersion || '').replace(/^v/i, '').split('.')
        .map(part => parseInt(part, 10) || 0);
      const latest = String(releaseTag || '').replace(/^v/i, '').split('.')
        .map(part => parseInt(part, 10) || 0);
      for (let i = 0; i < Math.max(current.length, latest.length); i++) {
        if ((latest[i] || 0) !== (current[i] || 0)) {
          return (latest[i] || 0) > (current[i] || 0);
        }
      }
      return false;
    }

    // --- ЖИВЕ ОНОВЛЕННЯ СТАНУ (300 мс) ---
    async function updateStatus() {
      if (isStatusUpdating) return;
      isStatusUpdating = true;

      try {
        const st = await apiCall('/api/status');
        if (!st) return;
        document.getElementById('txtCurrentVersion').textContent =
          st.firmware_version || 'невідома';

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
        document.getElementById('txtAutoStatus').textContent = st.automatic_enabled
          ? `Автоматичний режим: ${st.automatic_angle.toFixed(1)}° кожні ${(st.automatic_interval_ms / 1000).toFixed(1)} с`
          : 'Автоматичний режим вимкнено.';
        const imuOnline = !!st.imu_initialized && !!st.imu_mpu6050_connected;
        document.getElementById('badgeImu').textContent = imuOnline ? 'ONLINE' : 'OFFLINE';
        document.getElementById('badgeImu').className = 'badge ' + (imuOnline ? 'badge-idle' : 'badge-error');
        document.getElementById('valImuPitch').textContent = (st.imu_pitch_deg || 0).toFixed(2);
        document.getElementById('valImuRoll').textContent = (st.imu_roll_deg || 0).toFixed(2);
        document.getElementById('valImuGyro').textContent =
          `${(st.imu_gyro_x_dps || 0).toFixed(2)} / ${(st.imu_gyro_y_dps || 0).toFixed(2)} / ${(st.imu_gyro_z_dps || 0).toFixed(2)} °/с`;
        document.getElementById('txtImuSensors').textContent =
          `MPU6050: ${st.imu_mpu6050_connected ? 'OK' : '—'}; ` +
          `BMP180: ${st.imu_barometer_connected ? 'OK' : '—'}`;
        document.getElementById('txtImuError').textContent = st.imu_error || '';
        const isBusy = (st.state === 'HOMING');
        document.getElementById('btnHome').disabled = isBusy;
        document.getElementById('btnMove').disabled = isBusy;
      } finally {
        isStatusUpdating = false;
      }
    }

    // =========================================================================
    // JAVASCRIPT ФУНКЦІЇ ДЛЯ ПЛАТФОРМИ НАХИЛУ (BLUETOOTH BLE)
    // =========================================================================
    let isTiltUpdating = false;
    let tiltHoldActive = false;

    function setTiltAnglePreset(val) {
      document.getElementById('inputTiltAngle').value = val;
    }

    function onTiltSpeedChange(val) {
      document.getElementById('txtTiltSpeedLabel').textContent = val + ' °/с';
    }

    async function sendTiltMove() {
      const angle = parseFloat(document.getElementById('inputTiltAngle').value) || 0;
      const speed = parseFloat(document.getElementById('rangeTiltSpeed').value) || 5;
      toast(`Нахил на ${angle}° зі швидкістю ${speed}°/с...`);
      const res = await apiCall('/api/tilt/move', { angle: angle, speed: speed, relative: false });
      if (res && res.error) toast('Помилка нахилу: ' + res.error);
    }

    async function startTiltHoming() {
      toast('Запуск пошуку кінцевика крайньої точки нахилу...');
      const res = await apiCall('/api/tilt/home', {});
      if (res && res.error) toast('Помилка: ' + res.error);
    }

    async function stopTiltMotor() {
      toast('Зупинка нахилу...');
      await apiCall('/api/tilt/stop', {});
    }

    async function setTiltZero() {
      const res = await apiCall('/api/tilt/zero', {});
      if (res && res.status === 'ok') toast('Поточну позицію нахилу виставлено в 0.0°');
      else toast('Помилка: ' + (res ? res.error : 'немає відповіді'));
    }

    async function zeroTiltGyro() {
      if (!confirm('Зберегти поточне фізичне положення як 0.0° в енергонезалежну пам\'ять гіроскопа?')) return;
      toast('Скидання гіроскопа та запис у flash NVS...');
      const res = await apiCall('/api/tilt/gyro/zero', {});
      if (res && res.status === 'ok') toast('Нуль гіроскопа GY-521 збережено в пам\'ять.');
      else toast('Помилка скидання: ' + (res ? res.error : 'немає відповіді'));
    }

    async function calibrateTiltGyro() {
      toast('Калібрування гіроскопа нахилу (не рухайте стіл 1-2 сек)...');
      const res = await apiCall('/api/tilt/gyro/calibrate', {});
      if (res && res.status === 'ok') toast('Гіроскоп нахилу успішно відкалібровано.');
      else toast('Помилка: ' + (res ? res.error : 'немає відповіді'));
    }

    async function toggleTiltHold() {
      const newHoldState = !tiltHoldActive;
      const target = parseFloat(document.getElementById('inputTiltAngle').value) || 0;
      toast(newHoldState ? 'Увімкнення підтримання кута (Hold mode)...' : 'Вимкнення підтримання кута...');
      const res = await apiCall('/api/tilt/hold', { enabled: newHoldState, target: target });
      if (res && res.status === 'ok') {
        tiltHoldActive = newHoldState;
        toast(newHoldState ? 'Режим стабілізації АКТИВОВАНО.' : 'Режим стабілізації ВИМКНЕНО.');
      } else {
        toast('Помилка перемикання: ' + (res ? res.error : 'немає відповіді'));
      }
    }

    async function scanTiltBt() {
      const btn = document.getElementById('btnScanTiltBt');
      btn.disabled = true;
      btn.textContent = '🔍 Сканування Bluetooth (3 сек)...';
      toast('Сканування Bluetooth ефіру...');
      const res = await apiCall('/api/tilt/bt/scan');
      btn.disabled = false;
      btn.textContent = '🔍 Сканувати Bluetooth пристрої';

      if (!res || !res.devices) {
        toast('Помилка сканування Bluetooth.');
        return;
      }

      const select = document.getElementById('selectTiltBtScan');
      select.innerHTML = '<option value="">-- Оберіть знайдений пристрій --</option>';

      if (res.devices.length === 0) {
        toast('Пристроїв BLE не виявлено.');
        return;
      }

      res.devices.sort((a, b) => b.rssi - a.rssi);
      res.devices.forEach(d => {
        const opt = document.createElement('option');
        opt.value = d.address;
        const name = d.name || 'Невідомий пристрій';
        opt.textContent = `${name} [${d.address}] (${d.rssi} dBm)`;
        select.appendChild(opt);
      });

      document.getElementById('tiltBtScanContainer').style.display = 'block';
      toast(`Знайдено ${res.devices.length} BLE пристроїв`);
    }

    function onSelectTiltBtDevice(addr) {
      if (!addr) return;
      document.getElementById('inputTiltMac').value = addr;
    }

    async function connectTiltBt() {
      const mac = document.getElementById('inputTiltMac').value.trim();
      const autoConn = document.getElementById('chkTiltAutoConnect').checked;
      toast('Підключення до плати нахилу через Bluetooth...');
      const res = await apiCall('/api/tilt/bt/connect', { address: mac, auto_connect: autoConn });
      if (res && res.status === 'ok') {
        toast('Команду підключення прийнято.');
      } else {
        toast('Помилка: ' + (res ? res.message : 'немає зв\'язку'));
      }
    }

    async function disconnectTiltBt() {
      toast('Відключення від плати нахилу...');
      await apiCall('/api/tilt/bt/connect', { disconnect: true, auto_connect: false });
      toast('Відключено.');
    }

    async function loadTiltSettings() {
      const res = await apiCall('/api/tilt/settings');
      if (!res || res.status === 'error') return;

      if (res.pin_step !== undefined) document.getElementById('inputTiltPinStep').value = res.pin_step;
      if (res.pin_dir !== undefined) document.getElementById('inputTiltPinDir').value = res.pin_dir;
      if (res.pin_enable !== undefined) document.getElementById('inputTiltPinEnable').value = res.pin_enable;
      if (res.step_active_low !== undefined) document.getElementById('selectTiltStepActive').value = res.step_active_low ? '1' : '0';
      if (res.dir_positive_high !== undefined) document.getElementById('selectTiltDirPositive').value = res.dir_positive_high ? '1' : '0';
      if (res.enable_active_high !== undefined) document.getElementById('selectTiltEnableActive').value = res.enable_active_high ? '1' : '0';

      if (res.pin_endstop !== undefined) document.getElementById('inputTiltPinEndstop').value = res.pin_endstop;
      if (res.endstop_inverted !== undefined) document.getElementById('chkTiltEndstopInvert').checked = !!res.endstop_inverted;
      if (res.endstop_debounce_ms !== undefined) document.getElementById('inputTiltDebounceMs').value = res.endstop_debounce_ms;

      if (res.pin_sda !== undefined) document.getElementById('inputTiltPinSda').value = res.pin_sda;
      if (res.pin_scl !== undefined) document.getElementById('inputTiltPinScl').value = res.pin_scl;
      if (res.mpu_addr !== undefined) document.getElementById('inputTiltMpuAddr').value = '0x' + Number(res.mpu_addr).toString(16).toUpperCase();
      if (res.tilt_axis !== undefined) document.getElementById('selectTiltAxis').value = String(res.tilt_axis);

      if (res.steps_per_rev !== undefined) document.getElementById('selectTiltMotorSteps').value = String(res.steps_per_rev);
      if (res.microsteps !== undefined) document.getElementById('selectTiltMicrosteps').value = String(res.microsteps);
      if (res.gear_ratio !== undefined) {
        document.getElementById('inputTiltMotorTeeth').value = '20';
        document.getElementById('inputTiltPlatformTeeth').value = String(20 * Number(res.gear_ratio));
      }
      recalcTiltKinematics();

      if (res.min_angle !== undefined) document.getElementById('inputTiltMinAngle').value = res.min_angle;
      if (res.max_angle !== undefined) document.getElementById('inputTiltMaxAngle').value = res.max_angle;
      if (res.def_speed !== undefined) document.getElementById('inputTiltDefSpeed').value = res.def_speed;
      if (res.max_speed !== undefined) document.getElementById('inputTiltMaxSpeed').value = res.max_speed;
      if (res.accel !== undefined) document.getElementById('inputTiltAccel').value = res.accel;

      if (res.homing_dir !== undefined) document.getElementById('selectTiltHomeDir').value = String(res.homing_dir);
      if (res.homing_fast_speed !== undefined) document.getElementById('inputTiltHomeFast').value = res.homing_fast_speed;
      if (res.homing_slow_speed !== undefined) document.getElementById('inputTiltHomeSlow').value = res.homing_slow_speed;
      if (res.homing_backoff_deg !== undefined) document.getElementById('inputTiltHomeBackoff').value = res.homing_backoff_deg;
      if (res.auto_home !== undefined) document.getElementById('chkTiltBootHome').checked = !!res.auto_home;

      if (res.hold_deadband !== undefined) document.getElementById('inputTiltHoldDeadband').value = res.hold_deadband;
      if (res.hold_kp !== undefined) document.getElementById('inputTiltHoldKp').value = res.hold_kp;
    }

    function recalcTiltKinematics() {
      const motorTeeth = parseFloat(document.getElementById('inputTiltMotorTeeth').value) || 1;
      const platformTeeth = parseFloat(document.getElementById('inputTiltPlatformTeeth').value) || 1;
      const motorSteps = parseFloat(document.getElementById('selectTiltMotorSteps').value) || 200;
      const microsteps = parseFloat(document.getElementById('selectTiltMicrosteps').value) || 16;
      const ratio = platformTeeth / motorTeeth;
      const stepsPerDegree = (motorSteps * microsteps * ratio) / 360;

      document.getElementById('lblTiltGearRatio').textContent =
        `${ratio.toFixed(2)} (${motorTeeth}:${platformTeeth})`;
      document.getElementById('lblTiltStepsPerDeg').textContent =
        `${stepsPerDegree.toFixed(2)} кроків/град`;
    }

    async function saveTiltSettings() {
      if (!confirm('Зберегти налаштування на платі нахилу? Деякі зміни можуть спричинити перезавантаження плати.')) return;

      const mpuRaw = document.getElementById('inputTiltMpuAddr').value.trim();
      const mpuParsed = parseInt(mpuRaw, 16);

      const payload = {
        pin_step: parseInt(document.getElementById('inputTiltPinStep').value),
        pin_dir: parseInt(document.getElementById('inputTiltPinDir').value),
        pin_enable: parseInt(document.getElementById('inputTiltPinEnable').value),
        step_active_low: document.getElementById('selectTiltStepActive').value === '1',
        dir_positive_high: document.getElementById('selectTiltDirPositive').value === '1',
        enable_active_high: document.getElementById('selectTiltEnableActive').value === '1',

        pin_endstop: parseInt(document.getElementById('inputTiltPinEndstop').value),
        endstop_inverted: document.getElementById('chkTiltEndstopInvert').checked,
        endstop_debounce_ms: parseInt(document.getElementById('inputTiltDebounceMs').value),

        pin_sda: parseInt(document.getElementById('inputTiltPinSda').value),
        pin_scl: parseInt(document.getElementById('inputTiltPinScl').value),
        mpu_addr: isNaN(mpuParsed) ? 104 : mpuParsed,
        tilt_axis: parseInt(document.getElementById('selectTiltAxis').value),

        steps_per_rev: parseFloat(document.getElementById('selectTiltMotorSteps').value),
        microsteps: parseFloat(document.getElementById('selectTiltMicrosteps').value),
        gear_ratio: (parseFloat(document.getElementById('inputTiltPlatformTeeth').value) || 1) /
          (parseFloat(document.getElementById('inputTiltMotorTeeth').value) || 1),

        min_angle: parseFloat(document.getElementById('inputTiltMinAngle').value),
        max_angle: parseFloat(document.getElementById('inputTiltMaxAngle').value),
        def_speed: parseFloat(document.getElementById('inputTiltDefSpeed').value),
        max_speed: parseFloat(document.getElementById('inputTiltMaxSpeed').value),
        accel: parseFloat(document.getElementById('inputTiltAccel').value),

        homing_dir: parseInt(document.getElementById('selectTiltHomeDir').value),
        homing_fast_speed: parseFloat(document.getElementById('inputTiltHomeFast').value),
        homing_slow_speed: parseFloat(document.getElementById('inputTiltHomeSlow').value),
        homing_backoff_deg: parseFloat(document.getElementById('inputTiltHomeBackoff').value),
        auto_home: document.getElementById('chkTiltBootHome').checked,

        hold_deadband: parseFloat(document.getElementById('inputTiltHoldDeadband').value),
        hold_kp: parseFloat(document.getElementById('inputTiltHoldKp').value)
      };

      toast('Надсилання налаштувань по Bluetooth...');
      const res = await apiCall('/api/tilt/settings', payload);
      if (res && res.status === 'ok') {
        toast('Налаштування плати нахилу успішно збережено!');
      } else {
        toast('Помилка збереження: ' + (res ? res.error : 'немає відповіді'));
      }
    }

    async function updateTiltStatus() {
      if (isTiltUpdating) return;
      isTiltUpdating = true;

      try {
        const st = await apiCall('/api/tilt/status');
        if (!st) return;

        const isConn = !!st.connected;
        const badgeConn = document.getElementById('badgeTiltConnection');
        badgeConn.textContent = isConn ? 'BLE ONLINE' : 'BLE OFFLINE';
        badgeConn.className = 'badge ' + (isConn ? 'badge-connected' : 'badge-disconnected');

        const badgeHdr = document.getElementById('badgeTiltState');
        badgeHdr.textContent = 'НАХИЛ: ' + (isConn ? (st.state || 'IDLE') : 'DISCONNECTED');
        badgeHdr.className = 'badge badge-' + (isConn ? String(st.state || 'idle').toLowerCase() : 'disconnected');

        document.getElementById('lblTiltRssi').textContent = isConn ? `RSSI: ${st.rssi || 0} dBm` : 'RSSI: - dBm';
        if (st.target_address && !document.getElementById('inputTiltMac').value) {
          document.getElementById('inputTiltMac').value = st.target_address;
        }

        // Кути нахилу
        document.getElementById('valTiltAngle').textContent = (st.angle || 0).toFixed(2);
        document.getElementById('valTiltTarget').textContent = (st.target_angle || 0).toFixed(1);
        document.getElementById('valTiltMotorAngle').textContent = (st.motor_angle || 0).toFixed(2);
        document.getElementById('valTiltSpeed').textContent = Math.round(st.speed || 0);

        // Режим підтримання кута (Hold)
        tiltHoldActive = !!st.hold_active;
        const dotHold = document.getElementById('dotTiltHold');
        const txtHold = document.getElementById('txtTiltHold');
        if (tiltHoldActive) {
          dotHold.className = 'indicator-dot dot-on';
          txtHold.textContent = 'Утримання АКТИВНЕ (Стабілізація)';
          document.getElementById('btnTiltToggleHold').classList.add('active');
        } else {
          dotHold.className = 'indicator-dot dot-off';
          txtHold.textContent = 'Утримання вимкнено';
          document.getElementById('btnTiltToggleHold').classList.remove('active');
        }

        // Кінцевик нахилу
        const dotEnd = document.getElementById('dotTiltEndstop');
        const txtEnd = document.getElementById('txtTiltEndstop');
        if (st.endstop_triggered) {
          dotEnd.className = 'indicator-dot dot-on';
          txtEnd.textContent = 'Натиснутий (Крайня точка)';
        } else {
          dotEnd.className = 'indicator-dot dot-off';
          txtEnd.textContent = 'Розімкнений';
        }

        // Homing нахилу
        const dotHome = document.getElementById('dotTiltHomed');
        const txtHome = document.getElementById('txtTiltHomed');
        if (st.is_homed) {
          dotHome.className = 'indicator-dot dot-on';
          txtHome.textContent = 'Відкалібрований';
        } else {
          dotHome.className = 'indicator-dot dot-off';
          txtHome.textContent = 'Не відкалібрований';
        }

        // Дані GY-521
        document.getElementById('valTiltPitch').textContent = (st.pitch || 0).toFixed(2);
        document.getElementById('valTiltRoll').textContent = (st.roll || 0).toFixed(2);
        document.getElementById('txtTiltGyroState').textContent = st.gyro_connected ? 'MPU-6050 OK' : 'Немає зв\'язку';
        document.getElementById('txtTiltError').textContent = st.error || '';

        const isBusy = (st.state === 'HOMING');
        document.getElementById('btnTiltHome').disabled = isBusy || !isConn;
        document.getElementById('btnTiltMove').disabled = isBusy || !isConn;
      } finally {
        isTiltUpdating = false;
      }
    }

    // Запуск таймерів опитування обох плат (300 мс)
    setInterval(updateStatus, 300);
    setInterval(updateTiltStatus, 300);
    updateStatus();
    updateTiltStatus();
    loadHardwareSettings();
    loadTiltSettings();
    loadWiFiConfig();
  </script>
</body>
</html>
)rawliteral";

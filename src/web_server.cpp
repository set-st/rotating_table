#include "web_server.h"
#include "web_ui.h"
#include "motion_controller.h"
#include "wifi_manager.h"
#include "config.h"
#include "imu_sensor.h"
#include "tilt_controller.h"
#include "ota_updater.h"
#include <ArduinoJson.h>

TableWebServer webServer;

TableWebServer::TableWebServer() : server(WEB_SERVER_PORT) {}

void TableWebServer::begin() {
    setupRoutes();
    server.begin();
    Serial.printf("[Web] HTTP-сервер запущено на порту %d\n", WEB_SERVER_PORT);
}

void TableWebServer::handleClient() {
    server.handleClient();
}

void TableWebServer::sendCorsHeaders() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void TableWebServer::handleOptions() {
    sendCorsHeaders();
    server.send(204); // Немає вмісту
}

void TableWebServer::setupRoutes() {
    // Головна веб-сторінка
    server.on("/", HTTP_GET, [this]() { handleRoot(); });

    // CORS preflight запити
    server.on("/api/status", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/home", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/move", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/stop", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/zero", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/settings", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/imu/scan", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/imu/calibrate", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/imu/zero", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/automatic/start", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/automatic/stop", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/tilt/status", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/tilt/settings", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/tilt/move", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/tilt/level", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/tilt/stop", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/tilt/home", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/ota/latest", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/ota/update", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/wifi/config", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/wifi/scan", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/wifi/save", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/wifi/reset", HTTP_OPTIONS, [this]() { handleOptions(); });

    // REST API ендпоінти
    server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    server.on("/api/home", HTTP_POST, [this]() { handleHome(); });
    server.on("/api/move", HTTP_POST, [this]() { handleMove(); });
    server.on("/api/stop", HTTP_POST, [this]() { handleStop(); });
    server.on("/api/zero", HTTP_POST, [this]() { handleZero(); });

    // Налаштування кінцевика
    server.on("/api/settings", HTTP_GET, [this]() { handleGetSettings(); });
    server.on("/api/settings", HTTP_POST, [this]() { handleSaveSettings(); });
    server.on("/api/imu/scan", HTTP_GET, [this]() { handleImuScan(); });
    server.on("/api/imu/calibrate", HTTP_POST, [this]() { handleImuCalibrate(); });
    server.on("/api/imu/zero", HTTP_POST, [this]() { handleImuZero(); });
    server.on("/api/automatic/start", HTTP_POST, [this]() { handleAutomaticStart(); });
    server.on("/api/automatic/stop", HTTP_POST, [this]() { handleAutomaticStop(); });
    server.on("/api/tilt/status", HTTP_GET, [this]() { handleTiltStatus(); });
    server.on("/api/tilt/settings", HTTP_GET, [this]() { handleTiltSettings(); });
    server.on("/api/tilt/settings", HTTP_POST, [this]() { handleTiltSettings(); });
    server.on("/api/tilt/move", HTTP_POST, [this]() { handleTiltMove(); });
    server.on("/api/tilt/level", HTTP_POST, [this]() { handleTiltLevel(); });
    server.on("/api/tilt/stop", HTTP_POST, [this]() { handleTiltStop(); });
    server.on("/api/tilt/home", HTTP_POST, [this]() { handleTiltHome(); });
    server.on("/api/ota/latest", HTTP_GET, [this]() { handleOtaLatest(); });
    server.on("/api/ota/update", HTTP_POST, [this]() { handleOtaUpdate(); });

    // Налаштування Wi-Fi
    server.on("/api/wifi/config", HTTP_GET, [this]() { handleWiFiConfig(); });
    server.on("/api/wifi/scan", HTTP_GET, [this]() { handleWiFiScan(); });
    server.on("/api/wifi/save", HTTP_POST, [this]() { handleWiFiSave(); });
    server.on("/api/wifi/reset", HTTP_POST, [this]() { handleWiFiReset(); });

    // 404 Сторінку не знайдено
    server.onNotFound([this]() { handleNotFound(); });
}

void TableWebServer::handleRoot() {
    sendCorsHeaders();
    server.send_P(200, "text/html", PAGE_INDEX);
}

void TableWebServer::handleStatus() {
    sendCorsHeaders();
    MotionStatus st = motionCtrl.getStatus();

    JsonDocument doc;
    doc["status"] = "ok";
    doc["state"] = st.stateStr;
    doc["current_angle"] = st.currentAngle;
    doc["target_angle"] = st.targetAngle;
    doc["speed"] = st.currentSpeed;
    doc["is_homed"] = st.isHomed;
    doc["endstop_triggered"] = st.endstopTriggered;
    doc["automatic_enabled"] = st.automaticEnabled;
    doc["automatic_angle"] = st.automaticAngle;
    doc["automatic_speed"] = st.automaticSpeed;
    doc["automatic_interval_ms"] = st.automaticIntervalMs;
    ImuStatus imu = imuSensor.getStatus();
    doc["imu_initialized"] = imu.initialized;
    doc["imu_mpu6050_connected"] = imu.mpu6050Connected;
    doc["imu_barometer_connected"] = imu.barometerConnected;
    doc["imu_pitch_deg"] = imu.pitchDeg;
    doc["imu_roll_deg"] = imu.rollDeg;
    doc["imu_gyro_x_dps"] = imu.gyroXDegS;
    doc["imu_gyro_y_dps"] = imu.gyroYDegS;
    doc["imu_gyro_z_dps"] = imu.gyroZDegS;
    doc["imu_updated_ms"] = imu.updatedAtMs;
    TiltStatus tilt = tiltController.getStatus();
    doc["tilt_enabled"] = tilt.enabled;
    doc["tilt_moving"] = tilt.moving;
    doc["tilt_height_mm"] = tilt.heightMm;
    doc["tilt_pitch_deg"] = tilt.pitchDeg;
    doc["tilt_roll_deg"] = tilt.rollDeg;
    doc["tilt_target_height_mm"] = tilt.targetHeightMm;
    doc["tilt_target_pitch_deg"] = tilt.targetPitchDeg;
    doc["tilt_target_roll_deg"] = tilt.targetRollDeg;
    if (strlen(imu.errorMessage) > 0) {
        doc["imu_error"] = imu.errorMessage;
    }
    if (strlen(st.errorMessage) > 0) {
        doc["error"] = st.errorMessage;
    }

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void TableWebServer::handleImuScan() {
    sendCorsHeaders();
    ImuScanResult scan = imuSensor.scanBus();
    JsonDocument doc;
    doc["status"] = "ok";
    JsonArray addresses = doc["addresses"].to<JsonArray>();
    for (uint8_t i = 0; i < scan.count; ++i) {
        addresses.add(scan.addresses[i]);
    }
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void TableWebServer::handleImuCalibrate() {
    sendCorsHeaders();
    JsonDocument doc;
    doc["status"] = imuSensor.calibrateGyro() ? "ok" : "error";
    if (doc["status"] == "error") {
        doc["error"] = "Не вдалося відкалібрувати гіроскоп. Перевірте підключення та нерухомість датчика.";
    }
    String response;
    serializeJson(doc, response);
    server.send(doc["status"] == "ok" ? 200 : 500, "application/json", response);
}

void TableWebServer::handleImuZero() {
    sendCorsHeaders();
    JsonDocument doc;
    doc["status"] = imuSensor.zeroOrientation() ? "ok" : "error";
    if (doc["status"] == "error") {
        doc["error"] = "IMU не ініціалізовано";
    }
    String response;
    serializeJson(doc, response);
    server.send(doc["status"] == "ok" ? 200 : 500, "application/json", response);
}

void TableWebServer::handleAutomaticStart() {
    sendCorsHeaders();
    JsonDocument doc;
    if (!server.hasArg("plain") ||
        deserializeJson(doc, server.arg("plain"))) {
        doc["status"] = "error";
        doc["error"] = "Некоректне тіло запиту";
        String response;
        serializeJson(doc, response);
        server.send(400, "application/json", response);
        return;
    }

    const float angle = doc["angle"] | 0.0f;
    const float speed = doc["speed"] | 0.0f;
    const uint32_t intervalMs = doc["interval_ms"] | 0UL;
    JsonDocument responseDoc;
    if (!motionCtrl.startAutomatic(angle, speed, intervalMs)) {
        responseDoc["status"] = "error";
        responseDoc["error"] = "Перевірте кут, швидкість та інтервал (мінімум 100 мс)";
        String response;
        serializeJson(responseDoc, response);
        server.send(400, "application/json", response);
        return;
    }
    responseDoc["status"] = "ok";
    responseDoc["message"] = "Автоматичний режим запущено";
    String response;
    serializeJson(responseDoc, response);
    server.send(200, "application/json", response);
}

void TableWebServer::handleAutomaticStop() {
    sendCorsHeaders();
    motionCtrl.stopAutomatic();
    JsonDocument doc;
    doc["status"] = "ok";
    doc["message"] = "Автоматичний режим зупинено";
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void TableWebServer::handleOtaLatest() {
    sendCorsHeaders();
    OtaReleaseInfo info = otaUpdater.getLatestRelease();
    JsonDocument doc;
    doc["status"] = info.available ? "ok" : "error";
    if (info.available) {
        doc["tag"] = info.tagName;
        doc["asset"] = info.assetName;
        doc["size"] = info.assetSize;
    } else {
        doc["error"] = info.error;
    }
    String response;
    serializeJson(doc, response);
    server.send(info.available ? 200 : 502, "application/json", response);
}

void TableWebServer::handleOtaUpdate() {
    sendCorsHeaders();
    if (!wifiMgr.isConnectedSTA()) {
        JsonDocument doc;
        doc["status"] = "error";
        doc["message"] = "OTA доступне лише при підключенні до роутера з Інтернетом; SoftAP не має виходу в Інтернет";
        String response;
        serializeJson(doc, response);
        server.send(503, "application/json", response);
        return;
    }
    String error;
    const bool success = otaUpdater.installLatestRelease(error);
    JsonDocument doc;
    doc["status"] = success ? "ok" : "error";
    doc["message"] = success ? "Оновлення записано. Контролер перезавантажується..."
                             : error;
    String response;
    serializeJson(doc, response);
    server.send(success ? 200 : 502, "application/json", response);
    if (success) {
        wifiMgr.scheduleRestart(1500);
    }
}

void TableWebServer::handleHome() {
    sendCorsHeaders();

    tiltController.emergencyStop();
    if (motionCtrl.startHoming()) {
        JsonDocument doc;
        doc["status"] = "ok";
        doc["message"] = "Калібрування запущено";
        String res;
        serializeJson(doc, res);
        server.send(200, "application/json", res);
    } else {
        JsonDocument doc;
        doc["status"] = "error";
        doc["error"] = "Неможливо запустити калібрування (контролер зайнятий)";
        String res;
        serializeJson(doc, res);
        server.send(409, "application/json", res);
    }
}

void TableWebServer::handleMove() {
    sendCorsHeaders();

    if (!server.hasArg("plain")) {
        JsonDocument errDoc;
        errDoc["status"] = "error";
        errDoc["error"] = "Відсутнє тіло запиту";
        String res;
        serializeJson(errDoc, res);
        server.send(400, "application/json", res);
        return;
    }

    JsonDocument reqDoc;
    DeserializationError err = deserializeJson(reqDoc, server.arg("plain"));
    if (err) {
        JsonDocument errDoc;
        errDoc["status"] = "error";
        errDoc["error"] = "Некоректний формат JSON";
        String res;
        serializeJson(errDoc, res);
        server.send(400, "application/json", res);
        return;
    }

    if (!reqDoc["angle"].is<float>()) {
        JsonDocument errDoc;
        errDoc["status"] = "error";
        errDoc["error"] = "Поле 'angle' (float) є обов'язковим";
        String res;
        serializeJson(errDoc, res);
        server.send(400, "application/json", res);
        return;
    }

    float angle = reqDoc["angle"].as<float>();
    float speed = reqDoc["speed"] | DEFAULT_SPEED_DEG_S;
    bool relative = reqDoc["relative"] | false;

    if (motionCtrl.moveTo(angle, speed, relative)) {
        JsonDocument respDoc;
        respDoc["status"] = "ok";
        respDoc["message"] = "Команду повороту прийнято";
        respDoc["target_angle"] = angle;
        respDoc["speed"] = speed;
        respDoc["relative"] = relative;
        String res;
        serializeJson(respDoc, res);
        server.send(200, "application/json", res);
    } else {
        JsonDocument errDoc;
        errDoc["status"] = "error";
        errDoc["error"] = "Неможливо виконати поворот (триває калібрування або контролер зайнятий)";
        String res;
        serializeJson(errDoc, res);
        server.send(409, "application/json", res);
    }
}

void TableWebServer::handleStop() {
    sendCorsHeaders();
    motionCtrl.stopAutomatic();
    motionCtrl.emergencyStop();
    tiltController.emergencyStop();

    JsonDocument doc;
    doc["status"] = "ok";
    doc["message"] = "Двигун зупинено";
    String res;
    serializeJson(doc, res);
    server.send(200, "application/json", res);
}

static void sendTiltStatus(WebServer& server, const TiltStatus& st) {
    JsonDocument doc;
    doc["status"] = "ok";
    doc["enabled"] = st.enabled;
    doc["moving"] = st.moving;
    doc["stopped"] = st.stopped;
    doc["height_mm"] = st.heightMm;
    doc["pitch_deg"] = st.pitchDeg;
    doc["roll_deg"] = st.rollDeg;
    doc["target_height_mm"] = st.targetHeightMm;
    doc["target_pitch_deg"] = st.targetPitchDeg;
    doc["target_roll_deg"] = st.targetRollDeg;
    JsonArray pulses = doc["pulse_us"].to<JsonArray>();
    for (uint8_t i = 0; i < 4; ++i) pulses.add(st.pulseUs[i]);
    if (strlen(st.errorMessage) > 0) doc["error"] = st.errorMessage;
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void TableWebServer::handleTiltStatus() {
    sendCorsHeaders();
    sendTiltStatus(server, tiltController.getStatus());
}

void TableWebServer::handleTiltSettings() {
    sendCorsHeaders();
    TiltConfig cfg = tiltController.getConfig();
    if (server.method() == HTTP_POST) {
        if (!server.hasArg("plain")) {
            server.send(400, "application/json", "{\"status\":\"error\",\"error\":\"Відсутнє тіло запиту\"}");
            return;
        }
        JsonDocument req;
        if (deserializeJson(req, server.arg("plain"))) {
            server.send(400, "application/json", "{\"status\":\"error\",\"error\":\"Некоректний формат JSON\"}");
            return;
        }
        cfg.enabled = req["enabled"] | cfg.enabled;
        cfg.minHeightMm = req["min_height_mm"] | cfg.minHeightMm;
        cfg.maxHeightMm = req["max_height_mm"] | cfg.maxHeightMm;
        cfg.maxPitchDeg = req["max_pitch_deg"] | cfg.maxPitchDeg;
        cfg.maxRollDeg = req["max_roll_deg"] | cfg.maxRollDeg;
        cfg.maxTiltSpeed = req["max_tilt_speed"] | cfg.maxTiltSpeed;
        cfg.pulsePerMm = req["pulse_per_mm"] | cfg.pulsePerMm;
        for (uint8_t i = 0; i < 4; ++i) {
            char key[24];
            snprintf(key, sizeof(key), "actuator_%u_pin", i);
            if (req[key].is<int>()) cfg.actuatorPin[i] = req[key].as<int>();
            snprintf(key, sizeof(key), "actuator_%u_channel", i);
            if (req[key].is<int>()) cfg.pwmChannel[i] = req[key].as<uint8_t>();
            snprintf(key, sizeof(key), "actuator_%u_min_us", i);
            if (req[key].is<int>()) cfg.minPulseUs[i] = req[key].as<uint16_t>();
            snprintf(key, sizeof(key), "actuator_%u_max_us", i);
            if (req[key].is<int>()) cfg.maxPulseUs[i] = req[key].as<uint16_t>();
            snprintf(key, sizeof(key), "actuator_%u_neutral_us", i);
            if (req[key].is<int>()) cfg.neutralPulseUs[i] = req[key].as<uint16_t>();
            snprintf(key, sizeof(key), "actuator_%u_offset_mm", i);
            if (req[key].is<float>()) cfg.actuatorOffsetMm[i] = req[key].as<float>();
            snprintf(key, sizeof(key), "actuator_%u_inverted", i);
            if (req[key].is<bool>()) cfg.actuatorInverted[i] = req[key].as<bool>();
        }
        bool rebootRequired = false;
        if (!tiltController.applyConfig(cfg, rebootRequired)) {
            server.send(400, "application/json", "{\"status\":\"error\",\"error\":\"Некоректні налаштування серво\"}");
            return;
        }
        JsonDocument responseDoc;
        responseDoc["status"] = "ok";
        responseDoc["reboot_required"] = rebootRequired;
        if (rebootRequired) wifiMgr.scheduleRestart(1500);
        String response;
        serializeJson(responseDoc, response);
        server.send(200, "application/json", response);
        return;
    }
    JsonDocument doc;
    doc["status"] = "ok";
    doc["enabled"] = cfg.enabled;
    doc["min_height_mm"] = cfg.minHeightMm;
    doc["max_height_mm"] = cfg.maxHeightMm;
    doc["max_pitch_deg"] = cfg.maxPitchDeg;
    doc["max_roll_deg"] = cfg.maxRollDeg;
    doc["max_tilt_speed"] = cfg.maxTiltSpeed;
    doc["pulse_per_mm"] = cfg.pulsePerMm;
    for (uint8_t i = 0; i < 4; ++i) {
        char key[24];
        snprintf(key, sizeof(key), "actuator_%u_pin", i); doc[key] = cfg.actuatorPin[i];
        snprintf(key, sizeof(key), "actuator_%u_channel", i); doc[key] = cfg.pwmChannel[i];
        snprintf(key, sizeof(key), "actuator_%u_min_us", i); doc[key] = cfg.minPulseUs[i];
        snprintf(key, sizeof(key), "actuator_%u_max_us", i); doc[key] = cfg.maxPulseUs[i];
        snprintf(key, sizeof(key), "actuator_%u_neutral_us", i); doc[key] = cfg.neutralPulseUs[i];
        snprintf(key, sizeof(key), "actuator_%u_offset_mm", i); doc[key] = cfg.actuatorOffsetMm[i];
        snprintf(key, sizeof(key), "actuator_%u_inverted", i); doc[key] = cfg.actuatorInverted[i];
    }
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void TableWebServer::handleTiltMove() {
    sendCorsHeaders();
    if (motionCtrl.getStatus().state == STATE_HOMING) {
        server.send(409, "application/json", "{\"status\":\"error\",\"error\":\"Нахил заборонений під час homing\"}");
        return;
    }
    JsonDocument req;
    if (!server.hasArg("plain") || deserializeJson(req, server.arg("plain"))) {
        server.send(400, "application/json", "{\"status\":\"error\",\"error\":\"Некоректне тіло запиту\"}");
        return;
    }
    const bool ok = tiltController.moveTo(req["height_mm"] | 0.0f,
                                          req["pitch_deg"] | 0.0f,
                                          req["roll_deg"] | 0.0f,
                                          req["speed"] | 0.0f);
    server.send(ok ? 200 : 409, "application/json",
                ok ? "{\"status\":\"ok\"}" : "{\"status\":\"error\",\"error\":\"Команда нахилу відхилена\"}");
}

void TableWebServer::handleTiltLevel() {
    sendCorsHeaders();
    const bool ok = tiltController.level();
    server.send(ok ? 200 : 409, "application/json",
                ok ? "{\"status\":\"ok\"}" : "{\"status\":\"error\",\"error\":\"Вирівнювання відхилено\"}");
}

void TableWebServer::handleTiltStop() {
    sendCorsHeaders();
    tiltController.emergencyStop();
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void TableWebServer::handleTiltHome() {
    sendCorsHeaders();
    if (motionCtrl.getStatus().state == STATE_HOMING) {
        server.send(409, "application/json", "{\"status\":\"error\",\"error\":\"Нахил заборонений під час homing\"}");
        return;
    }
    const bool ok = tiltController.home();
    server.send(ok ? 200 : 409, "application/json",
                ok ? "{\"status\":\"ok\"}" : "{\"status\":\"error\",\"error\":\"Повернення серво в нуль відхилено\"}");
}

void TableWebServer::handleZero() {
    sendCorsHeaders();
    motionCtrl.setZero();

    JsonDocument doc;
    doc["status"] = "ok";
    doc["message"] = "Поточну позицію встановлено як 0.0 градусів";
    String res;
    serializeJson(doc, res);
    server.send(200, "application/json", res);
}

void TableWebServer::handleGetSettings() {
    sendCorsHeaders();
    HardwareConfig cfg = motionCtrl.getConfig();

    JsonDocument doc;
    doc["status"] = "ok";
    doc["pin_step"] = cfg.pinStep;
    doc["pin_dir"] = cfg.pinDir;
    doc["pin_enable"] = cfg.pinEnable;
    doc["pin_endstop"] = cfg.pinEndstop;
    doc["pin_button_left"] = cfg.pinButtonLeft;
    doc["pin_button_right"] = cfg.pinButtonRight;
    doc["pin_button_stop"] = cfg.pinButtonStop;

    doc["invert_dir"] = cfg.invertDir;
    doc["endstop_inverted"] = cfg.endstopInverted;
    doc["endstop_debounce_ms"] = cfg.endstopDebounceMs;
    doc["button_left_inverted"] = cfg.buttonLeftInverted;
    doc["button_right_inverted"] = cfg.buttonRightInverted;
    doc["button_stop_inverted"] = cfg.buttonStopInverted;

    doc["motor_teeth"] = cfg.motorTeeth;
    doc["table_teeth"] = cfg.tableTeeth;
    doc["gear_ratio"] = cfg.getGearRatio();
    doc["steps_per_rev"] = cfg.stepsPerRev;
    doc["microsteps"] = cfg.microsteps;
    doc["steps_per_degree"] = cfg.getStepsPerDegree();

    doc["default_speed"] = cfg.defaultSpeed;
    doc["max_speed"] = cfg.maxSpeed;
    doc["acceleration"] = cfg.acceleration;
    doc["button_move_speed"] = cfg.buttonMoveSpeed;
    doc["button_move_angle"] = cfg.buttonMoveAngle;

    doc["homing_direction"] = cfg.homingDirection;
    doc["auto_home_on_boot"] = cfg.autoHomeOnBoot;
    ImuConfig imuCfg = imuSensor.getConfig();
    doc["i2c_sda_pin"] = imuCfg.sdaPin;
    doc["i2c_scl_pin"] = imuCfg.sclPin;
    doc["mpu6050_address"] = imuCfg.mpu6050Address;
    doc["barometer_address"] = imuCfg.barometerAddress;

    String res;
    serializeJson(doc, res);
    server.send(200, "application/json", res);
}

void TableWebServer::handleSaveSettings() {
    sendCorsHeaders();

    if (!server.hasArg("plain")) {
        JsonDocument errDoc;
        errDoc["status"] = "error";
        errDoc["error"] = "Відсутнє тіло запиту";
        String res;
        serializeJson(errDoc, res);
        server.send(400, "application/json", res);
        return;
    }

    JsonDocument reqDoc;
    DeserializationError err = deserializeJson(reqDoc, server.arg("plain"));
    if (err) {
        JsonDocument errDoc;
        errDoc["status"] = "error";
        errDoc["error"] = "Некоректний формат JSON";
        String res;
        serializeJson(errDoc, res);
        server.send(400, "application/json", res);
        return;
    }

    HardwareConfig cfg = motionCtrl.getConfig();

    if (reqDoc["pin_step"].is<int>()) cfg.pinStep = reqDoc["pin_step"].as<int>();
    if (reqDoc["pin_dir"].is<int>()) cfg.pinDir = reqDoc["pin_dir"].as<int>();
    if (reqDoc["pin_enable"].is<int>()) cfg.pinEnable = reqDoc["pin_enable"].as<int>();
    if (reqDoc["pin_endstop"].is<int>()) cfg.pinEndstop = reqDoc["pin_endstop"].as<int>();
    if (reqDoc["pin_button_left"].is<int>()) cfg.pinButtonLeft = reqDoc["pin_button_left"].as<int>();
    if (reqDoc["pin_button_right"].is<int>()) cfg.pinButtonRight = reqDoc["pin_button_right"].as<int>();
    if (reqDoc["pin_button_stop"].is<int>()) cfg.pinButtonStop = reqDoc["pin_button_stop"].as<int>();

    if (reqDoc["invert_dir"].is<bool>()) cfg.invertDir = reqDoc["invert_dir"].as<bool>();
    if (reqDoc["endstop_inverted"].is<bool>()) cfg.endstopInverted = reqDoc["endstop_inverted"].as<bool>();
    if (reqDoc["endstop_debounce_ms"].is<uint32_t>()) cfg.endstopDebounceMs = reqDoc["endstop_debounce_ms"].as<uint32_t>();
    if (reqDoc["button_left_inverted"].is<bool>()) cfg.buttonLeftInverted = reqDoc["button_left_inverted"].as<bool>();
    if (reqDoc["button_right_inverted"].is<bool>()) cfg.buttonRightInverted = reqDoc["button_right_inverted"].as<bool>();
    if (reqDoc["button_stop_inverted"].is<bool>()) cfg.buttonStopInverted = reqDoc["button_stop_inverted"].as<bool>();

    if (reqDoc["motor_teeth"].is<float>()) cfg.motorTeeth = reqDoc["motor_teeth"].as<float>();
    if (reqDoc["table_teeth"].is<float>()) cfg.tableTeeth = reqDoc["table_teeth"].as<float>();
    if (reqDoc["steps_per_rev"].is<float>()) cfg.stepsPerRev = reqDoc["steps_per_rev"].as<float>();
    if (reqDoc["microsteps"].is<float>()) cfg.microsteps = reqDoc["microsteps"].as<float>();

    if (reqDoc["default_speed"].is<float>()) cfg.defaultSpeed = reqDoc["default_speed"].as<float>();
    if (reqDoc["max_speed"].is<float>()) cfg.maxSpeed = reqDoc["max_speed"].as<float>();
    if (reqDoc["acceleration"].is<float>()) cfg.acceleration = reqDoc["acceleration"].as<float>();
    if (reqDoc["button_move_speed"].is<float>()) cfg.buttonMoveSpeed = reqDoc["button_move_speed"].as<float>();
    if (reqDoc["button_move_angle"].is<float>()) cfg.buttonMoveAngle = reqDoc["button_move_angle"].as<float>();

    if (cfg.buttonMoveSpeed <= 0.0f || cfg.buttonMoveAngle <= 0.0f) {
        JsonDocument errDoc;
        errDoc["status"] = "error";
        errDoc["error"] = "Швидкість та кут апаратних кнопок повинні бути більшими за 0";
        String res;
        serializeJson(errDoc, res);
        server.send(400, "application/json", res);
        return;
    }

    if (reqDoc["homing_direction"].is<int>()) cfg.homingDirection = reqDoc["homing_direction"].as<int>();
    if (reqDoc["auto_home_on_boot"].is<bool>()) cfg.autoHomeOnBoot = reqDoc["auto_home_on_boot"].as<bool>();

    ImuConfig imuCfg = imuSensor.getConfig();
    if (reqDoc["i2c_sda_pin"].is<int>()) imuCfg.sdaPin = reqDoc["i2c_sda_pin"].as<int>();
    if (reqDoc["i2c_scl_pin"].is<int>()) imuCfg.sclPin = reqDoc["i2c_scl_pin"].as<int>();
    if (reqDoc["mpu6050_address"].is<int>()) imuCfg.mpu6050Address = reqDoc["mpu6050_address"].as<int>();
    if (reqDoc["barometer_address"].is<int>()) imuCfg.barometerAddress = reqDoc["barometer_address"].as<int>();

    if (imuCfg.sdaPin < 0 || imuCfg.sclPin < 0 ||
        imuCfg.mpu6050Address < 0x03 || imuCfg.mpu6050Address > 0x77 ||
        imuCfg.barometerAddress < 0x03 || imuCfg.barometerAddress > 0x77) {
        JsonDocument errDoc;
        errDoc["status"] = "error";
        errDoc["error"] = "Некоректні I2C-піни або адреси датчиків";
        String res;
        serializeJson(errDoc, res);
        server.send(400, "application/json", res);
        return;
    }

    bool rebootRequired = false;
    bool imuRebootRequired = false;
    if (!imuSensor.applyConfig(imuCfg, imuRebootRequired)) {
        JsonDocument errDoc;
        errDoc["status"] = "error";
        errDoc["error"] = "Не вдалося зберегти налаштування IMU";
        String res;
        serializeJson(errDoc, res);
        server.send(500, "application/json", res);
        return;
    }
    rebootRequired = imuRebootRequired;
    bool motionRebootRequired = false;
    if (motionCtrl.applyConfig(cfg, motionRebootRequired)) {
        rebootRequired = rebootRequired || motionRebootRequired;
        if (rebootRequired) {
            wifiMgr.scheduleRestart(1500);
        }

        JsonDocument respDoc;
        respDoc["status"] = "ok";
        respDoc["message"] = rebootRequired ? "Апаратні або I2C-параметри змінено. Контролер перезавантажується..."
                                            : "Налаштування столу успішно застосовано";
        respDoc["reboot_required"] = rebootRequired;
        respDoc["gear_ratio"] = cfg.getGearRatio();
        respDoc["steps_per_degree"] = cfg.getStepsPerDegree();

        String res;
        serializeJson(respDoc, res);
        server.send(200, "application/json", res);
    } else {
        JsonDocument errDoc;
        errDoc["status"] = "error";
        errDoc["error"] = "Не вдалося застосувати налаштування";
        String res;
        serializeJson(errDoc, res);
        server.send(500, "application/json", res);
    }
}

void TableWebServer::handleWiFiConfig() {
    sendCorsHeaders();
    WiFiSettings cfg = wifiMgr.getSettings();

    JsonDocument doc;
    doc["status"] = "ok";
    doc["mode"] = wifiMgr.getModeStr();
    doc["ip"] = wifiMgr.getIPAddress();
    doc["connected"] = wifiMgr.isConnectedSTA();
    doc["current_ssid"] = wifiMgr.getCurrentSSID();
    doc["sta_ssid"] = cfg.staSSID;
    doc["ap_ssid"] = cfg.apSSID;

    String res;
    serializeJson(doc, res);
    server.send(200, "application/json", res);
}

void TableWebServer::handleWiFiScan() {
    sendCorsHeaders();
    JsonDocument doc;
    doc["status"] = "ok";
    JsonArray networks = doc["networks"].to<JsonArray>();
    wifiMgr.scanNetworks(networks);

    String res;
    serializeJson(doc, res);
    server.send(200, "application/json", res);
}

void TableWebServer::handleWiFiSave() {
    sendCorsHeaders();

    if (!server.hasArg("plain")) {
        JsonDocument errDoc;
        errDoc["status"] = "error";
        errDoc["error"] = "Відсутнє тіло запиту";
        String res;
        serializeJson(errDoc, res);
        server.send(400, "application/json", res);
        return;
    }

    JsonDocument reqDoc;
    DeserializationError err = deserializeJson(reqDoc, server.arg("plain"));
    if (err) {
        JsonDocument errDoc;
        errDoc["status"] = "error";
        errDoc["error"] = "Некоректний формат JSON";
        String res;
        serializeJson(errDoc, res);
        server.send(400, "application/json", res);
        return;
    }

    String staSSID = reqDoc["sta_ssid"] | "";
    String staPass = reqDoc["sta_pass"] | "";
    String apSSID = reqDoc["ap_ssid"] | "";
    String apPass = reqDoc["ap_pass"] | "";

    // Пароль власної точки доступу повинен бути >= 8 символів або порожнім
    if (apPass.length() > 0 && apPass.length() < 8) {
        JsonDocument errDoc;
        errDoc["status"] = "error";
        errDoc["error"] = "Пароль точки доступу має містити щонайменше 8 символів або бути порожнім";
        String res;
        serializeJson(errDoc, res);
        server.send(400, "application/json", res);
        return;
    }

    if (wifiMgr.saveSettings(staSSID, staPass, apSSID, apPass)) {
        wifiMgr.scheduleRestart(1500);

        JsonDocument respDoc;
        respDoc["status"] = "ok";
        respDoc["message"] = "Налаштування Wi-Fi збережено. Контролер перезавантажується...";
        String res;
        serializeJson(respDoc, res);
        server.send(200, "application/json", res);
    } else {
        JsonDocument errDoc;
        errDoc["status"] = "error";
        errDoc["error"] = "Не вдалося зберегти налаштування у пам'ять NVS";
        String res;
        serializeJson(errDoc, res);
        server.send(500, "application/json", res);
    }
}

void TableWebServer::handleWiFiReset() {
    sendCorsHeaders();
    if (wifiMgr.resetSettings()) {
        wifiMgr.scheduleRestart(1500);

        JsonDocument doc;
        doc["status"] = "ok";
        doc["message"] = "Налаштування мережі скинуто до вихідних. Перезавантаження...";
        String res;
        serializeJson(doc, res);
        server.send(200, "application/json", res);
    } else {
        JsonDocument doc;
        doc["status"] = "error";
        doc["error"] = "Не вдалося скинути налаштування";
        String res;
        serializeJson(doc, res);
        server.send(500, "application/json", res);
    }
}

void TableWebServer::handleNotFound() {
    sendCorsHeaders();
    JsonDocument doc;
    doc["status"] = "error";
    doc["error"] = "Ендпоінт не знайдено";
    String res;
    serializeJson(doc, res);
    server.send(404, "application/json", res);
}

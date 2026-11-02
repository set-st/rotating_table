#include "railway_web.h"
#include "railway_ui.h"
#include "railway_controller.h"
#include "wifi_manager.h"
#include "config.h"
#include "ota_updater.h"

RailwayWebServer railwayWeb;

RailwayWebServer::RailwayWebServer() : server(WEB_SERVER_PORT) {}

void RailwayWebServer::sendCorsHeaders() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void RailwayWebServer::sendJson(int code, const JsonDocument& doc) {
    String body;
    serializeJson(doc, body);
    sendCorsHeaders();
    server.send(code, "application/json", body);
}

void RailwayWebServer::sendError(int code, const char* message) {
    JsonDocument doc;
    doc["status"] = "error";
    doc["error"] = message;
    sendJson(code, doc);
}

bool RailwayWebServer::readBody(JsonDocument& doc) {
    if (!server.hasArg("plain")) {
        sendError(400, "Відсутнє тіло запиту");
        return false;
    }
    if (deserializeJson(doc, server.arg("plain"))) {
        sendError(400, "Некоректний формат JSON");
        return false;
    }
    return true;
}

void RailwayWebServer::route(const char* path, HTTPMethod method,
                             void (RailwayWebServer::*handler)()) {
    server.on(path, method, [this, handler]() { (this->*handler)(); });
}

void RailwayWebServer::begin() {
    route("/", HTTP_GET, &RailwayWebServer::handleRoot);

    struct Item { const char* path; HTTPMethod method; void (RailwayWebServer::*handler)(); };
    const Item items[] = {
        {"/api/status", HTTP_GET, &RailwayWebServer::handleStatus},
        {"/api/home", HTTP_POST, &RailwayWebServer::handleHome},
        {"/api/move", HTTP_POST, &RailwayWebServer::handleMove},
        {"/api/jog", HTTP_POST, &RailwayWebServer::handleJog},
        {"/api/stop", HTTP_POST, &RailwayWebServer::handleStop},
        {"/api/zero", HTTP_POST, &RailwayWebServer::handleZero},
        {"/api/enable", HTTP_POST, &RailwayWebServer::handleEnable},
        {"/api/settings", HTTP_GET, &RailwayWebServer::handleGetSettings},
        {"/api/settings", HTTP_POST, &RailwayWebServer::handleSaveSettings},
        {"/api/wifi/config", HTTP_GET, &RailwayWebServer::handleWiFiConfig},
        {"/api/wifi/scan", HTTP_GET, &RailwayWebServer::handleWiFiScan},
        {"/api/wifi/save", HTTP_POST, &RailwayWebServer::handleWiFiSave},
        {"/api/wifi/reset", HTTP_POST, &RailwayWebServer::handleWiFiReset},
        {"/api/ota/latest", HTTP_GET, &RailwayWebServer::handleOtaLatest},
        {"/api/ota/update", HTTP_POST, &RailwayWebServer::handleOtaUpdate},
    };
    for (const Item& it : items) {
        route(it.path, it.method, it.handler);
        server.on(it.path, HTTP_OPTIONS, [this]() {
            sendCorsHeaders();
            server.send(204);
        });
    }
    server.onNotFound([this]() { handleNotFound(); });
    server.begin();
    Serial.printf("[Web] HTTP-сервер RailWay запущено на порту %d\n", WEB_SERVER_PORT);
}

void RailwayWebServer::handleClient() {
    server.handleClient();
}

void RailwayWebServer::handleRoot() {
    sendCorsHeaders();
    server.send_P(200, "text/html", PAGE_RAILWAY);
}

void RailwayWebServer::handleStatus() {
    const RailwayStatus st = railway.getStatus();
    const RailwaySettings cfg = railway.getSettings();
    JsonDocument doc;
    doc["status"] = "ok";
    doc["firmware_version"] = FIRMWARE_VERSION;
    doc["state"] = st.stateStr;
    doc["x_mm"] = st.xMm;
    doc["z_mm"] = st.zMm;
    doc["target_x_mm"] = st.targetXMm;
    doc["target_z_mm"] = st.targetZMm;
    doc["speed_mm_s"] = st.speedMmS;
    doc["is_homed"] = st.homedX && st.homedZ;
    doc["ignore_homing"] = cfg.ignoreHoming;
    doc["homed_x"] = st.homedX;
    doc["homed_z"] = st.homedZ;
    doc["limit_x"] = st.limitX;
    doc["limit_x_far"] = st.limitXFar;
    doc["limit_z"] = st.limitZ;
    doc["drivers_enabled"] = st.enabled;
    doc["x_length_mm"] = cfg.xLengthMm;
    doc["x_length_measured"] = st.xLengthMeasured;
    doc["z_height_mm"] = cfg.zHeightMm;
    doc["z_endstop_top"] = cfg.zEndstopTop;
    doc["max_speed_mm_s"] = cfg.maxSpeedMmS;
    if (st.errorMessage[0]) doc["error"] = st.errorMessage;
    sendJson(200, doc);
}

void RailwayWebServer::handleHome() {
    String error;
    if (!railway.startHoming(error)) {
        sendError(409, error.c_str());
        return;
    }
    JsonDocument doc;
    doc["status"] = "ok";
    doc["message"] = "Homing запущено: X з вимірюванням довжини, потім Z";
    sendJson(200, doc);
}

void RailwayWebServer::handleMove() {
    JsonDocument req;
    if (!readBody(req)) return;
    if (!req["x"].is<float>() || !req["z"].is<float>()) {
        sendError(400, "Потрібні числові поля x та z (мм)");
        return;
    }
    String error;
    if (!railway.moveTo(req["x"].as<float>(), req["z"].as<float>(),
                        req["speed"] | 0.0f, error)) {
        sendError(409, error.c_str());
        return;
    }
    JsonDocument doc;
    doc["status"] = "ok";
    doc["target_x_mm"] = req["x"].as<float>();
    doc["target_z_mm"] = req["z"].as<float>();
    sendJson(200, doc);
}

void RailwayWebServer::handleJog() {
    JsonDocument req;
    if (!readBody(req)) return;
    const char* axis = req["axis"] | "";
    if (!axis[0] || !req["distance"].is<float>()) {
        sendError(400, "Потрібні поля axis (\"x\"/\"z\") та distance (мм)");
        return;
    }
    String error;
    if (!railway.jog(axis[0], req["distance"].as<float>(), req["speed"] | 0.0f, error)) {
        sendError(409, error.c_str());
        return;
    }
    JsonDocument doc;
    doc["status"] = "ok";
    sendJson(200, doc);
}

void RailwayWebServer::handleStop() {
    railway.stop();
    JsonDocument doc;
    doc["status"] = "ok";
    doc["message"] = "Рух зупинено";
    sendJson(200, doc);
}

void RailwayWebServer::handleZero() {
    String error;
    if (!railway.zeroHere(error)) {
        sendError(409, error.c_str());
        return;
    }
    JsonDocument doc;
    doc["status"] = "ok";
    doc["message"] = "Поточну позицію прийнято за X=0, Z=0";
    sendJson(200, doc);
}

void RailwayWebServer::handleEnable() {
    JsonDocument req;
    if (!readBody(req)) return;
    if (!req["enabled"].is<bool>()) {
        sendError(400, "Потрібне булеве поле enabled");
        return;
    }
    railway.setEnabled(req["enabled"].as<bool>());
    JsonDocument doc;
    doc["status"] = "ok";
    sendJson(200, doc);
}

void RailwayWebServer::handleGetSettings() {
    const RailwaySettings s = railway.getSettings();
    JsonDocument doc;
    doc["status"] = "ok";
    doc["steps_per_mm_x"] = s.stepsPerMmX;
    doc["steps_per_mm_z"] = s.stepsPerMmZ;
    doc["x_length_mm"] = s.xLengthMm;
    doc["x_length_measured"] = s.xLengthMeasured;
    doc["z_height_mm"] = s.zHeightMm;
    doc["x_dir_invert"] = s.xDirInvert;
    doc["z_dir_invert"] = s.zDirInvert;
    doc["limit_active_low"] = s.limitActiveLow;
    doc["z_endstop_top"] = s.zEndstopTop;
    doc["auto_home_on_boot"] = s.autoHomeOnBoot;
    doc["ignore_homing"] = s.ignoreHoming;
    doc["max_speed_mm_s"] = s.maxSpeedMmS;
    doc["accel_mm_s2"] = s.accelMmS2;
    doc["homing_speed_mm_s"] = s.homingSpeedMmS;
    doc["homing_backoff_mm"] = s.homingBackoffMm;
    sendJson(200, doc);
}

void RailwayWebServer::handleSaveSettings() {
    JsonDocument req;
    if (!readBody(req)) return;
    RailwaySettings s = railway.getSettings();
    s.stepsPerMmX = req["steps_per_mm_x"] | s.stepsPerMmX;
    s.stepsPerMmZ = req["steps_per_mm_z"] | s.stepsPerMmZ;
    s.xLengthMm = req["x_length_mm"] | s.xLengthMm;
    s.zHeightMm = req["z_height_mm"] | s.zHeightMm;
    s.xDirInvert = req["x_dir_invert"] | s.xDirInvert;
    s.zDirInvert = req["z_dir_invert"] | s.zDirInvert;
    s.limitActiveLow = req["limit_active_low"] | s.limitActiveLow;
    s.zEndstopTop = req["z_endstop_top"] | s.zEndstopTop;
    s.autoHomeOnBoot = req["auto_home_on_boot"] | s.autoHomeOnBoot;
    s.ignoreHoming = req["ignore_homing"] | s.ignoreHoming;
    s.maxSpeedMmS = req["max_speed_mm_s"] | s.maxSpeedMmS;
    s.accelMmS2 = req["accel_mm_s2"] | s.accelMmS2;
    s.homingSpeedMmS = req["homing_speed_mm_s"] | s.homingSpeedMmS;
    s.homingBackoffMm = req["homing_backoff_mm"] | s.homingBackoffMm;

    String error;
    if (!railway.saveSettings(s, error)) {
        sendError(400, error.c_str());
        return;
    }
    JsonDocument doc;
    doc["status"] = "ok";
    doc["message"] = "Налаштування збережено";
    sendJson(200, doc);
}

void RailwayWebServer::handleWiFiConfig() {
    const WiFiSettings cfg = wifiMgr.getSettings();
    JsonDocument doc;
    doc["status"] = "ok";
    doc["mode"] = wifiMgr.getModeStr();
    doc["ip"] = wifiMgr.getIPAddress();
    doc["connected"] = wifiMgr.isConnectedSTA();
    doc["current_ssid"] = wifiMgr.getCurrentSSID();
    doc["sta_ssid"] = cfg.staSSID;
    doc["ap_ssid"] = cfg.apSSID;
    sendJson(200, doc);
}

void RailwayWebServer::handleWiFiScan() {
    JsonDocument doc;
    doc["status"] = "ok";
    JsonArray networks = doc["networks"].to<JsonArray>();
    wifiMgr.scanNetworks(networks);
    sendJson(200, doc);
}

void RailwayWebServer::handleWiFiSave() {
    JsonDocument req;
    if (!readBody(req)) return;
    const String apPass = req["ap_pass"] | "";
    if (apPass.length() > 0 && apPass.length() < 8) {
        sendError(400, "Пароль точки доступу має містити щонайменше 8 символів або бути порожнім");
        return;
    }
    if (!wifiMgr.saveSettings(req["sta_ssid"] | "", req["sta_pass"] | "",
                              req["ap_ssid"] | "", apPass)) {
        sendError(500, "Не вдалося зберегти налаштування у пам'ять NVS");
        return;
    }
    wifiMgr.scheduleRestart(1500);
    JsonDocument doc;
    doc["status"] = "ok";
    doc["message"] = "Налаштування Wi-Fi збережено. Контролер перезавантажується...";
    sendJson(200, doc);
}

void RailwayWebServer::handleWiFiReset() {
    if (!wifiMgr.resetSettings()) {
        sendError(500, "Не вдалося скинути налаштування");
        return;
    }
    wifiMgr.scheduleRestart(1500);
    JsonDocument doc;
    doc["status"] = "ok";
    doc["message"] = "Налаштування мережі скинуто до вихідних. Перезавантаження...";
    sendJson(200, doc);
}

void RailwayWebServer::handleOtaLatest() {
    const OtaReleaseInfo info = otaUpdater.getLatestRelease("railway.bin");
    JsonDocument doc;
    doc["status"] = info.available ? "ok" : "error";
    doc["current_version"] = FIRMWARE_VERSION;
    if (info.available) {
        doc["tag"] = info.tagName;
        doc["asset"] = info.assetName;
        doc["size"] = info.assetSize;
    } else {
        doc["error"] = info.error;
    }
    sendJson(info.available ? 200 : 502, doc);
}

void RailwayWebServer::handleOtaUpdate() {
    if (!wifiMgr.isConnectedSTA()) {
        sendError(503, "OTA доступне лише при підключенні до роутера з Інтернетом; SoftAP не має виходу в Інтернет");
        return;
    }
    // Під час запису прошивки каретка не повинна рухатися
    railway.stop();
    String error;
    if (!otaUpdater.installLatestRelease(error, "railway.bin")) {
        sendError(502, error.c_str());
        return;
    }
    JsonDocument doc;
    doc["status"] = "ok";
    doc["message"] = "Оновлення записано. Контролер перезавантажується...";
    sendJson(200, doc);
    wifiMgr.scheduleRestart(1500);
}

void RailwayWebServer::handleNotFound() {
    sendError(404, "Ендпоінт не знайдено");
}

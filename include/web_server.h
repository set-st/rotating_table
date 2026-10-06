#pragma once
#include <Arduino.h>
#include <WebServer.h>

class TableWebServer {
public:
    TableWebServer();
    void begin();
    void handleClient();

private:
    WebServer server;

    void setupRoutes();
    void sendCorsHeaders();

    // Обробники маршрутів HTTP API
    void handleRoot();
    void handleStatus();
    void handleHome();
    void handleMove();
    void handleStop();
    void handleZero();
    // Налаштування кінцевика та параметрів столу
    void handleGetSettings();
    void handleSaveSettings();
    void handleImuScan();
    void handleImuCalibrate();
    void handleImuZero();
    void handleAutomaticStart();
    void handleAutomaticStop();
    void handleOtaLatest();
    void handleOtaUpdate();
    void handleTiltOtaLatest();
    void handleTiltOtaUpdate();

    // Налаштування Wi-Fi
    void handleWiFiConfig();
    void handleWiFiScan();
    void handleWiFiSave();
    void handleWiFiReset();

    // Керування та налаштування платформи нахилу (через Bluetooth)
    void handleTiltStatus();
    void handleTiltMove();
    void handleTiltHome();
    void handleTiltStop();
    void handleTiltZero();
    void handleTiltGyroZero();
    void handleTiltGyroCalibrate();
    void handleTiltHold();
    void handleTiltGetSettings();
    void handleTiltSaveSettings();
    void handleTiltBtScan();
    void handleTiltBtConnect();

    void handleNotFound();
    void handleOptions();
};

extern TableWebServer webServer;

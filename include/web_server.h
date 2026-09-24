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

    // Налаштування Wi-Fi
    void handleWiFiConfig();
    void handleWiFiScan();
    void handleWiFiSave();
    void handleWiFiReset();

    void handleNotFound();
    void handleOptions();
};

extern TableWebServer webServer;

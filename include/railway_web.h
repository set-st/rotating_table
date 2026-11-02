#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>

class RailwayWebServer {
public:
    RailwayWebServer();
    void begin();
    void handleClient();

private:
    WebServer server;

    void route(const char* path, HTTPMethod method, void (RailwayWebServer::*handler)());
    void sendJson(int code, const JsonDocument& doc);
    void sendError(int code, const char* message);
    bool readBody(JsonDocument& doc);
    void sendCorsHeaders();

    void handleRoot();
    void handleStatus();
    void handleHome();
    void handleMove();
    void handleJog();
    void handleStop();
    void handleZero();
    void handleEnable();
    void handleGetSettings();
    void handleSaveSettings();
    void handleWiFiConfig();
    void handleWiFiScan();
    void handleWiFiSave();
    void handleWiFiReset();
    void handleOtaLatest();
    void handleOtaUpdate();
    void handleNotFound();
};

extern RailwayWebServer railwayWeb;

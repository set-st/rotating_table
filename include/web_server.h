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

    // Route handlers
    void handleRoot();
    void handleStatus();
    void handleHome();
    void handleMove();
    void handleStop();
    void handleZero();
    void handleNotFound();
    void handleOptions();
};

extern TableWebServer webServer;

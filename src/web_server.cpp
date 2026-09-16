#include "web_server.h"
#include "web_ui.h"
#include "motion_controller.h"
#include "config.h"
#include <ArduinoJson.h>

TableWebServer webServer;

TableWebServer::TableWebServer() : server(WEB_SERVER_PORT) {}

void TableWebServer::begin() {
    setupRoutes();
    server.begin();
    Serial.printf("[Web] HTTP Server started on port %d\n", WEB_SERVER_PORT);
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
    server.send(204); // No Content
}

void TableWebServer::setupRoutes() {
    // Root UI
    server.on("/", HTTP_GET, [this]() { handleRoot(); });

    // CORS preflight requests
    server.on("/api/status", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/home", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/move", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/stop", HTTP_OPTIONS, [this]() { handleOptions(); });
    server.on("/api/zero", HTTP_OPTIONS, [this]() { handleOptions(); });

    // REST API endpoints
    server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    server.on("/api/home", HTTP_POST, [this]() { handleHome(); });
    server.on("/api/move", HTTP_POST, [this]() { handleMove(); });
    server.on("/api/stop", HTTP_POST, [this]() { handleStop(); });
    server.on("/api/zero", HTTP_POST, [this]() { handleZero(); });

    // 404 Not Found
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
    if (strlen(st.errorMessage) > 0) {
        doc["error"] = st.errorMessage;
    }

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void TableWebServer::handleHome() {
    sendCorsHeaders();

    if (motionCtrl.startHoming()) {
        JsonDocument doc;
        doc["status"] = "ok";
        doc["message"] = "Homing initiated";
        String res;
        serializeJson(doc, res);
        server.send(200, "application/json", res);
    } else {
        JsonDocument doc;
        doc["status"] = "error";
        doc["error"] = "Cannot start homing (controller busy)";
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
        errDoc["error"] = "Missing request body";
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
        errDoc["error"] = "Invalid JSON format";
        String res;
        serializeJson(errDoc, res);
        server.send(400, "application/json", res);
        return;
    }

    if (!reqDoc.containsKey("angle")) {
        JsonDocument errDoc;
        errDoc["status"] = "error";
        errDoc["error"] = "Field 'angle' (float) is required";
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
        respDoc["message"] = "Move command accepted";
        respDoc["target_angle"] = angle;
        respDoc["speed"] = speed;
        respDoc["relative"] = relative;
        String res;
        serializeJson(respDoc, res);
        server.send(200, "application/json", res);
    } else {
        JsonDocument errDoc;
        errDoc["status"] = "error";
        errDoc["error"] = "Cannot move (homing in progress or controller busy)";
        String res;
        serializeJson(errDoc, res);
        server.send(409, "application/json", res);
    }
}

void TableWebServer::handleStop() {
    sendCorsHeaders();
    motionCtrl.emergencyStop();

    JsonDocument doc;
    doc["status"] = "ok";
    doc["message"] = "Motor stopped";
    String res;
    serializeJson(doc, res);
    server.send(200, "application/json", res);
}

void TableWebServer::handleZero() {
    sendCorsHeaders();
    motionCtrl.setZero();

    JsonDocument doc;
    doc["status"] = "ok";
    doc["message"] = "Current position calibrated to 0.0 degrees";
    String res;
    serializeJson(doc, res);
    server.send(200, "application/json", res);
}

void TableWebServer::handleNotFound() {
    sendCorsHeaders();
    JsonDocument doc;
    doc["status"] = "error";
    doc["error"] = "Endpoint not found";
    String res;
    serializeJson(doc, res);
    server.send(404, "application/json", res);
}

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include "config.h"
#include "motion_controller.h"
#include "web_server.h"

void setupWiFi() {
    bool tryConnectSTA = (strcmp(WIFI_SSID, "YOUR_WIFI_SSID") != 0 && strlen(WIFI_SSID) > 0);

    if (tryConnectSTA) {
        Serial.printf("[WiFi] Connecting to %s", WIFI_SSID);
        WiFi.mode(WIFI_STA);
        WiFi.setHostname(HOSTNAME);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        uint32_t startAttempt = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startAttempt) < WIFI_CONNECT_TIMEOUT_MS) {
            delay(400);
            Serial.print(".");
        }
        Serial.println();
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[WiFi] Connected successfully!");
        Serial.printf("[WiFi] IP Address: http://%s\n", WiFi.localIP().toString().c_str());
    } else {
        if (tryConnectSTA) {
            Serial.println("[WiFi] Failed to connect to router. Falling back to AP mode...");
        } else {
            Serial.println("[WiFi] Starting Access Point mode...");
        }

        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_SSID, strlen(AP_PASSWORD) >= 8 ? AP_PASSWORD : nullptr);

        Serial.println("[WiFi] Access Point started:");
        Serial.printf("       SSID: %s\n", AP_SSID);
        Serial.printf("       Pass: %s\n", strlen(AP_PASSWORD) >= 8 ? AP_PASSWORD : "(open network)");
        Serial.printf("       URL:  http://%s\n", WiFi.softAPIP().toString().c_str());
    }

    // Initialize mDNS (http://rotating-table.local)
    if (MDNS.begin(HOSTNAME)) {
        MDNS.addService("http", "tcp", WEB_SERVER_PORT);
        Serial.printf("[mDNS] Responder started: http://%s.local\n", HOSTNAME);
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n==================================================");
    Serial.println("   ESP32 Rotating Table Controller Firmware");
    Serial.println("==================================================");

    // 1. Initialize motion controller and FreeRTOS stepping task
    if (!motionCtrl.begin()) {
        Serial.println("[Error] Failed to initialize Motion Controller!");
    }

    // 2. Setup Wi-Fi and mDNS
    setupWiFi();

    // 3. Start Web Server
    webServer.begin();

    // 4. Automatic homing at startup if enabled
    if (AUTO_HOME_ON_BOOT) {
        Serial.println("[Boot] Triggering automatic initial homing...");
        motionCtrl.startHoming();
    }

    Serial.println("[System] Initialization complete. Ready for commands.");
}

void loop() {
    // Handle incoming HTTP requests
    webServer.handleClient();
    delay(2);
}

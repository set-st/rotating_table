#include "wifi_manager.h"
#include "config.h"
#include <ESPmDNS.h>

static const char* NVS_NAMESPACE = "table_wifi";
TableWiFiManager wifiMgr;

TableWiFiManager::TableWiFiManager()
    : restartAtMillis(0), isRestartScheduled(false) {}

void TableWiFiManager::loadFromPreferences() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true); // Лише для читання

    // Параметри STA (клієнта роутера)
    if (prefs.isKey("sta_ssid")) {
        currentSettings.staSSID = prefs.getString("sta_ssid", "");
        currentSettings.staPassword = prefs.getString("sta_pass", "");
    } else {
        // Значення за замовчуванням із config.h
        if (strcmp(WIFI_SSID, "YOUR_WIFI_SSID") != 0) {
            currentSettings.staSSID = WIFI_SSID;
            currentSettings.staPassword = WIFI_PASSWORD;
        } else {
            currentSettings.staSSID = "";
            currentSettings.staPassword = "";
        }
    }

    // Параметри власної точки доступу (SoftAP)
    if (prefs.isKey("ap_ssid")) {
        currentSettings.apSSID = prefs.getString("ap_ssid", AP_SSID);
        currentSettings.apPassword = prefs.getString("ap_pass", AP_PASSWORD);
    } else {
        currentSettings.apSSID = AP_SSID;
        currentSettings.apPassword = AP_PASSWORD;
    }

    prefs.end();
}

void TableWiFiManager::begin() {
    loadFromPreferences();
}

void TableWiFiManager::setupWiFi() {
    bool hasSTA = currentSettings.staSSID.length() > 0;

    if (hasSTA) {
        Serial.printf("[WiFi] Спроба підключення до роутера '%s'...\n", currentSettings.staSSID.c_str());
        WiFi.mode(WIFI_STA);
        WiFi.setHostname(HOSTNAME);
        WiFi.begin(currentSettings.staSSID.c_str(), currentSettings.staPassword.c_str());

        uint32_t startAttempt = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startAttempt) < WIFI_CONNECT_TIMEOUT_MS) {
            delay(300);
            Serial.print(".");
        }
        Serial.println();
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("[WiFi] Успішно підключено до мережі Wi-Fi!");
        Serial.printf("[WiFi] IP-адреса пристрою: http://%s\n", WiFi.localIP().toString().c_str());
    } else {
        if (hasSTA) {
            Serial.println("[WiFi] Не вдалося підключитися до роутера. Перехід у режим власної точки доступу...");
        } else {
            Serial.println("[WiFi] Мережу Wi-Fi не налаштовано. Запуск власної точки доступу...");
        }

        WiFi.mode(WIFI_AP);
        const char* apPass = (currentSettings.apPassword.length() >= 8) ? currentSettings.apPassword.c_str() : nullptr;
        WiFi.softAP(currentSettings.apSSID.c_str(), apPass);

        Serial.println("[WiFi] Точку доступу запущено:");
        Serial.printf("       SSID: %s\n", currentSettings.apSSID.c_str());
        Serial.printf("       Пароль: %s\n", apPass ? apPass : "(відкрита мережа)");
        Serial.printf("       URL:  http://%s\n", WiFi.softAPIP().toString().c_str());
    }

    // Запуск mDNS-відповідача (http://rotating-table.local)
    if (MDNS.begin(HOSTNAME)) {
        MDNS.addService("http", "tcp", WEB_SERVER_PORT);
        Serial.printf("[mDNS] Служба запущена: http://%s.local\n", HOSTNAME);
    }
}

WiFiSettings TableWiFiManager::getSettings() const {
    return currentSettings;
}

bool TableWiFiManager::saveSettings(const String& staSSID, const String& staPassword,
                                    const String& apSSID, const String& apPassword) {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) { // Читання та запис
        Serial.println("[WiFi] Помилка відкриття NVS для запису!");
        return false;
    }

    prefs.putString("sta_ssid", staSSID);
    prefs.putString("sta_pass", staPassword);

    if (apSSID.length() > 0) {
        prefs.putString("ap_ssid", apSSID);
    }
    prefs.putString("ap_pass", apPassword);
    prefs.end();

    currentSettings.staSSID = staSSID;
    currentSettings.staPassword = staPassword;
    if (apSSID.length() > 0) {
        currentSettings.apSSID = apSSID;
    }
    currentSettings.apPassword = apPassword;

    Serial.println("[WiFi] Нові налаштування збережено в NVS пам'ять.");
    return true;
}

bool TableWiFiManager::resetSettings() {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        return false;
    }
    prefs.clear();
    prefs.end();

    loadFromPreferences();
    Serial.println("[WiFi] Налаштування мережі очищено. Відновлено значення з config.h.");
    return true;
}

void TableWiFiManager::scheduleRestart(uint32_t delayMs) {
    restartAtMillis = millis() + delayMs;
    isRestartScheduled = true;
    Serial.printf("[System] Перезавантаження заплановано через %u мс\n", delayMs);
}

void TableWiFiManager::handle() {
    if (isRestartScheduled && millis() >= restartAtMillis) {
        Serial.println("[System] Виконується перезавантаження контролера...");
        delay(100);
        ESP.restart();
    }
}

bool TableWiFiManager::isConnectedSTA() const {
    return WiFi.status() == WL_CONNECTED;
}

String TableWiFiManager::getModeStr() const {
    if (WiFi.status() == WL_CONNECTED) {
        return "STA";
    }
    return "AP";
}

String TableWiFiManager::getIPAddress() const {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    }
    return WiFi.softAPIP().toString();
}

String TableWiFiManager::getCurrentSSID() const {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.SSID();
    }
    return currentSettings.apSSID;
}

void TableWiFiManager::scanNetworks(JsonArray& outArray) {
    wifi_mode_t curMode = WiFi.getMode();
    if (curMode == WIFI_AP) {
        WiFi.mode(WIFI_AP_STA);
    }

    int16_t n = WiFi.scanNetworks(false, false);
    if (n > 0) {
        for (int i = 0; i < n; ++i) {
            JsonObject net = outArray.add<JsonObject>();
            net["ssid"] = WiFi.SSID(i);
            net["rssi"] = WiFi.RSSI(i);
            net["secure"] = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
        }
    }
    WiFi.scanDelete();

    if (curMode == WIFI_AP) {
        WiFi.mode(WIFI_AP);
    }
}

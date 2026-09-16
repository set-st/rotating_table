#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// Структура налаштувань мережі Wi-Fi
struct WiFiSettings {
    String staSSID;     // SSID мережі роутера
    String staPassword; // Пароль до мережі роутера
    String apSSID;      // SSID власної точки доступу столу
    String apPassword;  // Пароль власної точки доступу
};

class TableWiFiManager {
public:
    TableWiFiManager();

    // Завантаження збережених налаштувань із пам'яті NVS
    void begin();

    // Запуск підключення до Wi-Fi або старт власної точки доступу
    void setupWiFi();

    // Обробник відкладеного перезавантаження (викликається в loop())
    void handle();

    // Отримання поточних налаштувань
    WiFiSettings getSettings() const;

    // Збереження нових параметрів Wi-Fi у постійну пам'ять NVS
    bool saveSettings(const String& staSSID, const String& staPassword,
                      const String& apSSID, const String& apPassword);

    // Скидання налаштувань Wi-Fi до значень за замовчуванням
    bool resetSettings();

    // Планування плавного перезавантаження контролера
    void scheduleRestart(uint32_t delayMs = 1500);

    // Інформаційні методи про поточне з'єднання
    bool isConnectedSTA() const;
    String getModeStr() const;
    String getIPAddress() const;
    String getCurrentSSID() const;

    // Сканування доступних 2.4 ГГц мереж навколо
    void scanNetworks(JsonArray& outArray);

private:
    WiFiSettings currentSettings;
    uint32_t restartAtMillis;
    bool isRestartScheduled;

    void loadFromPreferences();
};

extern TableWiFiManager wifiMgr;

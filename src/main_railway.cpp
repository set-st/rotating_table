#include <Arduino.h>
#include "config.h"
#include "railway_controller.h"
#include "railway_web.h"
#include "wifi_manager.h"

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n==================================================");
    Serial.println("  ESP32 RailWay (MKS DLC32): Прошивка Керування");
    Serial.println("==================================================");

    if (!railway.begin()) {
        Serial.println("[Error] Помилка ініціалізації контролера RailWay!");
    }

    wifiMgr.begin();
    wifiMgr.setupWiFi();
    railwayWeb.begin();

    if (railway.getSettings().autoHomeOnBoot) {
        String error;
        Serial.println("[Boot] Автоматичний пошук кінцевиків при старті...");
        if (!railway.startHoming(error)) {
            Serial.printf("[Boot] Homing не запущено: %s\n", error.c_str());
        }
    }

    Serial.println("[System] Ініціалізація завершена. Система готова до роботи.");
}

void loop() {
    railwayWeb.handleClient();
    wifiMgr.handle();
    delay(2);
}

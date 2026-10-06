#include <Arduino.h>
#include "config.h"
#include "motion_controller.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "imu_sensor.h"
#include "ota_updater.h"
#include "table_ble_client.h"

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n==================================================");
    Serial.println("  ESP32 Поворотний Стіл: Прошивка Керування");
    Serial.println("==================================================");

    // 1. Ініціалізація контролера руху та фонового завдання FreeRTOS
    if (!motionCtrl.begin()) {
        Serial.println("[Error] Помилка ініціалізації контролера руху!");
    }

    if (!imuSensor.begin()) {
        Serial.println("[Error] Помилка ініціалізації GY-87/MPU6050!");
    }

    // 2. Ініціалізація Bluetooth зв'язку з платформою нахилу
    tableBleClient.begin();

    // 3. Налаштування мережі Wi-Fi (зчитування NVS або fallback на config.h, mDNS)
    wifiMgr.begin();
    wifiMgr.setupWiFi();

    // 4. Запуск веб-сервера
    webServer.begin();

    // 5. Автоматичний пошук кінцевика при старті, якщо увімкнено у налаштуваннях
    if (motionCtrl.getConfig().autoHomeOnBoot) {
        Serial.println("[Boot] Запуск автоматичного калібрування нуля при старті...");
        motionCtrl.startHoming();
    }

    Serial.println("[System] Ініціалізація завершена. Система готова до роботи.");
}

void loop() {
    // Обробка вхідних HTTP-запитів
    webServer.handleClient();
    imuSensor.update();

    // Оновлення Bluetooth зв'язку з платформою нахилу
    tableBleClient.update();

    // Обробка запланованого відкладеного перезавантаження після зміни налаштувань
    wifiMgr.handle();

    delay(2);
}

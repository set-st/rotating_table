#include <Arduino.h>
#include "tilt_config.h"
#include "tilt_controller.h"
#include "tilt_imu.h"
#include "tilt_ble_server.h"

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n==================================================");
    Serial.println("  ESP32 Платформа Нахилу: Прошивка Керування");
    Serial.println("==================================================");

    // 1. Ініціалізація контролера руху (драйвер з опторозв'язкою, кінцевик, FreeRTOS завдання)
    if (!tiltCtrl.begin()) {
        Serial.println("[Error] Помилка ініціалізації контролера руху нахилу!");
    }

    // 2. Ініціалізація гіроскопа GY-521 (MPU-6050)
    TiltHardwareConfig cfg = tiltCtrl.getConfig();
    if (!tiltImu.begin(cfg.pinSda, cfg.pinScl, cfg.mpuAddress, cfg.tiltAxis)) {
        Serial.println("[Warning] GY-521 (MPU6050) не виявлено! Перевірте підключення I2C.");
    }

    // 3. Ініціалізація Bluetooth BLE Сервера
    if (!tiltBleServer.begin(TILT_DEFAULT_DEVICE_NAME)) {
        Serial.println("[Error] Помилка запуску Bluetooth BLE сервера!");
    }

    // 4. Автоматичний пошук крайньої точки (Homing) при увімкненні, якщо налаштовано
    if (cfg.autoHomeOnBoot) {
        Serial.println("[Boot] Запуск автоматичного пошуку крайньої точки...");
        tiltCtrl.startHoming();
    }

    Serial.println("[System] Платформа нахилу готова до роботи.");
}

void loop() {
    // Оновлення фільтра гіроскопа та контуру підтримання кута (closed-loop holding)
    tiltCtrl.update();

    // Оновлення передачі телеметрії через Bluetooth BLE
    tiltBleServer.update();

    delay(2);
}

#include "tilt_ble_server.h"
#include "tilt_controller.h"
#include <ArduinoJson.h>
#include <Update.h>
#include <stdlib.h>

TiltBleServer tiltBleServer;

class TiltServerCallbacks : public NimBLEServerCallbacks {
    TiltBleServer* parent;
public:
    TiltServerCallbacks(TiltBleServer* p) : parent(p) {}
    void onConnect(NimBLEServer* pServer) override {
        parent->deviceConnected = true;
        Serial.println("[Tilt BLE] Центральний пристрій підключено.");
    }
    void onDisconnect(NimBLEServer* pServer) override {
        if (parent->otaInProgress) {
            Update.abort();
            parent->otaInProgress = false;
            parent->setOtaStatus("ERROR:connection lost");
        }
        parent->deviceConnected = false;
        Serial.println("[Tilt BLE] Центральний пристрій відключено. Відновлення реклами...");
    }
};

class TiltCmdCallbacks : public NimBLECharacteristicCallbacks {
    TiltBleServer* parent;
public:
    TiltCmdCallbacks(TiltBleServer* p) : parent(p) {}
    void onWrite(NimBLECharacteristic* pCharacteristic) override {
        std::string value = pCharacteristic->getValue();
        if (!value.empty()) {
            parent->processCommand(value.c_str());
        }
    }
};

class TiltSettingsCallbacks : public NimBLECharacteristicCallbacks {
    TiltBleServer* parent;
public:
    TiltSettingsCallbacks(TiltBleServer* p) : parent(p) {}
    void onWrite(NimBLECharacteristic* pCharacteristic) override {
        std::string value = pCharacteristic->getValue();
        if (!value.empty()) {
            parent->processSettingsWrite(value.c_str());
        }
    }
};

class TiltOtaControlCallbacks : public NimBLECharacteristicCallbacks {
    TiltBleServer* parent;
public:
    explicit TiltOtaControlCallbacks(TiltBleServer* p) : parent(p) {}
    void onRead(NimBLECharacteristic* characteristic) override {
        characteristic->setValue(parent->otaStatus.c_str());
    }
    void onWrite(NimBLECharacteristic* characteristic) override {
        parent->processOtaControl(characteristic->getValue());
    }
};

class TiltOtaDataCallbacks : public NimBLECharacteristicCallbacks {
    TiltBleServer* parent;
public:
    explicit TiltOtaDataCallbacks(TiltBleServer* p) : parent(p) {}
    void onWrite(NimBLECharacteristic* characteristic) override {
        std::string value = characteristic->getValue();
        uint8_t* data = value.empty()
                            ? nullptr
                            : reinterpret_cast<uint8_t*>(&value[0]);
        parent->processOtaData(data, value.size());
    }
};

TiltBleServer::TiltBleServer()
    : pServer(nullptr), pCmdChar(nullptr), pStatusChar(nullptr), pSettingsChar(nullptr),
      pOtaControlChar(nullptr), pOtaDataChar(nullptr),
      deviceConnected(false), oldDeviceConnected(false), lastStatusNotifyMs(0),
      otaStatus("IDLE"), otaInProgress(false), otaExpectedBytes(0),
      otaReceivedBytes(0), otaRestartAt(0) {}

bool TiltBleServer::begin(const char* deviceName) {
    NimBLEDevice::init(deviceName);
    NimBLEDevice::setMTU(517);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); // Максимальна потужність сигналу

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new TiltServerCallbacks(this));

    NimBLEService* pService = pServer->createService(TILT_BLE_SERVICE_UUID);

    // 1. Характеристика команд (Write / Write without response)
    pCmdChar = pService->createCharacteristic(
        TILT_BLE_CHAR_CMD_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    pCmdChar->setCallbacks(new TiltCmdCallbacks(this));

    // 2. Характеристика статусу (Read / Notify)
    pStatusChar = pService->createCharacteristic(
        TILT_BLE_CHAR_STATUS_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    // 3. Характеристика налаштувань (Read / Write / Notify)
    pSettingsChar = pService->createCharacteristic(
        TILT_BLE_CHAR_SETTINGS_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY
    );
    pSettingsChar->setCallbacks(new TiltSettingsCallbacks(this));

    pOtaControlChar = pService->createCharacteristic(
        TILT_BLE_CHAR_OTA_CONTROL_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
    );
    pOtaControlChar->setCallbacks(new TiltOtaControlCallbacks(this));
    pOtaControlChar->setValue(otaStatus.c_str());

    pOtaDataChar = pService->createCharacteristic(
        TILT_BLE_CHAR_OTA_DATA_UUID,
        NIMBLE_PROPERTY::WRITE
    );
    pOtaDataChar->setCallbacks(new TiltOtaDataCallbacks(this));

    pService->start();

    // Запуск реклами BLE для виявлення основною платою
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(TILT_BLE_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->start();

    Serial.printf("[Tilt BLE] BLE Сервер запущено. Ім'я пристрою: '%s'\n", deviceName);
    sendSettingsNotification();
    return true;
}

void TiltBleServer::setOtaStatus(const String& status) {
    otaStatus = status;
    if (pOtaControlChar) {
        pOtaControlChar->setValue(otaStatus.c_str());
    }
}

void TiltBleServer::processOtaControl(const std::string& payload) {
    if (payload.rfind("BEGIN:", 0) == 0) {
        char* end = nullptr;
        const unsigned long size = strtoul(payload.c_str() + 6, &end, 10);
        if (otaInProgress || !end || *end != '\0' || size == 0) {
            setOtaStatus("ERROR:invalid begin");
            return;
        }

        tiltCtrl.emergencyStop();
        tiltCtrl.setDriverEnabled(false);
        if (!Update.begin(size, U_FLASH)) {
            setOtaStatus("ERROR:update begin");
            return;
        }

        otaExpectedBytes = size;
        otaReceivedBytes = 0;
        otaInProgress = true;
        setOtaStatus("READY");
        Serial.printf("[Tilt OTA] Початок BLE OTA, розмір %lu байт.\n", size);
        return;
    }

    if (payload == "FINISH") {
        if (!otaInProgress || otaReceivedBytes != otaExpectedBytes) {
            Update.abort();
            otaInProgress = false;
            setOtaStatus("ERROR:size mismatch");
            return;
        }

        if (!Update.end() || !Update.isFinished()) {
            const uint8_t error = Update.getError();
            Update.abort();
            otaInProgress = false;
            setOtaStatus("ERROR:update finish " + String(error));
            return;
        }

        otaInProgress = false;
        otaRestartAt = millis() + 2000;
        setOtaStatus("DONE");
        Serial.println("[Tilt OTA] Прошивку записано; заплановано перезавантаження.");
        return;
    }

    if (payload == "ABORT") {
        if (otaInProgress) {
            Update.abort();
            otaInProgress = false;
            otaRestartAt = 0;
            setOtaStatus("ABORTED");
        }
        return;
    }

    setOtaStatus("ERROR:unknown command");
}

void TiltBleServer::processOtaData(uint8_t* data, size_t length) {
    if (!otaInProgress || length == 0 ||
        otaReceivedBytes + length > otaExpectedBytes) {
        if (otaInProgress) {
            Update.abort();
            otaInProgress = false;
        }
        setOtaStatus("ERROR:invalid data");
        return;
    }

    const size_t written = Update.write(data, length);
    if (written != length) {
        const uint8_t error = Update.getError();
        Update.abort();
        otaInProgress = false;
        setOtaStatus("ERROR:update write " + String(error));
        return;
    }
    otaReceivedBytes += written;
}

void TiltBleServer::processCommand(const char* payload) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("[Tilt BLE] Помилка парсингу JSON команди: %s\n", err.c_str());
        return;
    }

    const char* cmd = doc["cmd"] | "";
    if (strcmp(cmd, "move") == 0) {
        float angle = doc["angle"] | 0.0f;
        float speed = doc["speed"] | 0.0f;
        bool relative = doc["relative"] | false;
        tiltCtrl.moveTo(angle, speed, relative);
    } else if (strcmp(cmd, "home") == 0) {
        tiltCtrl.startHoming();
    } else if (strcmp(cmd, "stop") == 0) {
        tiltCtrl.emergencyStop();
    } else if (strcmp(cmd, "zero") == 0) {
        tiltCtrl.setZero();
    } else if (strcmp(cmd, "gyro_zero") == 0) {
        tiltCtrl.resetGyroZero();
    } else if (strcmp(cmd, "gyro_cal") == 0) {
        tiltCtrl.calibrateGyro();
    } else if (strcmp(cmd, "hold") == 0) {
        bool enabled = doc["enabled"] | false;
        float target = doc["target"] | 0.0f;
        tiltCtrl.setHoldActive(enabled, target);
    } else if (strcmp(cmd, "get_cfg") == 0) {
        sendSettingsNotification();
    } else {
        Serial.printf("[Tilt BLE] Невідома команда: %s\n", cmd);
    }

    sendStatusNotification();
}

void TiltBleServer::processSettingsWrite(const char* payload) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.printf("[Tilt BLE] Помилка парсингу JSON налаштувань: %s\n", err.c_str());
        return;
    }

    TiltHardwareConfig cfg = tiltCtrl.getConfig();

    if (doc["pin_step"].is<int>()) cfg.pinStep = doc["pin_step"].as<int>();
    if (doc["pin_dir"].is<int>()) cfg.pinDir = doc["pin_dir"].as<int>();
    if (doc["pin_enable"].is<int>()) cfg.pinEnable = doc["pin_enable"].as<int>();
    if (doc["step_active_low"].is<bool>()) cfg.stepActiveLow = doc["step_active_low"].as<bool>();
    if (doc["dir_positive_high"].is<bool>()) cfg.dirPositiveHigh = doc["dir_positive_high"].as<bool>();
    if (doc["enable_active_high"].is<bool>()) cfg.enableActiveHigh = doc["enable_active_high"].as<bool>();

    if (doc["pin_endstop"].is<int>()) cfg.pinEndstop = doc["pin_endstop"].as<int>();
    if (doc["endstop_inverted"].is<bool>()) cfg.endstopInverted = doc["endstop_inverted"].as<bool>();
    if (doc["endstop_debounce_ms"].is<uint32_t>()) cfg.endstopDebounceMs = doc["endstop_debounce_ms"].as<uint32_t>();

    if (doc["pin_sda"].is<int>()) cfg.pinSda = doc["pin_sda"].as<int>();
    if (doc["pin_scl"].is<int>()) cfg.pinScl = doc["pin_scl"].as<int>();
    if (doc["mpu_addr"].is<uint8_t>()) cfg.mpuAddress = doc["mpu_addr"].as<uint8_t>();
    if (doc["tilt_axis"].is<int>()) cfg.tiltAxis = doc["tilt_axis"].as<int>();

    if (doc["steps_per_rev"].is<float>()) cfg.stepsPerRev = doc["steps_per_rev"].as<float>();
    if (doc["microsteps"].is<float>()) cfg.microsteps = doc["microsteps"].as<float>();
    if (doc["gear_ratio"].is<float>()) cfg.gearRatio = doc["gear_ratio"].as<float>();

    if (doc["def_speed"].is<float>()) cfg.defaultSpeed = doc["def_speed"].as<float>();
    if (doc["max_speed"].is<float>()) cfg.maxSpeed = doc["max_speed"].as<float>();
    if (doc["accel"].is<float>()) cfg.acceleration = doc["accel"].as<float>();
    if (doc["min_angle"].is<float>()) cfg.minAngle = doc["min_angle"].as<float>();
    if (doc["max_angle"].is<float>()) cfg.maxAngle = doc["max_angle"].as<float>();

    if (doc["homing_dir"].is<int>()) cfg.homingDirection = doc["homing_dir"].as<int>();
    if (doc["homing_fast_speed"].is<float>()) cfg.homingFastSpeed = doc["homing_fast_speed"].as<float>();
    if (doc["homing_slow_speed"].is<float>()) cfg.homingSlowSpeed = doc["homing_slow_speed"].as<float>();
    if (doc["homing_backoff_deg"].is<float>()) cfg.homingBackoffDeg = doc["homing_backoff_deg"].as<float>();
    if (doc["auto_home"].is<bool>()) cfg.autoHomeOnBoot = doc["auto_home"].as<bool>();

    if (doc["hold_deadband"].is<float>()) cfg.holdDeadband = doc["hold_deadband"].as<float>();
    if (doc["hold_kp"].is<float>()) cfg.holdKp = doc["hold_kp"].as<float>();

    bool rebootRequired = false;
    tiltCtrl.applyConfig(cfg, rebootRequired);
    Serial.println("[Tilt BLE] Нові налаштування прийнято та збережено в NVS.");

    sendSettingsNotification();

    if (rebootRequired) {
        Serial.println("[Tilt BLE] Зміна критичних пінів: перезавантаження через 1 секунду...");
        delay(1000);
        ESP.restart();
    }
}

void TiltBleServer::sendStatusNotification() {
    if (!deviceConnected || !pStatusChar) return;

    TiltStatus st = tiltCtrl.getStatus();

    JsonDocument doc;
    doc["state"] = st.stateStr;
    doc["fw"] = TILT_FIRMWARE_VERSION;
    doc["angle"] = serialized(String(st.gyroAngle, 2));
    doc["motor_angle"] = serialized(String(st.motorAngle, 2));
    doc["target"] = serialized(String(st.targetAngle, 2));
    doc["speed"] = serialized(String(st.currentSpeed, 1));
    doc["homed"] = st.isHomed;
    doc["endstop"] = st.endstopTriggered;
    doc["hold"] = st.holdActive;
    doc["gyro_ok"] = st.gyroConnected;
    doc["pitch"] = serialized(String(st.pitchDeg, 2));
    doc["roll"] = serialized(String(st.rollDeg, 2));
    if (strlen(st.errorMessage) > 0) {
        doc["err"] = st.errorMessage;
    }

    String out;
    serializeJson(doc, out);
    pStatusChar->setValue(out.c_str());
    pStatusChar->notify();
}

void TiltBleServer::sendSettingsNotification() {
    if (!pSettingsChar) return;

    TiltHardwareConfig cfg = tiltCtrl.getConfig();

    JsonDocument doc;
    doc["pin_step"] = cfg.pinStep;
    doc["pin_dir"] = cfg.pinDir;
    doc["pin_enable"] = cfg.pinEnable;
    doc["step_active_low"] = cfg.stepActiveLow;
    doc["dir_positive_high"] = cfg.dirPositiveHigh;
    doc["enable_active_high"] = cfg.enableActiveHigh;

    doc["pin_endstop"] = cfg.pinEndstop;
    doc["endstop_inverted"] = cfg.endstopInverted;
    doc["endstop_debounce_ms"] = cfg.endstopDebounceMs;

    doc["pin_sda"] = cfg.pinSda;
    doc["pin_scl"] = cfg.pinScl;
    doc["mpu_addr"] = cfg.mpuAddress;
    doc["tilt_axis"] = cfg.tiltAxis;

    doc["steps_per_rev"] = cfg.stepsPerRev;
    doc["microsteps"] = cfg.microsteps;
    doc["gear_ratio"] = cfg.gearRatio;

    doc["def_speed"] = cfg.defaultSpeed;
    doc["max_speed"] = cfg.maxSpeed;
    doc["accel"] = cfg.acceleration;
    doc["min_angle"] = cfg.minAngle;
    doc["max_angle"] = cfg.maxAngle;

    doc["homing_dir"] = cfg.homingDirection;
    doc["homing_fast_speed"] = cfg.homingFastSpeed;
    doc["homing_slow_speed"] = cfg.homingSlowSpeed;
    doc["homing_backoff_deg"] = cfg.homingBackoffDeg;
    doc["auto_home"] = cfg.autoHomeOnBoot;

    doc["hold_deadband"] = cfg.holdDeadband;
    doc["hold_kp"] = cfg.holdKp;

    String out;
    serializeJson(doc, out);
    pSettingsChar->setValue(out.c_str());
    if (deviceConnected) {
        pSettingsChar->notify();
    }
}

void TiltBleServer::update() {
    if (otaRestartAt != 0 &&
        static_cast<int32_t>(millis() - otaRestartAt) >= 0) {
        ESP.restart();
    }
    // Відновлення реклами при відключенні
    if (!deviceConnected && oldDeviceConnected) {
        delay(200);
        NimBLEDevice::startAdvertising();
        Serial.println("[Tilt BLE] Рекламу перезапущено.");
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }

    // Періодичне надсилання статусу (кожні 150 мс для плавного оновлення)
    if (deviceConnected && millis() - lastStatusNotifyMs >= 150) {
        lastStatusNotifyMs = millis();
        sendStatusNotification();
    }
}

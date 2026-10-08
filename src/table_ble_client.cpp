#include "table_ble_client.h"
#include <NimBLEDevice.h>
#include <ArduinoJson.h>
#include <Preferences.h>

static const char* NVS_TABLE_BLE_NS = "table_ble";
TableBleClient tableBleClient;

class TableBleClientCallbacks : public NimBLEClientCallbacks {
    TableBleClient* parent;
public:
    TableBleClientCallbacks(TableBleClient* p) : parent(p) {}
    uint32_t onPassKeyRequest() override {
        Serial.printf("[Table BLE Client] BLE passkey request: %06u\n", TILT_BLE_PAIR_PASSKEY);
        return TILT_BLE_PAIR_PASSKEY;
    }
    void onConnect(NimBLEClient* pClient) override {
        Serial.println("[Table BLE Client] З'єднання з платформою нахилу встановлено.");
    }
    void onDisconnect(NimBLEClient* pClient) override {
        parent->connected = false;
        parent->pCmdChar = nullptr;
        parent->pStatusChar = nullptr;
        parent->pSettingsChar = nullptr;
        parent->pOtaControlChar = nullptr;
        parent->pOtaDataChar = nullptr;
        Serial.println("[Table BLE Client] Зв'язок із платформою нахилу втрачено.");
    }
};

TableBleClient::TableBleClient()
    : connected(false), connecting(false), autoConnectEnabled(true),
      targetAddress(""), lastRssi(0), lastConnectAttemptMs(0),
    pClient(nullptr), pCmdChar(nullptr), pStatusChar(nullptr), pSettingsChar(nullptr),
    pOtaControlChar(nullptr), pOtaDataChar(nullptr) {
    mutex = xSemaphoreCreateMutex();
    cachedStatus.stateStr = "OFFLINE";
    strncpy(cachedStatus.firmwareVersion, "unknown",
            sizeof(cachedStatus.firmwareVersion) - 1);
    cachedSettingsJson = "{}";
}

void TableBleClient::loadNvs() {
    Preferences prefs;
    if (prefs.begin(NVS_TABLE_BLE_NS, true)) {
        targetAddress = prefs.getString("mac", "");
        autoConnectEnabled = prefs.getBool("auto", true);
        prefs.end();
        Serial.printf("[Table BLE Client] NVS: Цільова адреса: '%s', Авто-підключення: %d\n",
                      targetAddress.c_str(), autoConnectEnabled);
    }
}

void TableBleClient::saveNvs() {
    Preferences prefs;
    if (prefs.begin(NVS_TABLE_BLE_NS, false)) {
        prefs.putString("mac", targetAddress);
        prefs.putBool("auto", autoConnectEnabled);
        prefs.end();
    }
}

bool TableBleClient::begin() {
    loadNvs();
    NimBLEDevice::setSecurityAuth(true, true, true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
    NimBLEDevice::setSecurityPasskey(TILT_BLE_PAIR_PASSKEY);
    NimBLEDevice::init("RotatingTable-Client");
    NimBLEDevice::setMTU(517);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    Serial.printf("[Table BLE Client] MAC: %s | Common BLE passkey: %06u\n",
                  NimBLEDevice::getAddress().toString().c_str(), TILT_BLE_PAIR_PASSKEY);
    Serial.println("[Table BLE Client] BLE клієнт ініціалізовано.");
    return true;
}

bool TableBleClient::connectInternal(const String& address) {
    if (connecting) return false;
    connecting = true;

    NimBLEClient* client = static_cast<NimBLEClient*>(pClient);
    if (!client) {
        client = NimBLEDevice::createClient();
        client->setClientCallbacks(new TableBleClientCallbacks(this));
        pClient = client;
    }

    if (client->isConnected()) {
        client->disconnect();
        delay(100);
    }

    NimBLEAddress bleAddr;
    if (address.length() > 0) {
        bleAddr = NimBLEAddress(address.c_str());
        Serial.printf("[Table BLE Client] Спроба підключення до MAC: %s...\n", address.c_str());
    } else {
        // Якщо MAC не вказано, шукаємо пристрій за UUID сервісу або за назвою
        Serial.println("[Table BLE Client] Пошук пристрою TiltTable поблизу...");
        NimBLEScan* pScan = NimBLEDevice::getScan();
        pScan->setActiveScan(true);
        pScan->setInterval(100);
        pScan->setWindow(99);
        NimBLEScanResults results = pScan->start(2, false);

        bool found = false;
        for (int i = 0; i < results.getCount(); ++i) {
            NimBLEAdvertisedDevice dev = results.getDevice(i);
            if (dev.isAdvertisingService(NimBLEUUID(TILT_BLE_SERVICE_UUID)) ||
                dev.getName() == TILT_DEFAULT_DEVICE_NAME) {
                bleAddr = dev.getAddress();
                targetAddress = String(bleAddr.toString().c_str());
                saveNvs();
                found = true;
                Serial.printf("[Table BLE Client] Знайдено TiltTable: %s (%s, RSSI %d)\n",
                              dev.getName().c_str(), targetAddress.c_str(), dev.getRSSI());
                break;
            }
        }
        pScan->clearResults();

        if (!found) {
            connecting = false;
            return false;
        }
    }

    if (!client->connect(bleAddr)) {
        Serial.println("[Table BLE Client] Помилка з'єднання з BLE пристроєм.");
        connecting = false;
        return false;
    }

    if (!client->secureConnection()) {
        Serial.println("[Table BLE Client] Попередження: secureConnection() не виконався, продовжуємо без збору ключа.");
    }

    NimBLERemoteService* pSvc = client->getService(NimBLEUUID(TILT_BLE_SERVICE_UUID));
    if (!pSvc) {
        Serial.println("[Table BLE Client] Сервіс нахилу не знайдено на пристрої!");
        client->disconnect();
        connecting = false;
        return false;
    }

    NimBLERemoteCharacteristic* pCmd = pSvc->getCharacteristic(NimBLEUUID(TILT_BLE_CHAR_CMD_UUID));
    NimBLERemoteCharacteristic* pStat = pSvc->getCharacteristic(NimBLEUUID(TILT_BLE_CHAR_STATUS_UUID));
    NimBLERemoteCharacteristic* pSet = pSvc->getCharacteristic(NimBLEUUID(TILT_BLE_CHAR_SETTINGS_UUID));
    NimBLERemoteCharacteristic* pOtaControl =
        pSvc->getCharacteristic(NimBLEUUID(TILT_BLE_CHAR_OTA_CONTROL_UUID));
    NimBLERemoteCharacteristic* pOtaData =
        pSvc->getCharacteristic(NimBLEUUID(TILT_BLE_CHAR_OTA_DATA_UUID));

    if (!pCmd || !pStat) {
        Serial.println("[Table BLE Client] Необхідні характеристики не знайдено!");
        client->disconnect();
        connecting = false;
        return false;
    }

    pCmdChar = pCmd;
    pStatusChar = pStat;
    pSettingsChar = pSet;
    pOtaControlChar = pOtaControl;
    pOtaDataChar = pOtaData;

    // Підписка на сповіщення статусу
    if (pStat->canNotify()) {
        pStat->subscribe(true, [this](NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
            String jsonStr((char*)pData, length);
            JsonDocument doc;
            DeserializationError err = deserializeJson(doc, jsonStr);
            if (!err) {
                if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                    cachedStatus.stateStr = strdup(doc["state"] | "IDLE");
                    cachedStatus.state = stringToTiltState(cachedStatus.stateStr);
                    strlcpy(cachedStatus.firmwareVersion,
                            doc["fw"] | "unknown",
                            sizeof(cachedStatus.firmwareVersion));
                    cachedStatus.gyroAngle = doc["angle"] | 0.0f;
                    cachedStatus.motorAngle = doc["motor_angle"] | 0.0f;
                    cachedStatus.targetAngle = doc["target"] | 0.0f;
                    cachedStatus.currentSpeed = doc["speed"] | 0.0f;
                    cachedStatus.isHomed = doc["homed"] | false;
                    cachedStatus.endstopTriggered = doc["endstop"] | false;
                    cachedStatus.holdActive = doc["hold"] | false;
                    cachedStatus.gyroConnected = doc["gyro_ok"] | false;
                    cachedStatus.pitchDeg = doc["pitch"] | 0.0f;
                    cachedStatus.rollDeg = doc["roll"] | 0.0f;
                    if (doc["err"].is<const char*>()) {
                        strncpy(cachedStatus.errorMessage, doc["err"], sizeof(cachedStatus.errorMessage) - 1);
                    } else {
                        cachedStatus.errorMessage[0] = '\0';
                    }
                    xSemaphoreGive(mutex);
                }
            }
        });
    }

    // Підписка на оновлення налаштувань
    if (pSet) {
        if (pSet->canRead()) {
            std::string val = pSet->readValue();
            if (!val.empty()) {
                cachedSettingsJson = val.c_str();
            }
        }
        if (pSet->canNotify()) {
            pSet->subscribe(true, [this](NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t length, bool isNotify) {
                cachedSettingsJson = String((char*)pData, length);
            });
        }
    }

    lastRssi = client->getRssi();
    connected = true;
    connecting = false;
    Serial.println("[Table BLE Client] Платформу нахилу успішно підключено та синхронізовано.");
    return true;
}

void TableBleClient::update() {
    NimBLEClient* client = static_cast<NimBLEClient*>(pClient);
    if (client && client->isConnected()) {
        connected = true;
        // Періодично оновлюємо RSSI
        static uint32_t lastRssiCheck = 0;
        if (millis() - lastRssiCheck > 2000) {
            lastRssiCheck = millis();
            lastRssi = client->getRssi();
        }
    } else {
        connected = false;
        if (autoConnectEnabled && !connecting && millis() - lastConnectAttemptMs > 4000) {
            lastConnectAttemptMs = millis();
            connectInternal(targetAddress);
        }
    }
}

bool TableBleClient::isConnected() const {
    return connected;
}

int TableBleClient::getRssi() const {
    return lastRssi;
}

TiltStatus TableBleClient::getStatus() {
    TiltStatus st;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        st = cachedStatus;
        xSemaphoreGive(mutex);
    }
    if (!connected) {
        st.stateStr = "DISCONNECTED";
    }
    return st;
}

String TableBleClient::getSettingsJson() {
    return cachedSettingsJson;
}

size_t TableBleClient::getOtaChunkSize() {
    if (!connected || !pClient || !pOtaControlChar || !pOtaDataChar) {
        return 0;
    }
    NimBLEClient* client = static_cast<NimBLEClient*>(pClient);
    const uint16_t mtu = client->getMTU();
    return mtu > 3 ? min(static_cast<size_t>(mtu - 3), static_cast<size_t>(512))
                   : 0;
}

bool TableBleClient::beginTiltOta(size_t firmwareSize, String* outStatus) {
    if (!connected || !pOtaControlChar || !pOtaDataChar ||
        firmwareSize == 0 || firmwareSize > UINT32_MAX) {
        if (outStatus) {
            *outStatus = "Плата нахилу не підключена або OTA-канал недоступний";
        }
        return false;
    }
    NimBLERemoteCharacteristic* control =
        static_cast<NimBLERemoteCharacteristic*>(pOtaControlChar);
    const String command = "BEGIN:" + String(static_cast<uint32_t>(firmwareSize));
    if (!control->writeValue(command.c_str(), true)) {
        if (outStatus) {
            *outStatus = "Не вдалося надіслати команду BEGIN на плату нахилу";
        }
        return false;
    }
    const std::string response = control->readValue();
    const String status = response.empty() ? "" : String(response.c_str());
    if (outStatus) {
        *outStatus = status;
    }
    return status == "READY";
}

bool TableBleClient::writeTiltOtaChunk(const uint8_t* data, size_t length) {
    if (!connected || !pOtaDataChar || !data || length == 0 ||
        length > getOtaChunkSize()) {
        return false;
    }
    NimBLERemoteCharacteristic* characteristic =
        static_cast<NimBLERemoteCharacteristic*>(pOtaDataChar);
    return characteristic->writeValue(data, length, true);
}

bool TableBleClient::finishTiltOta() {
    if (!connected || !pOtaControlChar) {
        return false;
    }
    NimBLERemoteCharacteristic* control =
        static_cast<NimBLERemoteCharacteristic*>(pOtaControlChar);
    if (!control->writeValue("FINISH", true)) {
        return false;
    }
    const std::string response = control->readValue();
    return response == "DONE";
}

void TableBleClient::abortTiltOta() {
    if (!connected || !pOtaControlChar) {
        return;
    }
    NimBLERemoteCharacteristic* control =
        static_cast<NimBLERemoteCharacteristic*>(pOtaControlChar);
    control->writeValue("ABORT", true);
}

bool TableBleClient::sendRawCommand(const char* jsonPayload) {
    if (!connected || !pCmdChar) return false;
    NimBLERemoteCharacteristic* pCmd = static_cast<NimBLERemoteCharacteristic*>(pCmdChar);
    return pCmd->writeValue(jsonPayload, false);
}

bool TableBleClient::sendMove(float angle, float speed, bool relative) {
    JsonDocument doc;
    doc["cmd"] = "move";
    doc["angle"] = angle;
    doc["speed"] = speed;
    doc["relative"] = relative;
    String out;
    serializeJson(doc, out);
    return sendRawCommand(out.c_str());
}

bool TableBleClient::sendHome() {
    return sendRawCommand("{\"cmd\":\"home\"}");
}

bool TableBleClient::sendStop() {
    return sendRawCommand("{\"cmd\":\"stop\"}");
}

bool TableBleClient::sendZero() {
    return sendRawCommand("{\"cmd\":\"zero\"}");
}

bool TableBleClient::sendGyroZero() {
    return sendRawCommand("{\"cmd\":\"gyro_zero\"}");
}

bool TableBleClient::sendGyroCalibrate() {
    return sendRawCommand("{\"cmd\":\"gyro_cal\"}");
}

bool TableBleClient::sendHold(bool enabled, float targetAngle) {
    JsonDocument doc;
    doc["cmd"] = "hold";
    doc["enabled"] = enabled;
    doc["target"] = targetAngle;
    String out;
    serializeJson(doc, out);
    return sendRawCommand(out.c_str());
}

bool TableBleClient::sendSettings(const String& jsonPayload) {
    if (!connected || !pSettingsChar) return false;
    NimBLERemoteCharacteristic* pSet = static_cast<NimBLERemoteCharacteristic*>(pSettingsChar);
    bool ok = pSet->writeValue(jsonPayload.c_str(), true);
    if (ok) {
        cachedSettingsJson = jsonPayload;
    }
    return ok;
}

std::vector<BleDiscoveredDevice> TableBleClient::scanDevices(uint32_t durationSec) {
    std::vector<BleDiscoveredDevice> list;
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setActiveScan(true);
    pScan->setInterval(45);
    pScan->setWindow(15);
    pScan->setDuplicateFilter(false);
    NimBLEScanResults results = pScan->start(durationSec, false);

    for (int i = 0; i < results.getCount(); ++i) {
        NimBLEAdvertisedDevice dev = results.getDevice(i);
        BleDiscoveredDevice d;
        const std::string devName = dev.getName();
        d.name = devName.empty() ? String(TILT_DEFAULT_DEVICE_NAME) : String(devName.c_str());
        d.address = String(dev.getAddress().toString().c_str());
        d.rssi = dev.getRSSI();
        Serial.printf("[Table BLE Client] Scan result: %s (%s, RSSI %d)\n",
                      d.name.c_str(), d.address.c_str(), d.rssi);
        list.push_back(d);
    }
    pScan->clearResults();
    return list;
}

bool TableBleClient::connectTo(const String& address) {
    targetAddress = address;
    saveNvs();
    return connectInternal(address);
}

void TableBleClient::disconnect() {
    NimBLEClient* client = static_cast<NimBLEClient*>(pClient);
    if (client && client->isConnected()) {
        client->disconnect();
    }
    connected = false;
}

void TableBleClient::setAutoConnect(bool enable) {
    autoConnectEnabled = enable;
    saveNvs();
}

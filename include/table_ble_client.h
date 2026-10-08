#pragma once
#include <Arduino.h>
#include <vector>
#include "tilt_protocol.h"
#include "tilt_config.h"

struct BleDiscoveredDevice {
    String name;
    String address;
    int rssi;
};

class TableBleClient {
public:
    TableBleClient();

    bool begin();
    void update();

    bool isConnected() const;
    int getRssi() const;
    TiltStatus getStatus();
    size_t getOtaChunkSize();
    bool beginTiltOta(size_t firmwareSize, String* outStatus = nullptr);
    bool writeTiltOtaChunk(const uint8_t* data, size_t length);
    bool finishTiltOta();
    void abortTiltOta();
    String getSettingsJson();

    // Відправка команд на плату нахилу
    bool sendMove(float angle, float speed = 0.0f, bool relative = false);
    bool sendHome();
    bool sendStop();
    bool sendZero();
    bool sendGyroZero();
    bool sendGyroCalibrate();
    bool sendHold(bool enabled, float targetAngle = 0.0f);
    bool sendSettings(const String& jsonPayload);

    // Керування підключенням та сканування
    std::vector<BleDiscoveredDevice> scanDevices(uint32_t durationSec = 3);
    bool connectTo(const String& address);
    void disconnect();
    void setAutoConnect(bool enable);
    bool isAutoConnect() const { return autoConnectEnabled; }
    String getTargetAddress() const { return targetAddress; }

private:
    bool connected;
    bool connecting;
    bool autoConnectEnabled;
    String targetAddress;
    int lastRssi;

    TiltStatus cachedStatus;
    String cachedSettingsJson;
    uint32_t lastConnectAttemptMs;
    SemaphoreHandle_t mutex;

    void* pClient;
    void* pCmdChar;
    void* pStatusChar;
    void* pSettingsChar;
    void* pOtaControlChar;
    void* pOtaDataChar;

    void loadNvs();
    void saveNvs();
    bool connectInternal(const String& address);
    bool sendRawCommand(const char* jsonPayload);

    friend class TableBleClientCallbacks;
};

extern TableBleClient tableBleClient;

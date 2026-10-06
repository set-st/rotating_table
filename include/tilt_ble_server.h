#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>
#include "tilt_protocol.h"
#include "tilt_config.h"

class TiltBleServer {
public:
    TiltBleServer();

    bool begin(const char* deviceName = TILT_DEFAULT_DEVICE_NAME);
    void update();

    bool isClientConnected() const { return deviceConnected; }
    void sendStatusNotification();
    void sendSettingsNotification();

private:
    NimBLEServer* pServer;
    NimBLECharacteristic* pCmdChar;
    NimBLECharacteristic* pStatusChar;
    NimBLECharacteristic* pSettingsChar;
    NimBLECharacteristic* pOtaControlChar;
    NimBLECharacteristic* pOtaDataChar;

    bool deviceConnected;
    bool oldDeviceConnected;
    uint32_t lastStatusNotifyMs;
    String otaStatus;
    bool otaInProgress;
    size_t otaExpectedBytes;
    size_t otaReceivedBytes;
    uint32_t otaRestartAt;

    void processCommand(const char* payload);
    void processSettingsWrite(const char* payload);
    void processOtaControl(const std::string& payload);
    void processOtaData(uint8_t* data, size_t length);
    void setOtaStatus(const String& status);

    friend class TiltServerCallbacks;
    friend class TiltCmdCallbacks;
    friend class TiltSettingsCallbacks;
    friend class TiltOtaControlCallbacks;
    friend class TiltOtaDataCallbacks;
};

extern TiltBleServer tiltBleServer;

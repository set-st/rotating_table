#pragma once

#include <Arduino.h>

struct ImuConfig {
    int sdaPin;
    int sclPin;
    uint8_t mpu6050Address;
    uint8_t barometerAddress;
};

struct ImuStatus {
    bool initialized;
    bool mpu6050Connected;
    bool barometerConnected;
    float pitchDeg;
    float rollDeg;
    float gyroXDegS;
    float gyroYDegS;
    float gyroZDegS;
    uint32_t updatedAtMs;
    char errorMessage[64];
};

struct ImuScanResult {
    uint8_t addresses[32];
    uint8_t count;
};

class ImuSensor {
public:
    ImuSensor();

    bool begin();
    void update();
    ImuConfig getConfig() const;
    ImuStatus getStatus() const;
    ImuScanResult scanBus();
    bool applyConfig(const ImuConfig& newConfig, bool& rebootRequired);

private:
    ImuConfig config;
    ImuStatus status;
    float gyroBiasX;
    float gyroBiasY;
    float gyroBiasZ;
    uint32_t lastUpdateMs;
    uint32_t lastSampleUs;

    void loadConfig();
    void saveConfig();
    bool probe(uint8_t address);
    bool readMpuSample(int16_t& accelX, int16_t& accelY, int16_t& accelZ,
                       int16_t& gyroX, int16_t& gyroY, int16_t& gyroZ);
    void setError(const char* message);
};

extern ImuSensor imuSensor;

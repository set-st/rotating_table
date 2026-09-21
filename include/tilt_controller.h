#pragma once

#include <Arduino.h>

struct TiltConfig {
    bool enabled;
    int actuatorPin[4];
    uint8_t pwmChannel[4];
    uint16_t minPulseUs[4];
    uint16_t maxPulseUs[4];
    uint16_t neutralPulseUs[4];
    float actuatorOffsetMm[4];
    bool actuatorInverted[4];
    float minHeightMm;
    float maxHeightMm;
    float maxPitchDeg;
    float maxRollDeg;
    float maxTiltSpeed;
    float pulsePerMm;
};

struct TiltStatus {
    bool enabled;
    bool moving;
    bool stopped;
    float heightMm;
    float pitchDeg;
    float rollDeg;
    float targetHeightMm;
    float targetPitchDeg;
    float targetRollDeg;
    uint16_t pulseUs[4];
    char errorMessage[64];
};

class TiltController {
public:
    TiltController();

    bool begin();
    void update();
    TiltConfig getConfig() const;
    TiltStatus getStatus() const;
    bool applyConfig(const TiltConfig& newConfig, bool& rebootRequired);
    bool moveTo(float heightMm, float pitchDeg, float rollDeg, float speedMmS);
    bool level();
    bool home();
    void emergencyStop();

private:
    TiltConfig config;
    TiltStatus status;
    float currentPulse[4];
    float targetPulse[4];
    float requestedSpeedMmS;
    uint32_t lastUpdateMs;

    void loadConfig();
    void saveConfig();
    bool validateConfig(const TiltConfig& value) const;
    bool calculatePulses(float heightMm, float pitchDeg, float rollDeg,
                         float pulses[4]) const;
    void writePulse(uint8_t index, uint16_t pulseUs);
    void setError(const char* message);
};

extern TiltController tiltController;

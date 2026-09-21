#include "tilt_controller.h"

#include <Preferences.h>
#include <math.h>

static const char* NVS_TILT_NAMESPACE = "table_tilt";
static constexpr uint32_t PWM_FREQUENCY = 50;
static constexpr uint8_t PWM_RESOLUTION = 16;
static constexpr float HALF_PLATFORM_MM = 225.0f;
static constexpr float PI_F = 3.14159265359f;

TiltController tiltController;

TiltController::TiltController()
    : requestedSpeedMmS(10.0f), lastUpdateMs(0) {
    config = {false, {-1, -1, -1, -1}, {0, 1, 2, 3},
              {1000, 1000, 1000, 1000}, {2000, 2000, 2000, 2000},
              {1500, 1500, 1500, 1500}, {0, 0, 0, 0},
              {false, false, false, false}, -25.0f, 25.0f, 3.0f, 3.0f,
              10.0f, 10.0f};
    memset(&status, 0, sizeof(status));
}

void TiltController::loadConfig() {
    Preferences prefs;
    if (!prefs.begin(NVS_TILT_NAMESPACE, true)) {
        return;
    }
    config.enabled = prefs.getBool("enabled", config.enabled);
    for (uint8_t i = 0; i < 4; ++i) {
        char key[16];
        snprintf(key, sizeof(key), "pin%u", i);
        config.actuatorPin[i] = prefs.getInt(key, config.actuatorPin[i]);
        snprintf(key, sizeof(key), "chn%u", i);
        config.pwmChannel[i] = prefs.getUChar(key, config.pwmChannel[i]);
        snprintf(key, sizeof(key), "min%u", i);
        config.minPulseUs[i] = prefs.getUShort(key, config.minPulseUs[i]);
        snprintf(key, sizeof(key), "max%u", i);
        config.maxPulseUs[i] = prefs.getUShort(key, config.maxPulseUs[i]);
        snprintf(key, sizeof(key), "neu%u", i);
        config.neutralPulseUs[i] = prefs.getUShort(key, config.neutralPulseUs[i]);
        snprintf(key, sizeof(key), "off%u", i);
        config.actuatorOffsetMm[i] = prefs.getFloat(key, config.actuatorOffsetMm[i]);
        snprintf(key, sizeof(key), "inv%u", i);
        config.actuatorInverted[i] = prefs.getBool(key, config.actuatorInverted[i]);
    }
    config.minHeightMm = prefs.getFloat("min_h", config.minHeightMm);
    config.maxHeightMm = prefs.getFloat("max_h", config.maxHeightMm);
    config.maxPitchDeg = prefs.getFloat("max_pitch", config.maxPitchDeg);
    config.maxRollDeg = prefs.getFloat("max_roll", config.maxRollDeg);
    config.maxTiltSpeed = prefs.getFloat("speed", config.maxTiltSpeed);
    config.pulsePerMm = prefs.getFloat("pulse_mm", config.pulsePerMm);
    prefs.end();
}

void TiltController::saveConfig() {
    Preferences prefs;
    if (!prefs.begin(NVS_TILT_NAMESPACE, false)) {
        Serial.println("[Tilt] Не вдалося відкрити NVS");
        return;
    }
    prefs.putBool("enabled", config.enabled);
    for (uint8_t i = 0; i < 4; ++i) {
        char key[16];
        snprintf(key, sizeof(key), "pin%u", i); prefs.putInt(key, config.actuatorPin[i]);
        snprintf(key, sizeof(key), "chn%u", i); prefs.putUChar(key, config.pwmChannel[i]);
        snprintf(key, sizeof(key), "min%u", i); prefs.putUShort(key, config.minPulseUs[i]);
        snprintf(key, sizeof(key), "max%u", i); prefs.putUShort(key, config.maxPulseUs[i]);
        snprintf(key, sizeof(key), "neu%u", i); prefs.putUShort(key, config.neutralPulseUs[i]);
        snprintf(key, sizeof(key), "off%u", i); prefs.putFloat(key, config.actuatorOffsetMm[i]);
        snprintf(key, sizeof(key), "inv%u", i); prefs.putBool(key, config.actuatorInverted[i]);
    }
    prefs.putFloat("min_h", config.minHeightMm);
    prefs.putFloat("max_h", config.maxHeightMm);
    prefs.putFloat("max_pitch", config.maxPitchDeg);
    prefs.putFloat("max_roll", config.maxRollDeg);
    prefs.putFloat("speed", config.maxTiltSpeed);
    prefs.putFloat("pulse_mm", config.pulsePerMm);
    prefs.end();
}

bool TiltController::validateConfig(const TiltConfig& value) const {
    if (value.minHeightMm >= value.maxHeightMm ||
        value.maxPitchDeg <= 0.0f || value.maxRollDeg <= 0.0f ||
        value.maxTiltSpeed <= 0.0f || value.pulsePerMm <= 0.0f) {
        return false;
    }
    for (uint8_t i = 0; i < 4; ++i) {
        if (value.actuatorPin[i] < -1 || value.pwmChannel[i] > 15 ||
            value.minPulseUs[i] >= value.maxPulseUs[i] ||
            value.neutralPulseUs[i] < value.minPulseUs[i] ||
            value.neutralPulseUs[i] > value.maxPulseUs[i]) {
            return false;
        }
    }
    return true;
}

bool TiltController::begin() {
    loadConfig();
    if (!validateConfig(config)) {
        setError("Некоректна конфігурація серво");
        return false;
    }
    memset(&status, 0, sizeof(status));
    status.enabled = config.enabled;
    for (uint8_t i = 0; i < 4; ++i) {
        currentPulse[i] = config.neutralPulseUs[i];
        targetPulse[i] = currentPulse[i];
        if (config.actuatorPin[i] >= 0) {
            ledcSetup(config.pwmChannel[i], PWM_FREQUENCY, PWM_RESOLUTION);
            ledcAttachPin(config.actuatorPin[i], config.pwmChannel[i]);
            writePulse(i, static_cast<uint16_t>(currentPulse[i]));
        }
        status.pulseUs[i] = static_cast<uint16_t>(currentPulse[i]);
    }
    lastUpdateMs = millis();
    Serial.printf("[Tilt] Ініціалізація: %s, PWM 50 Гц\n",
                  config.enabled ? "увімкнено" : "вимкнено");
    return true;
}

void TiltController::setError(const char* message) {
    strncpy(status.errorMessage, message, sizeof(status.errorMessage) - 1);
    status.errorMessage[sizeof(status.errorMessage) - 1] = '\0';
}

bool TiltController::calculatePulses(float heightMm, float pitchDeg,
                                     float rollDeg, float pulses[4]) const {
    if (heightMm < config.minHeightMm || heightMm > config.maxHeightMm ||
        fabsf(pitchDeg) > config.maxPitchDeg ||
        fabsf(rollDeg) > config.maxRollDeg) {
        return false;
    }
    const float pitch = pitchDeg * PI_F / 180.0f;
    const float roll = rollDeg * PI_F / 180.0f;
    const float x[4] = {-HALF_PLATFORM_MM, HALF_PLATFORM_MM,
                        HALF_PLATFORM_MM, -HALF_PLATFORM_MM};
    const float y[4] = {-HALF_PLATFORM_MM, -HALF_PLATFORM_MM,
                        HALF_PLATFORM_MM, HALF_PLATFORM_MM};
    for (uint8_t i = 0; i < 4; ++i) {
        const float extension = heightMm + tanf(pitch) * y[i] +
                                tanf(roll) * x[i] + config.actuatorOffsetMm[i];
        const float direction = config.actuatorInverted[i] ? -1.0f : 1.0f;
        pulses[i] = config.neutralPulseUs[i] + direction * extension * config.pulsePerMm;
        if (pulses[i] < config.minPulseUs[i] || pulses[i] > config.maxPulseUs[i]) {
            return false;
        }
    }
    return true;
}

void TiltController::writePulse(uint8_t index, uint16_t pulseUs) {
    if (config.actuatorPin[index] < 0) {
        return;
    }
    const uint32_t duty = (static_cast<uint32_t>(pulseUs) * 65535UL) / 20000UL;
    ledcWrite(config.pwmChannel[index], duty);
}

bool TiltController::moveTo(float heightMm, float pitchDeg, float rollDeg,
                            float speedMmS) {
    if (!config.enabled) {
        setError("Сервоприводи вимкнені або зупинені");
        return false;
    }
    float pulses[4];
    if (!calculatePulses(heightMm, pitchDeg, rollDeg, pulses)) {
        setError("Ціль виходить за межі сервоприводів або кутів");
        return false;
    }
    if (speedMmS <= 0.0f) speedMmS = config.maxTiltSpeed;
    requestedSpeedMmS = constrain(speedMmS, 0.1f, config.maxTiltSpeed);
    status.targetHeightMm = heightMm;
    status.targetPitchDeg = pitchDeg;
    status.targetRollDeg = rollDeg;
    for (uint8_t i = 0; i < 4; ++i) targetPulse[i] = pulses[i];
    status.moving = true;
    status.stopped = false;
    status.errorMessage[0] = '\0';
    return true;
}

bool TiltController::level() {
    return moveTo(status.heightMm, 0.0f, 0.0f, config.maxTiltSpeed);
}

bool TiltController::home() {
    return moveTo(0.0f, 0.0f, 0.0f, config.maxTiltSpeed);
}

void TiltController::emergencyStop() {
    status.stopped = true;
    status.moving = false;
    for (uint8_t i = 0; i < 4; ++i) targetPulse[i] = currentPulse[i];
}

void TiltController::update() {
    if (!status.moving) return;
    const uint32_t now = millis();
    const float dt = (now - lastUpdateMs) / 1000.0f;
    lastUpdateMs = now;
    const float maxDelta = requestedSpeedMmS * config.pulsePerMm * dt;
    bool reached = true;
    for (uint8_t i = 0; i < 4; ++i) {
        const float delta = targetPulse[i] - currentPulse[i];
        if (fabsf(delta) > maxDelta) {
            currentPulse[i] += delta > 0 ? maxDelta : -maxDelta;
            reached = false;
        } else {
            currentPulse[i] = targetPulse[i];
        }
        writePulse(i, static_cast<uint16_t>(constrain(currentPulse[i], 500.0f, 2500.0f)));
        status.pulseUs[i] = static_cast<uint16_t>(currentPulse[i]);
    }
    if (reached) {
        status.heightMm = status.targetHeightMm;
        status.pitchDeg = status.targetPitchDeg;
        status.rollDeg = status.targetRollDeg;
        status.moving = false;
    }
}

TiltConfig TiltController::getConfig() const { return config; }
TiltStatus TiltController::getStatus() const { return status; }

bool TiltController::applyConfig(const TiltConfig& newConfig, bool& rebootRequired) {
    if (!validateConfig(newConfig)) return false;
    rebootRequired = false;
    for (uint8_t i = 0; i < 4; ++i) {
        rebootRequired = rebootRequired ||
            newConfig.actuatorPin[i] != config.actuatorPin[i] ||
            newConfig.pwmChannel[i] != config.pwmChannel[i];
    }
    config = newConfig;
    saveConfig();
    status.enabled = config.enabled;
    return true;
}

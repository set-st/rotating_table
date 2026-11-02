#pragma once
#include <Arduino.h>
#include "railway_config.h"

class RailwayController {
public:
    bool begin();

    RailwayStatus getStatus();
    RailwaySettings getSettings();

    // Перевірка, збереження в NVS та застосування налаштувань
    bool saveSettings(const RailwaySettings& s, String& error);

    bool startHoming(String& error);
    bool moveTo(float xMm, float zMm, float speedMmS, String& error);
    // Відносний рух однієї осі ('x' або 'z'); дозволений і без homing
    bool jog(char axis, float distanceMm, float speedMmS, String& error);
    void stop();
    // Поточну позицію вважати нульовою (X=0, Z=0) та позначити осі як homed
    bool zeroHere(String& error);
    void setEnabled(bool enabled);

private:
    enum class HomePhase : uint8_t { NONE, RELEASE, SEEK, BACKOFF, SLOW, MEASURE, FAR_BACKOFF };

    struct Axis {
        bool moving = false;
        int8_t dir = 1;
        float speed = 0;       // Поточна швидкість, кроків/с
        float vmax = 0;
        float accel = 0;
        uint8_t settle = 0;    // Затримка між зміною DIR та першим імпульсом
    };

    RailwaySettings cfg;
    Axis axes[2];                 // 0 = X, 1 = Z
    bool homed[2] = {false, false};
    bool limits[3] = {false, false, false};   // X, X far, Z (після фільтра)
    bool limitsRaw[3] = {false, false, false};
    RailwayState state = RailwayState::IDLE;
    char errorMsg[96] = {0};
    float targetMm[2] = {0, 0};
    bool outputsEnabled = true;

    bool homingActive = false;
    uint8_t homingIndex = 0;      // Порядок: X (з вимірюванням довжини), потім Z
    int32_t measuredTriggerSteps = 0;
    HomePhase homePhase = HomePhase::NONE;
    uint32_t homePhaseStartedMs = 0;
    uint32_t homePhaseTimeoutMs = 0;

    SemaphoreHandle_t mutex = nullptr;

    static void taskEntry(void* arg);
    void controlStep(float dt);
    void controlAxis(uint8_t a, float dt);
    void updateLimits();
    void updateHoming();
    void beginHomePhase(uint8_t a, HomePhase phase);

    bool limitActive(uint8_t a) const;
    bool farLimitActive(uint8_t a) const;
    void startMove(uint8_t a, int32_t deltaSteps, float vmaxSps, float accelSps2);
    void hardStop(uint8_t a);
    void stopAll();
    void fail(const char* msg);
    void loadSettings();
    float stepsPerMm(uint8_t a) const;
    float lengthMm(uint8_t a) const;
    bool busy() const;
    int32_t positionSteps(uint8_t a) const;
};

extern RailwayController railway;

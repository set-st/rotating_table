#pragma once
#include <Arduino.h>
#include <AccelStepper.h>
#include "tilt_config.h"
#include "tilt_protocol.h"

class TiltController {
public:
    TiltController();
    ~TiltController();

    bool begin();
    void update(); // Викликається в головному циклі (обробка стабілізації тощо)

    // Команди руху
    bool moveTo(float angleDeg, float speedDegS = 0.0f, bool relative = false);
    bool startHoming();
    void emergencyStop();

    // Ручне виставлення в нуль
    void setZero();

    // Скидання гіроскопа та збереження в енергонезалежну пам'ять
    bool resetGyroZero();
    bool calibrateGyro();

    // Увімкнення / вимкнення режиму утримання положення за гіроскопом
    void setHoldActive(bool active, float targetAngle = 0.0f);
    bool isHoldActive() const { return holdActive; }

    // Кінцевик
    bool isEndstopPressed();

    // Стан та конфігурація
    TiltStatus getStatus();
    TiltHardwareConfig getConfig() const;
    bool applyConfig(const TiltHardwareConfig& newCfg, bool& rebootRequired);

    void setDriverEnabled(bool enable);

private:
    AccelStepper stepper;
    TaskHandle_t motionTaskHandle;
    SemaphoreHandle_t mutex;

    TiltHardwareConfig cfg;
    float currentStepsPerDegree;

    volatile TiltState state;
    volatile bool isHomed;
    volatile float targetAngleDeg;
    volatile float requestedSpeedDegS;
    volatile bool holdActive;
    volatile float holdTargetDeg;

    uint32_t endstopTriggerStartTime;
    char lastError[64];

    // Підкроки Homing
    enum TiltHomingStep {
        THOME_FAST,
        THOME_BACKOFF,
        THOME_SLOW,
        THOME_COMPLETE
    };
    TiltHomingStep homingStep;
    uint32_t homingStartTime;

    static void motionTaskEntry(void* parameter);
    void motionLoop();

    long degToSteps(float deg) const;
    float stepsToDeg(long steps) const;
    void setState(TiltState newState, const char* errorMsg = nullptr);

    void loadConfig();
    void saveConfig();
    void applyKinematics();
};

extern TiltController tiltCtrl;

#pragma once
#include <Arduino.h>
#include <AccelStepper.h>
#include "config.h"

enum MotionState {
    STATE_IDLE,
    STATE_HOMING,
    STATE_MOVING,
    STATE_STOPPED,
    STATE_ERROR
};

struct MotionStatus {
    MotionState state;
    const char* stateStr;
    float currentAngle;
    float targetAngle;
    float currentSpeed;
    bool isHomed;
    bool endstopTriggered;
    char errorMessage[64];
};

class MotionController {
public:
    MotionController();
    ~MotionController();

    // Initialize pins and start FreeRTOS background motion task
    bool begin();

    // Trigger homing sequence (returns false if already busy homing)
    bool startHoming();

    // Command rotation to an angle
    // - angleDeg: target angle in degrees (absolute or relative)
    // - speedDegS: speed in degrees per second (<= 0 will use DEFAULT_SPEED_DEG_S)
    // - relative: if true, adds angleDeg to current position; if false, moves to absolute angleDeg
    bool moveTo(float angleDeg, float speedDegS = 0.0f, bool relative = false);

    // Immediate stop
    void emergencyStop();

    // Set current position as 0 degrees without moving
    void setZero();

    // Enable / disable stepper driver outputs
    void setDriverEnabled(bool enable);

    // Get snapshot of current controller status (thread-safe)
    MotionStatus getStatus();

    // Check directly if endstop is currently pressed
    bool isEndstopPressed();

private:
    AccelStepper stepper;
    TaskHandle_t motionTaskHandle;
    SemaphoreHandle_t mutex;

    MotionState state;
    bool isHomed;
    float targetAngleDeg;
    float requestedSpeedDegS;
    char lastError[64];

    // Homing internal sub-states
    enum HomingStep {
        HOME_FAST_APPROACH,
        HOME_BACKOFF,
        HOME_SLOW_APPROACH,
        HOME_COMPLETE
    };
    HomingStep homingStep;
    uint32_t homingStartTime;

    // FreeRTOS background task function
    static void motionTaskEntry(void* parameter);
    void motionLoop();

    // Internal helpers
    long degToSteps(float deg) const;
    float stepsToDeg(long steps) const;
    void setState(MotionState newState, const char* errorMsg = nullptr);
};

extern MotionController motionCtrl;

#include "motion_controller.h"

MotionController motionCtrl;

MotionController::MotionController()
    : stepper(AccelStepper::DRIVER, PIN_STEP, PIN_DIR),
      motionTaskHandle(nullptr),
      mutex(nullptr),
      state(STATE_IDLE),
      isHomed(false),
      targetAngleDeg(0.0f),
      requestedSpeedDegS(DEFAULT_SPEED_DEG_S),
      homingStep(HOME_FAST_APPROACH),
      homingStartTime(0) {
    memset(lastError, 0, sizeof(lastError));
}

MotionController::~MotionController() {
    if (motionTaskHandle != nullptr) {
        vTaskDelete(motionTaskHandle);
    }
    if (mutex != nullptr) {
        vSemaphoreDelete(mutex);
    }
}

bool MotionController::begin() {
    mutex = xSemaphoreCreateMutex();
    if (mutex == nullptr) {
        Serial.println("[Motion] Error: Failed to create mutex!");
        return false;
    }

    // Configure Endstop pin
    pinMode(PIN_ENDSTOP, ENDSTOP_PULLUP ? INPUT_PULLUP : INPUT);

    // Configure Driver Enable pin
    if (PIN_ENABLE >= 0) {
        pinMode(PIN_ENABLE, OUTPUT);
        setDriverEnabled(true);
    }

    // Configure AccelStepper
    stepper.setPinsInverted(INVERT_DIR, false, false);
    stepper.setMaxSpeed(MAX_SPEED_DEG_S * STEPS_PER_DEGREE);
    stepper.setAcceleration(DEFAULT_ACCEL_DEG_S2 * STEPS_PER_DEGREE);
    stepper.setCurrentPosition(0);

    Serial.printf("[Motion] Init OK. Steps/deg: %.3f\n", STEPS_PER_DEGREE);
    Serial.printf("[Motion] Endstop pin: %d (Active %s, Current: %s)\n",
                  PIN_ENDSTOP,
                  ENDSTOP_ACTIVE_LOW ? "LOW" : "HIGH",
                  isEndstopPressed() ? "PRESSED" : "OPEN");

    // Start background task on Core 1
    BaseType_t res = xTaskCreatePinnedToCore(
        motionTaskEntry,
        "MotionTask",
        4096,
        this,
        configMAX_PRIORITIES - 1, // High priority for precise step timing
        &motionTaskHandle,
        1                         // Core 1 (Core 0 handles Wi-Fi/IP stack)
    );

    return (res == pdPASS);
}

void MotionController::setDriverEnabled(bool enable) {
    if (PIN_ENABLE >= 0) {
        bool pinLevel = enable ? (ENABLE_ACTIVE_LOW ? LOW : HIGH)
                               : (ENABLE_ACTIVE_LOW ? HIGH : LOW);
        digitalWrite(PIN_ENABLE, pinLevel);
    }
}

bool MotionController::isEndstopPressed() {
    int val = digitalRead(PIN_ENDSTOP);
    return ENDSTOP_ACTIVE_LOW ? (val == LOW) : (val == HIGH);
}

long MotionController::degToSteps(float deg) const {
    return lroundf(deg * STEPS_PER_DEGREE);
}

float MotionController::stepsToDeg(long steps) const {
    if (STEPS_PER_DEGREE == 0.0f) return 0.0f;
    return (float)steps / STEPS_PER_DEGREE;
}

void MotionController::setState(MotionState newState, const char* errorMsg) {
    state = newState;
    if (errorMsg != nullptr) {
        strncpy(lastError, errorMsg, sizeof(lastError) - 1);
        lastError[sizeof(lastError) - 1] = '\0';
    } else if (newState != STATE_ERROR) {
        lastError[0] = '\0';
    }
}

bool MotionController::startHoming() {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }

    if (state == STATE_HOMING) {
        xSemaphoreGive(mutex);
        return true; // Already homing
    }

    Serial.println("[Motion] Starting homing sequence...");
    setDriverEnabled(true);

    homingStartTime = millis();
    homingStep = HOME_FAST_APPROACH;
    setState(STATE_HOMING);

    // If switch is already pressed at start, directly start backoff
    if (isEndstopPressed()) {
        long backoffSteps = -HOMING_DIRECTION * degToSteps(HOMING_BACKOFF_DEG);
        stepper.setCurrentPosition(0);
        stepper.setMaxSpeed(HOMING_SPEED_FAST_DEG_S * STEPS_PER_DEGREE);
        stepper.setAcceleration(DEFAULT_ACCEL_DEG_S2 * STEPS_PER_DEGREE);
        stepper.moveTo(backoffSteps);
        homingStep = HOME_BACKOFF;
    }

    xSemaphoreGive(mutex);
    return true;
}

bool MotionController::moveTo(float angleDeg, float speedDegS, bool relative) {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }

    if (state == STATE_HOMING) {
        xSemaphoreGive(mutex);
        return false; // Cannot interrupt active homing
    }

    setDriverEnabled(true);

    if (speedDegS <= 0.0f) {
        speedDegS = DEFAULT_SPEED_DEG_S;
    }
    if (speedDegS > MAX_SPEED_DEG_S) {
        speedDegS = MAX_SPEED_DEG_S;
    }

    requestedSpeedDegS = speedDegS;
    stepper.setMaxSpeed(speedDegS * STEPS_PER_DEGREE);
    stepper.setAcceleration(DEFAULT_ACCEL_DEG_S2 * STEPS_PER_DEGREE);

    if (relative) {
        targetAngleDeg = stepsToDeg(stepper.currentPosition()) + angleDeg;
        stepper.move(degToSteps(angleDeg));
    } else {
        targetAngleDeg = angleDeg;
        stepper.moveTo(degToSteps(angleDeg));
    }

    setState(STATE_MOVING);
    Serial.printf("[Motion] Move to %.2f° at %.1f°/s (relative: %s)\n",
                  targetAngleDeg, speedDegS, relative ? "true" : "false");

    xSemaphoreGive(mutex);
    return true;
}

void MotionController::emergencyStop() {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        stepper.stop();
        stepper.setCurrentPosition(stepper.currentPosition());
        targetAngleDeg = stepsToDeg(stepper.currentPosition());
        setState(STATE_STOPPED, "Emergency stop requested");
        Serial.println("[Motion] EMERGENCY STOP TRIGGERED!");
        xSemaphoreGive(mutex);
    }
}

void MotionController::setZero() {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        stepper.setCurrentPosition(0);
        targetAngleDeg = 0.0f;
        isHomed = true;
        setState(STATE_IDLE);
        Serial.println("[Motion] Position reset to 0.0°");
        xSemaphoreGive(mutex);
    }
}

MotionStatus MotionController::getStatus() {
    MotionStatus s;
    memset(&s, 0, sizeof(s));

    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        s.state = state;
        s.currentAngle = stepsToDeg(stepper.currentPosition());
        s.targetAngle = (state == STATE_MOVING) ? targetAngleDeg : s.currentAngle;
        s.currentSpeed = requestedSpeedDegS;
        s.isHomed = isHomed;
        s.endstopTriggered = isEndstopPressed();
        strncpy(s.errorMessage, lastError, sizeof(s.errorMessage) - 1);
        xSemaphoreGive(mutex);
    } else {
        s.state = state;
        s.currentAngle = 0.0f;
        s.targetAngle = 0.0f;
        s.currentSpeed = 0.0f;
        s.isHomed = isHomed;
        s.endstopTriggered = isEndstopPressed();
        strncpy(s.errorMessage, "Busy", sizeof(s.errorMessage) - 1);
    }

    switch (s.state) {
        case STATE_IDLE:    s.stateStr = "IDLE"; break;
        case STATE_HOMING:  s.stateStr = "HOMING"; break;
        case STATE_MOVING:  s.stateStr = "MOVING"; break;
        case STATE_STOPPED: s.stateStr = "STOPPED"; break;
        case STATE_ERROR:   s.stateStr = "ERROR"; break;
        default:            s.stateStr = "UNKNOWN"; break;
    }

    return s;
}

void MotionController::motionTaskEntry(void* parameter) {
    MotionController* self = static_cast<MotionController*>(parameter);
    self->motionLoop();
}

void MotionController::motionLoop() {
    uint32_t lastYieldTime = millis();

    while (true) {
        if (xSemaphoreTake(mutex, 0) == pdTRUE) {
            switch (state) {
                case STATE_HOMING: {
                    // Check safety timeout
                    if ((millis() - homingStartTime) > (HOMING_TIMEOUT_SEC * 1000UL)) {
                        stepper.stop();
                        setState(STATE_ERROR, "Homing timeout: Endstop not triggered within limit");
                        Serial.println("[Motion] Error: Homing timeout!");
                        break;
                    }

                    if (homingStep == HOME_FAST_APPROACH) {
                        if (isEndstopPressed()) {
                            // Hit endstop on fast approach
                            stepper.setCurrentPosition(0);
                            long backoffSteps = -HOMING_DIRECTION * degToSteps(HOMING_BACKOFF_DEG);
                            stepper.setMaxSpeed(HOMING_SPEED_FAST_DEG_S * STEPS_PER_DEGREE);
                            stepper.setAcceleration(DEFAULT_ACCEL_DEG_S2 * STEPS_PER_DEGREE);
                            stepper.moveTo(backoffSteps);
                            homingStep = HOME_BACKOFF;
                            Serial.println("[Motion] Fast approach hit endstop, backing off...");
                        } else {
                            float speedSteps = HOMING_DIRECTION * HOMING_SPEED_FAST_DEG_S * STEPS_PER_DEGREE;
                            stepper.setSpeed(speedSteps);
                            stepper.runSpeed();
                        }
                    } else if (homingStep == HOME_BACKOFF) {
                        stepper.run();
                        if (stepper.distanceToGo() == 0) {
                            homingStep = HOME_SLOW_APPROACH;
                            Serial.println("[Motion] Backoff complete, slow approach starting...");
                        }
                    } else if (homingStep == HOME_SLOW_APPROACH) {
                        if (isEndstopPressed()) {
                            // Precise endstop hit!
                            stepper.setCurrentPosition(0);
                            isHomed = true;
                            targetAngleDeg = 0.0f;
                            setState(STATE_IDLE);
                            Serial.println("[Motion] Homing successfully completed! Origin 0.0° set.");
                        } else {
                            float speedSteps = HOMING_DIRECTION * HOMING_SPEED_SLOW_DEG_S * STEPS_PER_DEGREE;
                            stepper.setSpeed(speedSteps);
                            stepper.runSpeed();
                        }
                    }
                    break;
                }

                case STATE_MOVING: {
                    stepper.run();
                    if (stepper.distanceToGo() == 0) {
                        setState(STATE_IDLE);
                        Serial.println("[Motion] Target position reached.");
                    }
                    break;
                }

                case STATE_IDLE:
                case STATE_STOPPED:
                case STATE_ERROR:
                default:
                    break;
            }

            xSemaphoreGive(mutex);
        }

        // Periodic yield to feed watchdog and allow FreeRTOS context switching
        if (state == STATE_MOVING || state == STATE_HOMING) {
            // High rate execution with small yield every ~2ms
            if (millis() - lastYieldTime >= 2) {
                lastYieldTime = millis();
                taskYIELD();
            }
        } else {
            // Idle state: sleep for 10ms to save CPU
            vTaskDelay(pdMS_TO_TICKS(10));
            lastYieldTime = millis();
        }
    }
}

#include "tilt_controller.h"
#include "tilt_imu.h"
#include <Preferences.h>

static const char* NVS_TILT_CFG_NS = "tilt_cfg";
TiltController tiltCtrl;

TiltController::TiltController()
    : stepper(AccelStepper::DRIVER, 18, 19),
      motionTaskHandle(nullptr), mutex(nullptr),
      currentStepsPerDegree(1.0f),
      state(TILT_STATE_IDLE), isHomed(false),
      targetAngleDeg(0.0f), requestedSpeedDegS(0.0f),
      holdActive(false), holdTargetDeg(0.0f),
      endstopTriggerStartTime(0),
      homingStep(THOME_FAST), homingStartTime(0) {
    memset(lastError, 0, sizeof(lastError));
}

TiltController::~TiltController() {
    if (motionTaskHandle) {
        vTaskDelete(motionTaskHandle);
    }
    if (mutex) {
        vSemaphoreDelete(mutex);
    }
}

void TiltController::loadConfig() {
    Preferences prefs;
    if (!prefs.begin(NVS_TILT_CFG_NS, false)) return;

    cfg.pinStep = prefs.getInt("step", cfg.pinStep);
    cfg.pinDir = prefs.getInt("dir", cfg.pinDir);
    cfg.pinEnable = prefs.getInt("enable", cfg.pinEnable);
    cfg.stepActiveLow = prefs.getBool("step_low", cfg.stepActiveLow);
    cfg.dirPositiveHigh = prefs.getBool("dir_high", cfg.dirPositiveHigh);
    cfg.enableActiveHigh = prefs.getBool("ena_high", cfg.enableActiveHigh);

    cfg.pinEndstop = prefs.getInt("endstop", cfg.pinEndstop);
    cfg.endstopInverted = prefs.getBool("endstop_inv", cfg.endstopInverted);
    cfg.endstopDebounceMs = prefs.getUInt("debounce", cfg.endstopDebounceMs);

    cfg.pinSda = prefs.getInt("sda", cfg.pinSda);
    cfg.pinScl = prefs.getInt("scl", cfg.pinScl);
    cfg.mpuAddress = prefs.getUChar("mpu_addr", cfg.mpuAddress);
    cfg.tiltAxis = prefs.getInt("axis", cfg.tiltAxis);
    cfg.holdDeadband = prefs.getFloat("deadband", cfg.holdDeadband);
    cfg.holdKp = prefs.getFloat("hold_kp", cfg.holdKp);

    cfg.stepsPerRev = prefs.getFloat("spr", cfg.stepsPerRev);
    cfg.microsteps = prefs.getFloat("micro", cfg.microsteps);
    cfg.gearRatio = prefs.getFloat("gear", cfg.gearRatio);
    const uint8_t kinematicsVersion = prefs.getUChar("kin_ver", 0);
    if (kinematicsVersion == 0 && cfg.gearRatio == 4.0f) {
        cfg.gearRatio = 3.0f;
    }
    prefs.putFloat("gear", cfg.gearRatio);
    prefs.putUChar("kin_ver", 1);

    cfg.defaultSpeed = prefs.getFloat("def_spd", cfg.defaultSpeed);
    cfg.maxSpeed = prefs.getFloat("max_spd", cfg.maxSpeed);
    cfg.acceleration = prefs.getFloat("accel", cfg.acceleration);
    cfg.minAngle = prefs.getFloat("min_ang", cfg.minAngle);
    cfg.maxAngle = prefs.getFloat("max_ang", cfg.maxAngle);

    cfg.homingDirection = prefs.getInt("h_dir", cfg.homingDirection);
    cfg.homingFastSpeed = prefs.getFloat("h_fast", cfg.homingFastSpeed);
    cfg.homingSlowSpeed = prefs.getFloat("h_slow", cfg.homingSlowSpeed);
    cfg.homingBackoffDeg = prefs.getFloat("h_back", cfg.homingBackoffDeg);
    cfg.autoHomeOnBoot = prefs.getBool("autohome", cfg.autoHomeOnBoot);

    prefs.end();
}

void TiltController::saveConfig() {
    Preferences prefs;
    if (!prefs.begin(NVS_TILT_CFG_NS, false)) return;

    prefs.putInt("step", cfg.pinStep);
    prefs.putInt("dir", cfg.pinDir);
    prefs.putInt("enable", cfg.pinEnable);
    prefs.putBool("step_low", cfg.stepActiveLow);
    prefs.putBool("dir_high", cfg.dirPositiveHigh);
    prefs.putBool("ena_high", cfg.enableActiveHigh);

    prefs.putInt("endstop", cfg.pinEndstop);
    prefs.putBool("endstop_inv", cfg.endstopInverted);
    prefs.putUInt("debounce", cfg.endstopDebounceMs);

    prefs.putInt("sda", cfg.pinSda);
    prefs.putInt("scl", cfg.pinScl);
    prefs.putUChar("mpu_addr", cfg.mpuAddress);
    prefs.putInt("axis", cfg.tiltAxis);
    prefs.putFloat("deadband", cfg.holdDeadband);
    prefs.putFloat("hold_kp", cfg.holdKp);

    prefs.putFloat("spr", cfg.stepsPerRev);
    prefs.putFloat("micro", cfg.microsteps);
    prefs.putFloat("gear", cfg.gearRatio);
    prefs.putUChar("kin_ver", 1);

    prefs.putFloat("def_spd", cfg.defaultSpeed);
    prefs.putFloat("max_spd", cfg.maxSpeed);
    prefs.putFloat("accel", cfg.acceleration);
    prefs.putFloat("min_ang", cfg.minAngle);
    prefs.putFloat("max_ang", cfg.maxAngle);

    prefs.putInt("h_dir", cfg.homingDirection);
    prefs.putFloat("h_fast", cfg.homingFastSpeed);
    prefs.putFloat("h_slow", cfg.homingSlowSpeed);
    prefs.putFloat("h_back", cfg.homingBackoffDeg);
    prefs.putBool("autohome", cfg.autoHomeOnBoot);

    prefs.end();
}

long TiltController::degToSteps(float deg) const {
    return lroundf(deg * currentStepsPerDegree);
}

float TiltController::stepsToDeg(long steps) const {
    if (currentStepsPerDegree == 0.0f) return 0.0f;
    return static_cast<float>(steps) / currentStepsPerDegree;
}

void TiltController::setState(TiltState newState, const char* errorMsg) {
    state = newState;
    if (errorMsg) {
        strncpy(lastError, errorMsg, sizeof(lastError) - 1);
        lastError[sizeof(lastError) - 1] = '\0';
    } else if (newState == TILT_STATE_IDLE) {
        lastError[0] = '\0';
    }
}

bool TiltController::isEndstopPressed() {
    if (cfg.pinEndstop < 0) return false;
    const int raw = digitalRead(cfg.pinEndstop);
    const bool active = cfg.endstopInverted ? (raw == HIGH) : (raw == LOW);

    if (active) {
        if (endstopTriggerStartTime == 0) {
            endstopTriggerStartTime = millis();
        }
        if (millis() - endstopTriggerStartTime >= cfg.endstopDebounceMs) {
            return true;
        }
    } else {
        endstopTriggerStartTime = 0;
    }
    return false;
}

void TiltController::applyKinematics() {
    currentStepsPerDegree = cfg.getStepsPerDegree();
    if (currentStepsPerDegree <= 0.0f) currentStepsPerDegree = 1.0f;

    stepper.setMaxSpeed(cfg.maxSpeed * currentStepsPerDegree);
    stepper.setAcceleration(cfg.acceleration * currentStepsPerDegree);
}

bool TiltController::begin() {
    loadConfig();

    mutex = xSemaphoreCreateMutex();
    if (!mutex) {
        Serial.println("[Tilt Ctrl] Помилка створення м'ютексу!");
        return false;
    }

    // Налаштування піна кінцевика
    if (cfg.pinEndstop >= 0) {
        pinMode(cfg.pinEndstop, INPUT_PULLUP);
    }

    // Налаштування крокового двигуна через оптоізольований драйвер (Active-LOW)
    stepper = AccelStepper(AccelStepper::DRIVER, cfg.pinStep, cfg.pinDir);
    stepper.setPinsInverted(!cfg.dirPositiveHigh, cfg.stepActiveLow, !cfg.enableActiveHigh);

    if (cfg.pinEnable >= 0) {
        stepper.setEnablePin(cfg.pinEnable);
        stepper.enableOutputs();
    }

    applyKinematics();

    // Запуск фонового високопріоритетного завдання FreeRTOS для генерації імпульсів
    BaseType_t result = xTaskCreatePinnedToCore(
        motionTaskEntry,
        "TiltMotionTask",
        4096,
        this,
        configMAX_PRIORITIES - 1,
        &motionTaskHandle,
        1
    );

    if (result != pdPASS) {
        Serial.println("[Tilt Ctrl] Помилка запуску фонового завдання FreeRTOS!");
        return false;
    }

    Serial.printf("[Tilt Ctrl] Ініціалізація успішна: STEP=%d, DIR=%d, ENA=%d (StepLow=%d, EnaHigh=%d)\n",
                  cfg.pinStep, cfg.pinDir, cfg.pinEnable, cfg.stepActiveLow, cfg.enableActiveHigh);
    return true;
}

void TiltController::setDriverEnabled(bool enable) {
    if (cfg.pinEnable < 0) return;
    if (enable) {
        stepper.enableOutputs();
    } else {
        stepper.disableOutputs();
    }
}

bool TiltController::moveTo(float angleDeg, float speedDegS, bool relative) {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;

    if (state == TILT_STATE_HOMING) {
        xSemaphoreGive(mutex);
        return false;
    }

    holdActive = false; // Звичайний рух вимикає активне утримання за гіроскопом

    float finalAngle = relative ? (stepsToDeg(stepper.currentPosition()) + angleDeg) : angleDeg;

    // Обмеження допустимих кутів нахилу
    finalAngle = constrain(finalAngle, cfg.minAngle, cfg.maxAngle);

    targetAngleDeg = finalAngle;
    requestedSpeedDegS = (speedDegS > 0.0f) ? min(speedDegS, cfg.maxSpeed) : cfg.defaultSpeed;

    stepper.setMaxSpeed(requestedSpeedDegS * currentStepsPerDegree);
    stepper.moveTo(degToSteps(targetAngleDeg));

    setState(TILT_STATE_MOVING);
    xSemaphoreGive(mutex);
    return true;
}

bool TiltController::startHoming() {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) != pdTRUE) return false;

    if (state == TILT_STATE_HOMING) {
        xSemaphoreGive(mutex);
        return false;
    }

    holdActive = false;
    homingStep = THOME_FAST;
    homingStartTime = millis();

    // Задаємо рух у напрямку кінцевика з великим запасом
    stepper.setMaxSpeed(cfg.homingFastSpeed * currentStepsPerDegree);
    const long travel = degToSteps(fabs(cfg.maxAngle - cfg.minAngle) * 2.0f);
    stepper.moveTo(stepper.currentPosition() + (cfg.homingDirection * travel));

    setState(TILT_STATE_HOMING);
    xSemaphoreGive(mutex);
    Serial.println("[Tilt Ctrl] Запуск процедури калібрування до крайньої точки (Homing)...");
    return true;
}

void TiltController::emergencyStop() {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        holdActive = false;
        stepper.stop();
        setState(TILT_STATE_STOPPED, "Зупинено користувачем");
        xSemaphoreGive(mutex);
    }
}

void TiltController::setZero() {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        stepper.setCurrentPosition(0);
        targetAngleDeg = 0.0f;
        isHomed = true;
        setState(TILT_STATE_IDLE);
        xSemaphoreGive(mutex);
        Serial.println("[Tilt Ctrl] Позицію крокового двигуна встановлено в 0.0°");
    }
}

bool TiltController::resetGyroZero() {
    bool ok = tiltImu.zeroOrientation();
    if (ok) {
        if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            holdTargetDeg = 0.0f;
            xSemaphoreGive(mutex);
        }
    }
    return ok;
}

bool TiltController::calibrateGyro() {
    return tiltImu.calibrateGyro(500);
}

void TiltController::setHoldActive(bool active, float targetAngle) {
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        holdActive = active;
        holdTargetDeg = targetAngle;
        if (active && state != TILT_STATE_HOMING) {
            setState(TILT_STATE_HOLDING);
        } else if (!active && state == TILT_STATE_HOLDING) {
            setState(TILT_STATE_IDLE);
        }
        xSemaphoreGive(mutex);
    }
}

void TiltController::motionTaskEntry(void* parameter) {
    TiltController* self = static_cast<TiltController*>(parameter);
    for (;;) {
        self->motionLoop();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void TiltController::motionLoop() {
    if (xSemaphoreTake(mutex, 0) != pdTRUE) return;

    const bool endstopPressed = isEndstopPressed();

    // 1. Обробка процедури Homing
    if (state == TILT_STATE_HOMING) {
        if (millis() - homingStartTime > 45000) { // 45 секунд таймаут
            stepper.stop();
            setState(TILT_STATE_ERROR, "Таймаут пошуку крайньої точки");
            xSemaphoreGive(mutex);
            return;
        }

        switch (homingStep) {
            case THOME_FAST:
                if (endstopPressed) {
                    stepper.setCurrentPosition(0);
                    // Відкат від кінцевика у зворотному напрямку
                    homingStep = THOME_BACKOFF;
                    stepper.setMaxSpeed(cfg.homingSlowSpeed * currentStepsPerDegree);
                    stepper.moveTo(degToSteps(-cfg.homingDirection * cfg.homingBackoffDeg));
                } else {
                    stepper.run();
                }
                break;

            case THOME_BACKOFF:
                stepper.run();
                if (stepper.distanceToGo() == 0) {
                    // Повільний повторний точний підхід
                    homingStep = THOME_SLOW;
                    stepper.setMaxSpeed(cfg.homingSlowSpeed * currentStepsPerDegree);
                    stepper.moveTo(degToSteps(cfg.homingDirection * (cfg.homingBackoffDeg * 2.0f)));
                }
                break;

            case THOME_SLOW:
                if (endstopPressed) {
                    stepper.stop();
                    // Встановлюємо координату як крайню точку або 0
                    stepper.setCurrentPosition(degToSteps(cfg.minAngle));
                    targetAngleDeg = cfg.minAngle;
                    isHomed = true;
                    homingStep = THOME_COMPLETE;
                    setState(TILT_STATE_IDLE);
                    Serial.println("[Tilt Ctrl] Калібрування крайньої точки успішно завершено!");
                } else {
                    stepper.run();
                }
                break;

            case THOME_COMPLETE:
                break;
        }
        xSemaphoreGive(mutex);
        return;
    }

    // 2. Безпека: перевірка кінцевика під час звичайного руху
    if (state == TILT_STATE_MOVING && endstopPressed) {
        // Якщо рух відбувається у бік кінцевика — негайна зупинка
        const long dist = stepper.distanceToGo();
        if ((cfg.homingDirection < 0 && dist < 0) || (cfg.homingDirection > 0 && dist > 0)) {
            stepper.stop();
            setState(TILT_STATE_STOPPED, "Спрацював кінцевик крайньої точки!");
            xSemaphoreGive(mutex);
            return;
        }
    }

    // 3. Звичайний рух
    if (state == TILT_STATE_MOVING) {
        stepper.run();
        if (stepper.distanceToGo() == 0) {
            setState(TILT_STATE_IDLE);
        }
    } else if (state == TILT_STATE_HOLDING) {
        stepper.run();
    }

    xSemaphoreGive(mutex);
}

void TiltController::update() {
    tiltImu.update();

    // 4. Закритий контур: Підтримання положення за гіроскопом (Stabilization Loop)
    if (holdActive && (state == TILT_STATE_IDLE || state == TILT_STATE_HOLDING)) {
        if (!tiltImu.isConnected()) return;

        const float currentAngle = tiltImu.getTiltAngle();
        const float error = holdTargetDeg - currentAngle;

        if (fabs(error) > cfg.holdDeadband) {
            if (xSemaphoreTake(mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                // Коригуємо позицію крокового двигуна відповідно до похибки гіроскопа
                const float correctionSpeed = constrain(fabs(error) * cfg.holdKp + 0.5f, 0.5f, cfg.maxSpeed);
                stepper.setMaxSpeed(correctionSpeed * currentStepsPerDegree);

                const long stepCorr = degToSteps(error * 0.4f);
                if (stepCorr != 0) {
                    stepper.moveTo(stepper.currentPosition() + stepCorr);
                    setState(TILT_STATE_HOLDING);
                }
                xSemaphoreGive(mutex);
            }
        }
    }
}

TiltStatus TiltController::getStatus() {
    TiltStatus st;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        st.state = static_cast<int>(state);
        st.stateStr = tiltStateToString(state);
        st.motorAngle = stepsToDeg(stepper.currentPosition());
        st.targetAngle = targetAngleDeg;
        st.currentSpeed = (currentStepsPerDegree > 0.0f) ? (fabs(stepper.speed()) / currentStepsPerDegree) : 0.0f;
        st.isHomed = isHomed;
        st.endstopTriggered = isEndstopPressed();
        st.holdActive = holdActive;
        strncpy(st.errorMessage, lastError, sizeof(st.errorMessage) - 1);
        xSemaphoreGive(mutex);
    } else {
        st.stateStr = "BUSY";
    }

    st.gyroConnected = tiltImu.isConnected();
    st.gyroAngle = tiltImu.getTiltAngle();
    st.pitchDeg = tiltImu.getPitch();
    st.rollDeg = tiltImu.getRoll();
    return st;
}

TiltHardwareConfig TiltController::getConfig() const {
    return cfg;
}

bool TiltController::applyConfig(const TiltHardwareConfig& newCfg, bool& rebootRequired) {
    rebootRequired = (newCfg.pinStep != cfg.pinStep ||
                      newCfg.pinDir != cfg.pinDir ||
                      newCfg.pinEnable != cfg.pinEnable ||
                      newCfg.pinEndstop != cfg.pinEndstop ||
                      newCfg.pinSda != cfg.pinSda ||
                      newCfg.pinScl != cfg.pinScl);

    cfg = newCfg;
    saveConfig();

    if (!rebootRequired) {
        applyKinematics();
        tiltImu.setAxis(cfg.tiltAxis);
    }
    return true;
}

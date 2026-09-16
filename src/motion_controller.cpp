#include "motion_controller.h"
#include <Preferences.h>

static const char *NVS_HW_NAMESPACE = "table_hw";

MotionController motionCtrl;

MotionController::MotionController()
    : stepper(AccelStepper::DRIVER, PIN_STEP, PIN_DIR),
      motionTaskHandle(nullptr), mutex(nullptr),
      currentStepsPerDegree(STEPS_PER_DEGREE), state(STATE_IDLE),
      isHomed(false), targetAngleDeg(0.0f),
      requestedSpeedDegS(DEFAULT_SPEED_DEG_S), endstopTriggerStartTime(0),
      homingStep(HOME_FAST_APPROACH), homingStartTime(0) {
  memset(lastError, 0, sizeof(lastError));

  // Початкові налаштування за замовчуванням
  cfg.pinStep = PIN_STEP;
  cfg.pinDir = PIN_DIR;
  cfg.pinEnable = PIN_ENABLE;
  cfg.pinEndstop = PIN_ENDSTOP;

  cfg.invertDir = INVERT_DIR;
  cfg.endstopInverted = DEFAULT_ENDSTOP_INVERTED;
  cfg.endstopDebounceMs = DEFAULT_ENDSTOP_DEBOUNCE_MS;

  cfg.motorTeeth = 20.0f;
  cfg.tableTeeth = 60.0f;
  cfg.stepsPerRev = STEPS_PER_MOTOR_REV;
  cfg.microsteps = MICROSTEPS;

  cfg.defaultSpeed = DEFAULT_SPEED_DEG_S;
  cfg.maxSpeed = MAX_SPEED_DEG_S;
  cfg.acceleration = DEFAULT_ACCEL_DEG_S2;

  cfg.homingDirection = HOMING_DIRECTION;
  cfg.autoHomeOnBoot = AUTO_HOME_ON_BOOT;
}

MotionController::~MotionController() {
  if (motionTaskHandle != nullptr) {
    vTaskDelete(motionTaskHandle);
  }
  if (mutex != nullptr) {
    vSemaphoreDelete(mutex);
  }
}

void MotionController::loadConfig() {
  Preferences prefs;
  if (prefs.begin(NVS_HW_NAMESPACE, true)) {
    cfg.pinStep = prefs.getInt("p_step", PIN_STEP);
    cfg.pinDir = prefs.getInt("p_dir", PIN_DIR);
    cfg.pinEnable = prefs.getInt("p_en", PIN_ENABLE);
    cfg.pinEndstop = prefs.getInt("p_es", PIN_ENDSTOP);

    cfg.invertDir = prefs.getBool("inv_dir", INVERT_DIR);
    cfg.endstopInverted = prefs.getBool("es_inv", DEFAULT_ENDSTOP_INVERTED);
    cfg.endstopDebounceMs =
        prefs.getUInt("es_deb", DEFAULT_ENDSTOP_DEBOUNCE_MS);

    cfg.motorTeeth = prefs.getFloat("m_teeth", 20.0f);
    cfg.tableTeeth = prefs.getFloat("t_teeth", 60.0f);
    cfg.stepsPerRev = prefs.getFloat("m_steps", STEPS_PER_MOTOR_REV);
    cfg.microsteps = prefs.getFloat("micro", MICROSTEPS);

    cfg.defaultSpeed = prefs.getFloat("def_spd", DEFAULT_SPEED_DEG_S);
    cfg.maxSpeed = prefs.getFloat("max_spd", MAX_SPEED_DEG_S);
    cfg.acceleration = prefs.getFloat("accel", DEFAULT_ACCEL_DEG_S2);

    cfg.homingDirection = prefs.getInt("home_dir", HOMING_DIRECTION);
    cfg.autoHomeOnBoot = prefs.getBool("boot_home", AUTO_HOME_ON_BOOT);

    prefs.end();
  }
  applyKinematics();
}

void MotionController::saveConfig() {
  Preferences prefs;
  if (prefs.begin(NVS_HW_NAMESPACE, false)) {
    prefs.putInt("p_step", cfg.pinStep);
    prefs.putInt("p_dir", cfg.pinDir);
    prefs.putInt("p_en", cfg.pinEnable);
    prefs.putInt("p_es", cfg.pinEndstop);

    prefs.putBool("inv_dir", cfg.invertDir);
    prefs.putBool("es_inv", cfg.endstopInverted);
    prefs.putUInt("es_deb", cfg.endstopDebounceMs);

    prefs.putFloat("m_teeth", cfg.motorTeeth);
    prefs.putFloat("t_teeth", cfg.tableTeeth);
    prefs.putFloat("m_steps", cfg.stepsPerRev);
    prefs.putFloat("micro", cfg.microsteps);

    prefs.putFloat("def_spd", cfg.defaultSpeed);
    prefs.putFloat("max_spd", cfg.maxSpeed);
    prefs.putFloat("accel", cfg.acceleration);

    prefs.putInt("home_dir", cfg.homingDirection);
    prefs.putBool("boot_home", cfg.autoHomeOnBoot);

    prefs.end();
  }
}

void MotionController::applyKinematics() {
  currentStepsPerDegree = cfg.getStepsPerDegree();
  if (currentStepsPerDegree <= 0.0f) {
    currentStepsPerDegree = 1.0f;
  }

  stepper.setPinsInverted(cfg.invertDir, false, false);
  stepper.setMaxSpeed(cfg.maxSpeed * currentStepsPerDegree);
  stepper.setAcceleration(cfg.acceleration * currentStepsPerDegree);
}

HardwareConfig MotionController::getConfig() const { return cfg; }

bool MotionController::applyConfig(const HardwareConfig &newCfg,
                                   bool &rebootRequired) {
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
    return false;
  }

  // Зміна пінів вимагає перезавантаження мікроконтролера
  rebootRequired =
      (newCfg.pinStep != cfg.pinStep || newCfg.pinDir != cfg.pinDir ||
       newCfg.pinEnable != cfg.pinEnable ||
       newCfg.pinEndstop != cfg.pinEndstop);

  cfg = newCfg;
  saveConfig();
  applyKinematics();

  xSemaphoreGive(mutex);
  Serial.println(
      "[Motion] Нову апаратну конфігурацію успішно збережено в NVS.");
  return true;
}

float MotionController::getStepsPerDegree() const {
  return currentStepsPerDegree;
}

bool MotionController::begin() {
  mutex = xSemaphoreCreateMutex();
  if (mutex == nullptr) {
    Serial.println("[Motion] Помилка: Не вдалося створити м'ютекс!");
    return false;
  }

  // Завантаження конфігурації з NVS
  loadConfig();

  // Налаштування піна кінцевика
  pinMode(cfg.pinEndstop, ENDSTOP_PULLUP ? INPUT_PULLUP : INPUT);

  // Налаштування піна Enable драйвера
  if (cfg.pinEnable >= 0) {
    pinMode(cfg.pinEnable, OUTPUT);
    setDriverEnabled(true);
  }

  // Налаштування AccelStepper
  applyKinematics();
  stepper.setCurrentPosition(0);

  Serial.printf("[Motion] Ініціалізація успішна.\n");
  Serial.printf(
      "         Шестерні: Мотор=%d з., Стіл=%d з. (Редукція: %.2f:1)\n",
      (int)cfg.motorTeeth, (int)cfg.tableTeeth, cfg.getGearRatio());
  Serial.printf("         Кроків на 1°: %.3f\n", currentStepsPerDegree);
  Serial.printf(
      "         Піни: STEP=%d, DIR=%d (інверсія: %s), EN=%d, ENDSTOP=%d\n",
      cfg.pinStep, cfg.pinDir, cfg.invertDir ? "ТАК" : "НІ", cfg.pinEnable,
      cfg.pinEndstop);

  // Запуск фонового завдання на CORE 0.
  // Core 1 повністю вивільняється для миттєвої обробки HTTP-запитів
  // веб-сервера.
  BaseType_t res = xTaskCreatePinnedToCore(
      motionTaskEntry, "MotionTask", 4096, this,
      4, // Пріоритет 4 (вище IDLE, нижче tcpip_task на Core 0)
      &motionTaskHandle,
      0 // Core 0
  );

  return (res == pdPASS);
}

void MotionController::setDriverEnabled(bool enable) {
  if (cfg.pinEnable >= 0) {
    bool pinLevel = enable ? (ENABLE_ACTIVE_LOW ? LOW : HIGH)
                           : (ENABLE_ACTIVE_LOW ? HIGH : LOW);
    digitalWrite(cfg.pinEnable, pinLevel);
  }
}

bool MotionController::isEndstopPressed() {
  int val = digitalRead(cfg.pinEndstop);
  bool rawActive = cfg.endstopInverted ? (val == HIGH) : (val == LOW);

  if (cfg.endstopDebounceMs == 0) {
    return rawActive;
  }

  uint32_t now = millis();
  if (rawActive) {
    if (endstopTriggerStartTime == 0) {
      endstopTriggerStartTime = now;
    }
    if (now - endstopTriggerStartTime >= cfg.endstopDebounceMs) {
      return true;
    }
    return false;
  } else {
    endstopTriggerStartTime = 0;
    return false;
  }
}

long MotionController::degToSteps(float deg) const {
  return lroundf(deg * currentStepsPerDegree);
}

float MotionController::stepsToDeg(long steps) const {
  if (currentStepsPerDegree <= 0.0f)
    return 0.0f;
  return (float)steps / currentStepsPerDegree;
}

void MotionController::setState(MotionState newState, const char *errorMsg) {
  state = newState;
  if (errorMsg != nullptr) {
    strncpy(lastError, errorMsg, sizeof(lastError) - 1);
    lastError[sizeof(lastError) - 1] = '\0';
  } else if (newState != STATE_ERROR) {
    lastError[0] = '\0';
  }
}

bool MotionController::startHoming() {
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50)) != pdTRUE) {
    return false;
  }

  if (state == STATE_HOMING) {
    xSemaphoreGive(mutex);
    return true;
  }

  Serial.println("[Motion] Запуск процедури калібрування (Homing)...");
  setDriverEnabled(true);

  homingStartTime = millis();
  homingStep = HOME_FAST_APPROACH;
  setState(STATE_HOMING);

  if (isEndstopPressed()) {
    long backoffSteps = -cfg.homingDirection * degToSteps(HOMING_BACKOFF_DEG);
    stepper.setCurrentPosition(0);
    stepper.setMaxSpeed(HOMING_SPEED_FAST_DEG_S * currentStepsPerDegree);
    stepper.setAcceleration(cfg.acceleration * currentStepsPerDegree);
    stepper.moveTo(backoffSteps);
    homingStep = HOME_BACKOFF;
  }

  xSemaphoreGive(mutex);
  return true;
}

bool MotionController::moveTo(float angleDeg, float speedDegS, bool relative) {
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50)) != pdTRUE) {
    return false;
  }

  if (state == STATE_HOMING) {
    xSemaphoreGive(mutex);
    return false;
  }

  setDriverEnabled(true);

  if (speedDegS <= 0.0f) {
    speedDegS = cfg.defaultSpeed;
  }
  speedDegS = constrain(speedDegS, 1.0f, cfg.maxSpeed);
  requestedSpeedDegS = speedDegS;

  stepper.setMaxSpeed(speedDegS * currentStepsPerDegree);
  stepper.setAcceleration(cfg.acceleration * currentStepsPerDegree);

  long targetSteps = 0;
  if (relative) {
    targetSteps = stepper.currentPosition() + degToSteps(angleDeg);
    targetAngleDeg = stepsToDeg(targetSteps);
  } else {
    targetSteps = degToSteps(angleDeg);
    targetAngleDeg = angleDeg;
  }

  stepper.moveTo(targetSteps);
  setState(STATE_MOVING);

  Serial.printf(
      "[Motion] Команда повороту: ціль=%.1f°, швидкість=%.1f°/с, відносно=%d\n",
      targetAngleDeg, speedDegS, relative);

  xSemaphoreGive(mutex);
  return true;
}

void MotionController::emergencyStop() {
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    stepper.stop();
    stepper.moveTo(stepper.currentPosition());
    setState(STATE_STOPPED);
    Serial.println("[Motion] Аварійна зупинка! Двигун зупинено.");
    xSemaphoreGive(mutex);
  }
}

void MotionController::setZero() {
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    stepper.setCurrentPosition(0);
    targetAngleDeg = 0.0f;
    isHomed = true;
    setState(STATE_IDLE);
    Serial.println("[Motion] Позицію скинуто в 0.0°");
    xSemaphoreGive(mutex);
  }
}

MotionStatus MotionController::getStatus() {
  MotionStatus s;
  memset(&s, 0, sizeof(s));

  // Миттєве атомарне зчитування для плавного відображення в реальному часі
  s.state = state;
  s.currentAngle = stepsToDeg(stepper.currentPosition());
  s.targetAngle = (state == STATE_MOVING) ? targetAngleDeg : s.currentAngle;
  s.currentSpeed = requestedSpeedDegS;
  s.isHomed = isHomed;
  s.endstopTriggered = isEndstopPressed();
  strncpy(s.errorMessage, lastError, sizeof(s.errorMessage) - 1);

  switch (s.state) {
  case STATE_IDLE:
    s.stateStr = "IDLE";
    break;
  case STATE_HOMING:
    s.stateStr = "HOMING";
    break;
  case STATE_MOVING:
    s.stateStr = "MOVING";
    break;
  case STATE_STOPPED:
    s.stateStr = "STOPPED";
    break;
  case STATE_ERROR:
    s.stateStr = "ERROR";
    break;
  default:
    s.stateStr = "UNKNOWN";
    break;
  }

  return s;
}

void MotionController::motionTaskEntry(void *parameter) {
  MotionController *self = static_cast<MotionController *>(parameter);
  self->motionLoop();
}

void MotionController::motionLoop() {
  uint32_t lastYieldTime = millis();

  while (true) {
    if (state == STATE_MOVING) {
      stepper.run();
      if (stepper.distanceToGo() == 0) {
        setState(STATE_IDLE);
        Serial.println("[Motion] Цільову позицію досягнуто.");
      }
    } else if (state == STATE_HOMING) {
      if ((millis() - homingStartTime) > (HOMING_TIMEOUT_SEC * 1000UL)) {
        stepper.stop();
        setState(STATE_ERROR, "Таймаут калібрування: кінцевик не спрацював");
        Serial.println("[Motion] Помилка: Таймаут калібрування!");
      } else if (homingStep == HOME_FAST_APPROACH) {
        if (isEndstopPressed()) {
          stepper.setCurrentPosition(0);
          long backoffSteps =
              -cfg.homingDirection * degToSteps(HOMING_BACKOFF_DEG);
          stepper.setMaxSpeed(HOMING_SPEED_FAST_DEG_S * currentStepsPerDegree);
          stepper.setAcceleration(cfg.acceleration * currentStepsPerDegree);
          stepper.moveTo(backoffSteps);
          homingStep = HOME_BACKOFF;
          Serial.println(
              "[Motion] Швидкий підхід торкнувся кінцевика, відкат...");
        } else {
          float speedSteps = cfg.homingDirection * HOMING_SPEED_FAST_DEG_S *
                             currentStepsPerDegree;
          stepper.setSpeed(speedSteps);
          stepper.runSpeed();
        }
      } else if (homingStep == HOME_BACKOFF) {
        stepper.run();
        if (stepper.distanceToGo() == 0) {
          homingStep = HOME_SLOW_APPROACH;
          Serial.println(
              "[Motion] Початок повільного точного підходу до кінцевика...");
        }
      } else if (homingStep == HOME_SLOW_APPROACH) {
        if (isEndstopPressed()) {
          stepper.stop();
          stepper.setCurrentPosition(0);
          targetAngleDeg = 0.0f;
          isHomed = true;
          setState(STATE_IDLE);
          Serial.println("[Motion] Точне торкання, калібрування завершено! "
                         "Позицію встановлено в 0.0°");
        } else {
          float speedSteps = cfg.homingDirection * HOMING_SPEED_SLOW_DEG_S *
                             currentStepsPerDegree;
          stepper.setSpeed(speedSteps);
          stepper.runSpeed();
        }
      }
    }

    // Керування паузами та скидання сторожового таймера на Core 0
    if (state == STATE_IDLE || state == STATE_STOPPED || state == STATE_ERROR) {
      vTaskDelay(pdMS_TO_TICKS(10));
      lastYieldTime = millis();
    } else {
      // Під час активного обертання робимо мікро-паузу 1 мс кожні 5 мс
      uint32_t now = millis();
      if (now - lastYieldTime >= 5) {
        lastYieldTime = now;
        vTaskDelay(1);
      }
    }
  }
}

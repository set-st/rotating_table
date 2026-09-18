#pragma once
#include <Arduino.h>
#include <AccelStepper.h>
#include "config.h"

// Стани контролера руху
enum MotionState {
    STATE_IDLE,    // Очікування команд
    STATE_HOMING,  // Процес пошуку кінцевика та калібрування
    STATE_MOVING,  // Обертання до цільового кута
    STATE_STOPPED, // Екстрена зупинка
    STATE_ERROR    // Помилка (наприклад, таймаут кінцевика)
};

// Знімок поточного стану для передачі через API
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

// Повна апаратна конфігурація столу (зберігається в NVS)
struct HardwareConfig {
    // Призначення пінів GPIO ESP32
    int pinStep;
    int pinDir;
    int pinEnable;
    int pinEndstop;
    int pinButtonLeft;
    int pinButtonRight;
    int pinButtonStop;

    // Напрямок та кінцевик
    bool invertDir;
    bool endstopInverted;
    uint32_t endstopDebounceMs;
    bool buttonLeftInverted;
    bool buttonRightInverted;
    bool buttonStopInverted;

    // Кінематика та зубчасті передачі
    float motorTeeth;   // Кількість зубів шестерні мотора (наприклад, 20)
    float tableTeeth;   // Кількість зубів шестерні столу (наприклад, 60)
    float stepsPerRev;  // Кроків на оберт двигуна (200 для 1.8°, 400 для 0.9°)
    float microsteps;   // Мікрокрок драйвера (1, 2, 4, 8, 16, 32...)

    // Швидкість та прискорення
    float defaultSpeed; // Стандартна швидкість (град/с)
    float maxSpeed;     // Максимальна швидкість (град/с)
    float acceleration; // Прискорення (град/с^2)

    // Керування апаратними кнопками
    float buttonMoveSpeed;
    float buttonMoveAngle;

    // Калібрування (Homing)
    int homingDirection; // -1 (вліво) або +1 (вправо)
    bool autoHomeOnBoot; // Автоматичний пошук нуля при увімкненні

    // Розрахунок передаточного числа: Зуби столу / Зуби мотора
    float getGearRatio() const {
        if (motorTeeth > 0.0f && tableTeeth > 0.0f) {
            return tableTeeth / motorTeeth;
        }
        return 1.0f;
    }

    // Розрахунок кількості імпульсів (кроків) на 1 градус повороту столу
    float getStepsPerDegree() const {
        float ratio = getGearRatio();
        return (stepsPerRev * microsteps * ratio) / 360.0f;
    }
};

class MotionController {
public:
    MotionController();
    ~MotionController();

    // Ініціалізація пінів, зчитування налаштувань та запуск фонового завдання FreeRTOS
    bool begin();

    // Запуск процедури пошуку кінцевика (калібрування нуля)
    bool startHoming();

    // Команда повороту столу на кут
    bool moveTo(float angleDeg, float speedDegS = 0.0f, bool relative = false);

    // Негайна аварійна зупинка двигуна
    void emergencyStop();

    // Встановлення поточної позиції як 0.0° без руху
    void setZero();

    // Увімкнення / вимкнення виходів драйвера двигуна (Enable)
    void setDriverEnabled(bool enable);

    // Отримання знімка поточного стану (атомарне та швидке читання для Web UI)
    MotionStatus getStatus();

    // Перевірка стану кінцевика з урахуванням інверсії та фільтрації брязкоту
    bool isEndstopPressed();

    // Отримання та застосування апаратних налаштувань
    HardwareConfig getConfig() const;
    bool applyConfig(const HardwareConfig& newCfg, bool& rebootRequired);

    float getStepsPerDegree() const;

private:
    AccelStepper stepper;
    TaskHandle_t motionTaskHandle;
    SemaphoreHandle_t mutex;

    HardwareConfig cfg;
    float currentStepsPerDegree;

    volatile MotionState state;
    volatile bool isHomed;
    volatile float targetAngleDeg;
    volatile float requestedSpeedDegS;
    char lastError[64];

    uint32_t endstopTriggerStartTime;
    bool previousButtonLeft;
    bool previousButtonRight;
    bool previousButtonStop;

    // Підкроки процедури калібрування
    enum HomingStep {
        HOME_FAST_APPROACH, // Швидкий підхід
        HOME_BACKOFF,       // Відкат назад
        HOME_SLOW_APPROACH, // Повільне точне торкання
        HOME_COMPLETE       // Завершено
    };
    HomingStep homingStep;
    uint32_t homingStartTime;

    // Функція фонового завдання FreeRTOS
    static void motionTaskEntry(void* parameter);
    void motionLoop();

    // Допоміжні функції перерахунку кроків і кутів
    long degToSteps(float deg) const;
    float stepsToDeg(long steps) const;
    void setState(MotionState newState, const char* errorMsg = nullptr);

    void loadConfig();
    void saveConfig();
    void applyKinematics();
    bool isButtonPressed(int pin, bool inverted) const;
    void handleHardwareButtons();
};

extern MotionController motionCtrl;

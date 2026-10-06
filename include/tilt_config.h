#pragma once
#include <Arduino.h>

// Версія прошивки платформи нахилу
#ifndef TILT_FIRMWARE_VERSION
#define TILT_FIRMWARE_VERSION "v1.0.0-tilt"
#endif

// =============================================================================
// КОНФІГУРАЦІЯ АПАРАТУРИ ПЛАТФОРМИ НАХИЛУ (ЗБЕРІГАЄТЬСЯ В NVS)
// =============================================================================
struct TiltHardwareConfig {
    // Піни драйвера крокового двигуна (оптоізольований)
    int pinStep = 18;
    int pinDir = 19;
    int pinEnable = 5;

    // Рівні сигналів:
    // Опторозв'язка зазвичай підключається зі спільним анодом (+5V/+3.3V на PUL+, DIR+, ENA+),
    // тому активні сигнали на пінах ESP32 (PUL-, DIR-, ENA-) передаються рівнем LOW!
    bool stepActiveLow = true;     // Імпульс кроку генерується низьким рівнем (LOW)
    bool dirPositiveHigh = false;  // LOW для одного напрямку, HIGH для іншого
    bool enableActiveHigh = false; // LOW вмикає драйвер (Enable Active LOW)

    // Кінцевий вимикач (крайня точка / endstop)
    int pinEndstop = 4;
    bool endstopInverted = false;  // false = Active LOW (NO контакт на GND із внутрішнім pull-up)
    uint32_t endstopDebounceMs = 10;

    // Гіроскоп GY-521 (MPU6050)
    int pinSda = 21;
    int pinScl = 22;
    uint8_t mpuAddress = 0x68;
    int tiltAxis = 0;              // 0 = Pitch (тангаж), 1 = Roll (крен)

    // Параметри підтримання кута (Closed-loop Stabilization)
    float holdDeadband = 0.2f;     // Зона нечутливості у градусах (±0.2°)
    float holdKp = 2.5f;           // Пропорційний коефіцієнт корекції швидкості

    // Кінематика
    float stepsPerRev = 200.0f;    // Кроків на оберт двигуна (1.8° = 200)
    float microsteps = 16.0f;      // Мікрокрок (16 = 1/16)
    float gearRatio = 3.0f;        // 60 зубів на платформі / 20 зубів на моторі

    // Швидкості та обмеження кутів (в градусах)
    float defaultSpeed = 5.0f;     // град/с
    float maxSpeed = 30.0f;        // град/с
    float acceleration = 25.0f;    // град/с^2
    float minAngle = -45.0f;       // Максимальний нахил вниз / назад (град)
    float maxAngle = 45.0f;        // Максимальний нахил вгору / вперед (град)

    // Калібрування до крайньої точки (Homing)
    int homingDirection = -1;      // -1 = рух до кінцевика, +1 = у протилежний бік
    float homingFastSpeed = 8.0f;  // град/с
    float homingSlowSpeed = 2.0f;  // град/с
    float homingBackoffDeg = 3.0f; // Відкат від кінцевика (град)
    bool autoHomeOnBoot = false;   // Автоматичний пошук нуля при запуску

    // Розрахунок кількості імпульсів (кроків) на 1 градус нахилу
    float getStepsPerDegree() const {
        if (gearRatio <= 0.0f) return 1.0f;
        return (stepsPerRev * microsteps * gearRatio) / 360.0f;
    }
};

// Знімок стану платформи нахилу
struct TiltStatus {
    int state = 0; // TiltState (IDLE, HOMING, MOVING, HOLDING, STOPPED, ERROR)
    const char* stateStr = "IDLE";
    char firmwareVersion[32] = {0};
    float gyroAngle = 0.0f;      // Поточний кут за гіроскопом GY-521 (із врахуванням збереженого нуля)
    float motorAngle = 0.0f;     // Розрахований кут за кроками двигуна
    float targetAngle = 0.0f;    // Цільовий кут
    float currentSpeed = 0.0f;   // Поточна швидкість (град/с)
    bool isHomed = false;        // Чи виконано пошук крайньої точки
    bool endstopTriggered = false; // Чи натиснуто кінцевик
    bool holdActive = false;     // Чи увімкнено режим автоматичного підтримання положення
    bool gyroConnected = false;  // Чи виявлено GY-521 (MPU6050)
    float pitchDeg = 0.0f;       // Сирий/відфільтрований кут Pitch
    float rollDeg = 0.0f;        // Сирий/відфільтрований кут Roll
    char errorMessage[64] = {0};
};

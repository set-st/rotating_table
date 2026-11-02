#pragma once
#include <Arduino.h>

// =============================================================================
// RailWay: плата MKS DLC32 (ESP32 + регістри зсуву I2S для STEP/DIR/EN).
// Використовуються осі X (горизонталь) та Z (вертикаль).
// =============================================================================

// Послідовний вивід у регістри зсуву. Біт N 32-бітного слова = I2SO(N).
constexpr uint8_t RW_I2S_BCK_PIN = 16;
constexpr uint8_t RW_I2S_WS_PIN = 17;   // Засувка (RCLK)
constexpr uint8_t RW_I2S_DATA_PIN = 21;

constexpr uint8_t RW_BIT_ENABLE = 0;    // Спільний EN драйверів (активний низький)
constexpr uint8_t RW_BIT_X_STEP = 1;
constexpr uint8_t RW_BIT_X_DIR = 2;
constexpr uint8_t RW_BIT_Z_STEP = 3;
constexpr uint8_t RW_BIT_Z_DIR = 4;

// Входи кінцевиків (лише вхід, без внутрішньої підтяжки)
constexpr uint8_t RW_PIN_LIMIT_X = 36;      // Початок горизонталі (X = 0)
constexpr uint8_t RW_PIN_LIMIT_X_FAR = 35;  // "Y LIMIT": протилежний кінець горизонталі
constexpr uint8_t RW_PIN_LIMIT_Z = 34;      // Початок вертикалі (Z = 0)

constexpr uint32_t RW_TICK_HZ = 20000;      // Частота таймера генерації імпульсів
constexpr float RW_MAX_SPEED_LIMIT_MM_S = 100.0f;
constexpr float RW_MIN_STEPS_PER_MM = 1.0f;
constexpr float RW_MAX_STEPS_PER_MM = 1000.0f;
constexpr float RW_MAX_LENGTH_MM = 5000.0f;

struct RailwaySettings {
    float stepsPerMmX = 80.0f;      // 200 кроків * 1/16 / (20 зубів * 2 мм)
    float stepsPerMmZ = 80.0f;
    float xLengthMm = 1000.0f;      // Вимірюється при homing між двома кінцевиками X
    bool xLengthMeasured = false;
    float zHeightMm = 500.0f;       // Задається вручну; обмежує хід униз від кінцевика Z (зверху)
    bool xDirInvert = false;        // Інверсія сигналу DIR
    bool zDirInvert = false;
    bool limitActiveLow = true;     // true: кінцевик спрацьовує низьким рівнем
    bool zEndstopTop = true;        // Лише для відображення: кінцевик Z зверху
    bool autoHomeOnBoot = false;
    bool ignoreHoming = false;      // Налагодження: дозволити рух без homing (як у Klipper)
    float maxSpeedMmS = 50.0f;
    float accelMmS2 = 200.0f;
    float homingSpeedMmS = 20.0f;
    float homingBackoffMm = 5.0f;
};

enum class RailwayState : uint8_t {
    IDLE,
    MOVING,
    HOMING,
    STOPPED,
    ERROR
};

struct RailwayStatus {
    RailwayState state = RailwayState::IDLE;
    const char* stateStr = "IDLE";
    float xMm = 0.0f;
    float zMm = 0.0f;
    float targetXMm = 0.0f;
    float targetZMm = 0.0f;
    float speedMmS = 0.0f;
    bool homedX = false;
    bool homedZ = false;
    bool xLengthMeasured = false;
    bool limitX = false;
    bool limitXFar = false;
    bool limitZ = false;
    bool enabled = true;
    char errorMessage[96] = {0};
};

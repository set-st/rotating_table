#pragma once

#include <Arduino.h>

// =============================================================================
// BLE СЕРВІС ТА ХАРАКТЕРИСТИКИ ДЛЯ ЗВ'ЯЗКУ МІЖ ПЛАТАМИ
// =============================================================================
#define TILT_BLE_SERVICE_UUID        "19b10000-e8f2-537e-4f6c-d104768a1214"
#define TILT_BLE_CHAR_CMD_UUID       "19b10001-e8f2-537e-4f6c-d104768a1214"
#define TILT_BLE_CHAR_STATUS_UUID    "19b10002-e8f2-537e-4f6c-d104768a1214"
#define TILT_BLE_CHAR_SETTINGS_UUID  "19b10003-e8f2-537e-4f6c-d104768a1214"
#define TILT_BLE_CHAR_OTA_CONTROL_UUID "19b10004-e8f2-537e-4f6c-d104768a1214"
#define TILT_BLE_CHAR_OTA_DATA_UUID    "19b10005-e8f2-537e-4f6c-d104768a1214"

#define TILT_DEFAULT_DEVICE_NAME     "TiltTable-ESP32"

// Стани контролера нахилу
enum TiltState {
    TILT_STATE_IDLE,
    TILT_STATE_HOMING,
    TILT_STATE_MOVING,
    TILT_STATE_HOLDING,
    TILT_STATE_STOPPED,
    TILT_STATE_ERROR
};

inline const char* tiltStateToString(TiltState st) {
    switch (st) {
        case TILT_STATE_IDLE:    return "IDLE";
        case TILT_STATE_HOMING:  return "HOMING";
        case TILT_STATE_MOVING:  return "MOVING";
        case TILT_STATE_HOLDING: return "HOLDING";
        case TILT_STATE_STOPPED: return "STOPPED";
        case TILT_STATE_ERROR:   return "ERROR";
        default:                 return "UNKNOWN";
    }
}

inline TiltState stringToTiltState(const char* str) {
    if (!str) return TILT_STATE_IDLE;
    if (strcmp(str, "HOMING") == 0) return TILT_STATE_HOMING;
    if (strcmp(str, "MOVING") == 0) return TILT_STATE_MOVING;
    if (strcmp(str, "HOLDING") == 0) return TILT_STATE_HOLDING;
    if (strcmp(str, "STOPPED") == 0) return TILT_STATE_STOPPED;
    if (strcmp(str, "ERROR") == 0) return TILT_STATE_ERROR;
    return TILT_STATE_IDLE;
}

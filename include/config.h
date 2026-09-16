#pragma once
#include <Arduino.h>

// =============================================================================
// PIN CONFIGURATION
// =============================================================================
// Stepper Driver (A4988, DRV8825, TMC2208/2209, etc.)
constexpr int PIN_STEP          = 18;  // Step pulse pin
constexpr int PIN_DIR           = 19;  // Direction pin
constexpr int PIN_ENABLE        = 5;   // Driver enable pin (-1 if not connected)

// Driver Enable Active Level
// Most drivers (A4988, DRV8825, TMC2208/2209) enable when EN pin is pulled LOW
constexpr bool ENABLE_ACTIVE_LOW = true;

// Limit Switch / Endstop (Концевик)
// By default connects between PIN_ENDSTOP and GND
constexpr int PIN_ENDSTOP       = 4;   // Endstop GPIO
constexpr bool ENDSTOP_PULLUP   = true; // Use ESP32 internal pullup (INPUT_PULLUP)
// When switch is pressed, does it pull pin LOW (to GND) or HIGH (to 3.3V)?
// Standard normally-open switch connected to GND will be LOW when pressed:
constexpr bool ENDSTOP_ACTIVE_LOW = true;

// Direction Inversion
// Set to true if motor rotates in reverse direction
constexpr bool INVERT_DIR       = false;

// =============================================================================
// MECHANICAL & KINEMATICS CONFIGURATION
// =============================================================================
constexpr float STEPS_PER_MOTOR_REV = 200.0f; // 1.8° stepper = 200, 0.9° stepper = 400
constexpr float MICROSTEPS          = 16.0f;  // Driver microstepping (1, 2, 4, 8, 16, 32...)
constexpr float GEAR_RATIO          = 1.0f;   // 1.0 = direct drive; >1.0 if gear/belt reduction (e.g. 4.0 for 4:1)

// Calculated steps per single degree of table rotation
constexpr float STEPS_PER_DEGREE    = (STEPS_PER_MOTOR_REV * MICROSTEPS * GEAR_RATIO) / 360.0f;

// =============================================================================
// SPEED & ACCELERATION LIMITS (in Degrees & Degrees/sec)
// =============================================================================
constexpr float DEFAULT_SPEED_DEG_S     = 30.0f;   // Default rotation speed in deg/s
constexpr float MAX_SPEED_DEG_S         = 180.0f;  // Absolute maximum speed in deg/s
constexpr float DEFAULT_ACCEL_DEG_S2    = 90.0f;   // Acceleration in deg/s^2

// =============================================================================
// HOMING (ПОИСК КОНЦЕВИКА И КАЛИБРОВКА НУЛЯ)
// =============================================================================
// Direction: -1 = counter-clockwise (влево), +1 = clockwise (вправо)
constexpr int   HOMING_DIRECTION        = -1;
constexpr float HOMING_SPEED_FAST_DEG_S = 25.0f;  // Fast approach speed
constexpr float HOMING_SPEED_SLOW_DEG_S = 5.0f;   // Precise touch speed
constexpr float HOMING_BACKOFF_DEG      = 5.0f;   // Back-off angle after first touch
constexpr uint32_t HOMING_TIMEOUT_SEC   = 35;     // Safety timeout (prevents infinite loop if endstop disconnected)
constexpr bool  AUTO_HOME_ON_BOOT       = true;   // Automatically search endstop at boot

// =============================================================================
// WI-FI & NETWORK CONFIGURATION
// =============================================================================
// Set your local Wi-Fi credentials here (or leave empty to start in AP mode):
#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 10000; // Time to wait for router before fallback to AP

// Fallback Access Point (AP) if router is unavailable or credentials not set:
#define AP_SSID       "RotatingTable-ESP32"
#define AP_PASSWORD   "12345678" // Minimum 8 characters for WPA2, or empty "" for open network

#define HOSTNAME      "rotating-table" // Accessible at http://rotating-table.local
constexpr uint16_t WEB_SERVER_PORT = 80;

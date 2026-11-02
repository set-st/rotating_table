#include "tilt_imu.h"
#include <Wire.h>
#include <Preferences.h>
#include <math.h>

static const char* NVS_TILT_IMU_NS = "tilt_imu";
static constexpr float RAD_TO_DEG_F = 57.295779513f;

TiltImu tiltImu;

TiltImu::TiltImu()
    : sdaPin(21), sclPin(22), mpuAddress(0x68), tiltAxis(0),
      initialized(false), connected(false), zeroConfigured(false),
      gyroBiasX(0.0f), gyroBiasY(0.0f), gyroBiasZ(0.0f),
      pitchOffsetDeg(0.0f), rollOffsetDeg(0.0f),
      filteredPitchDeg(0.0f), filteredRollDeg(0.0f), currentGyroRate(0.0f),
      lastUpdateMs(0), lastSampleUs(0), lastReadErrorLogMs(0) {}

void TiltImu::loadNvs() {
    Preferences prefs;
    if (prefs.begin(NVS_TILT_IMU_NS, true)) {
        pitchOffsetDeg = prefs.getFloat("pitch_zero", 0.0f);
        rollOffsetDeg = prefs.getFloat("roll_zero", 0.0f);
        zeroConfigured = prefs.getBool(
            "zero_set", pitchOffsetDeg != 0.0f || rollOffsetDeg != 0.0f);
        gyroBiasX = prefs.getFloat("bias_x", 0.0f);
        gyroBiasY = prefs.getFloat("bias_y", 0.0f);
        gyroBiasZ = prefs.getFloat("bias_z", 0.0f);
        prefs.end();
        Serial.printf("[Tilt IMU] NVS завантажено: Pitch Zero=%.2f°, Roll Zero=%.2f°\n",
                      pitchOffsetDeg, rollOffsetDeg);
    }
}

void TiltImu::saveNvs() {
    Preferences prefs;
    if (prefs.begin(NVS_TILT_IMU_NS, false)) {
        prefs.putFloat("pitch_zero", pitchOffsetDeg);
        prefs.putFloat("roll_zero", rollOffsetDeg);
        prefs.putBool("zero_set", zeroConfigured);
        prefs.putFloat("bias_x", gyroBiasX);
        prefs.putFloat("bias_y", gyroBiasY);
        prefs.putFloat("bias_z", gyroBiasZ);
        prefs.end();
        Serial.printf("[Tilt IMU] NVS збережено: Pitch Zero=%.2f°, Roll Zero=%.2f°\n",
                      pitchOffsetDeg, rollOffsetDeg);
    } else {
        Serial.println("[Tilt IMU] Помилка відкриття NVS для запису!");
    }
}

bool TiltImu::probe(uint8_t address) {
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
}

bool TiltImu::readSample(int16_t& ax, int16_t& ay, int16_t& az,
                         int16_t& gx, int16_t& gy, int16_t& gz) {
    Wire.beginTransmission(mpuAddress);
    Wire.write(0x3B);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }
    const uint8_t expectedBytes = 14;
    if (Wire.requestFrom(mpuAddress, expectedBytes) != expectedBytes ||
        Wire.available() < expectedBytes) {
        while (Wire.available()) {
            Wire.read();
        }
        return false;
    }

    auto readWord = [](int16_t& value) -> bool {
        const int highByte = Wire.read();
        const int lowByte = Wire.read();
        if (highByte < 0 || lowByte < 0) {
            return false;
        }
        value = static_cast<int16_t>((highByte << 8) | lowByte);
        return true;
    };
    int16_t temperature;
    return readWord(ax) && readWord(ay) && readWord(az) &&
           readWord(temperature) && readWord(gx) && readWord(gy) &&
           readWord(gz);
}

bool TiltImu::begin(int sda, int scl, uint8_t addr, int axis) {
    sdaPin = sda;
    sclPin = scl;
    mpuAddress = addr;
    tiltAxis = axis;

    loadNvs();

    if (!Wire.begin(sdaPin, sclPin)) {
        Serial.printf("[Tilt IMU] Не вдалося запустити I2C на SDA=%d, SCL=%d\n",
                      sdaPin, sclPin);
        return false;
    }
    Wire.setTimeOut(50);
    Wire.setClock(100000);

    connected = probe(mpuAddress);
    if (!connected) {
        Serial.printf("[Tilt IMU] MPU-6050 (GY-521) не знайдено за адресою 0x%02X на SDA=%d, SCL=%d\n",
                      mpuAddress, sdaPin, sclPin);
        return false;
    }

    Wire.beginTransmission(mpuAddress);
    Wire.write(0x75); // WHO_AM_I
    if (Wire.endTransmission(false) != 0 ||
        Wire.requestFrom(mpuAddress, static_cast<uint8_t>(1)) != 1 ||
        Wire.available() < 1) {
        Serial.println("[Tilt IMU] MPU-6050 відповідає на адресу, але WHO_AM_I прочитати не вдалося.");
        connected = false;
        return false;
    }
    const int whoAmI = Wire.read();
    Serial.printf("[Tilt IMU] WHO_AM_I=0x%02X (очікується 0x68)\n", whoAmI);

    auto writeRegister = [this](uint8_t reg, uint8_t value) {
        Wire.beginTransmission(mpuAddress);
        Wire.write(reg);
        Wire.write(value);
        return Wire.endTransmission() == 0;
    };
    if (!writeRegister(0x6B, 0x00)) {
        Serial.println("[Tilt IMU] Помилка пробудження MPU-6050 (PWR_MGMT_1).");
        connected = false;
        return false;
    }
    delay(100);
    if (!writeRegister(0x1C, 0x00)) {
        Serial.println("[Tilt IMU] Не вдалося налаштувати акселерометр MPU-6050.");
        connected = false;
        return false;
    }
    if (!writeRegister(0x1B, 0x00)) {
        Serial.println("[Tilt IMU] Не вдалося налаштувати гіроскоп MPU-6050.");
        connected = false;
        return false;
    }

    int16_t ax, ay, az, gx, gy, gz;
    if (!readSample(ax, ay, az, gx, gy, gz)) {
        Serial.printf("[Tilt IMU] Датчик відповідає за адресою 0x%02X, але не віддає 14 байтів вимірювання на 100 кГц.\n",
                      mpuAddress);
        connected = false;
        return false;
    }

    initialized = true;
    connected = true;
    filteredPitchDeg = 0.0f;
    filteredRollDeg = 0.0f;
    lastSampleUs = micros();
    lastUpdateMs = millis();
    lastReadErrorLogMs = millis();

    Serial.printf("[Tilt IMU] MPU-6050 ініціалізовано й перший зразок прочитано: SDA=%d, SCL=%d, Addr=0x%02X, ACC=(%d,%d,%d), GYRO=(%d,%d,%d)\n",
                  sdaPin, sclPin, mpuAddress, ax, ay, az, gx, gy, gz);
    return true;
}

void TiltImu::update() {
    if (!initialized || millis() - lastUpdateMs < 10) {
        return;
    }
    lastUpdateMs = millis();

    int16_t ax, ay, az, gx, gy, gz;
    if (!readSample(ax, ay, az, gx, gy, gz)) {
        connected = false;
        if (millis() - lastReadErrorLogMs >= 1000) {
            lastReadErrorLogMs = millis();
            Serial.printf("[Tilt IMU] Помилка читання вимірювань MPU-6050 за адресою 0x%02X (SDA=%d, SCL=%d, I2C=100 кГц).\n",
                          mpuAddress, sdaPin, sclPin);
        }
        return;
    }
    if (!connected) {
        Serial.println("[Tilt IMU] Читання MPU-6050 відновлено.");
    }
    connected = true;

    uint32_t nowUs = micros();
    float dt = (nowUs - lastSampleUs) / 1000000.0f;
    lastSampleUs = nowUs;
    if (dt <= 0.0f || dt > 0.5f) {
        dt = 0.01f;
    }

    // Розрахунок кутів за акселерометром
    const float accelPitch = atan2f(-static_cast<float>(ax),
                                    sqrtf(static_cast<float>(ay) * ay +
                                          static_cast<float>(az) * az)) * RAD_TO_DEG_F;
    const float accelRoll = atan2f(static_cast<float>(ay),
                                   static_cast<float>(az)) * RAD_TO_DEG_F;

    // Кутові швидкості гіроскопа (131 LSB / (°/s) при діапазоні ±250°/s)
    const float gyroX = (gx / 131.0f) - gyroBiasX;
    const float gyroY = (gy / 131.0f) - gyroBiasY;
    const float gyroZ = (gz / 131.0f) - gyroBiasZ;

    // Комплементарний фільтр (98% гіроскоп + 2% акселерометр)
    filteredPitchDeg = 0.98f * (filteredPitchDeg + gyroY * dt) + 0.02f * accelPitch;
    filteredRollDeg  = 0.98f * (filteredRollDeg  + gyroX * dt) + 0.02f * accelRoll;

    currentGyroRate = (tiltAxis == 0) ? gyroY : gyroX;
}

float TiltImu::getTiltAngle() const {
    // Повертає відносний кут нахилу із врахуванням збереженого нуля в NVS!
    if (tiltAxis == 0) {
        return filteredPitchDeg - pitchOffsetDeg;
    } else {
        return filteredRollDeg - rollOffsetDeg;
    }
}

float TiltImu::getPitch() const {
    return filteredPitchDeg - pitchOffsetDeg;
}

float TiltImu::getRoll() const {
    return filteredRollDeg - rollOffsetDeg;
}

float TiltImu::getGyroRate() const {
    return currentGyroRate;
}

bool TiltImu::isConnected() const {
    return connected;
}

bool TiltImu::hasZeroOrientation() const {
    return zeroConfigured;
}

bool TiltImu::zeroOrientation() {
    if (!initialized || !connected) {
        return false;
    }
    // Запам'ятовуємо поточні фізичні кути фільтра як нове нульове положення в NVS
    pitchOffsetDeg = filteredPitchDeg;
    rollOffsetDeg = filteredRollDeg;
    zeroConfigured = true;
    saveNvs();
    Serial.printf("[Tilt IMU] Встановлено та збережено новий нуль: Pitch=%.2f°, Roll=%.2f°\n",
                  pitchOffsetDeg, rollOffsetDeg);
    return true;
}

bool TiltImu::calibrateGyro(uint16_t sampleCount) {
    if (!initialized || !connected || sampleCount == 0) {
        return false;
    }
    Serial.printf("[Tilt IMU] Калібрування нульового дрейфу гіроскопа (зразків: %u)...\n", sampleCount);
    int64_t sumX = 0, sumY = 0, sumZ = 0;
    for (uint16_t i = 0; i < sampleCount; ++i) {
        int16_t ax, ay, az, gx, gy, gz;
        if (!readSample(ax, ay, az, gx, gy, gz)) {
            return false;
        }
        sumX += gx;
        sumY += gy;
        sumZ += gz;
        delay(2);
    }
    gyroBiasX = static_cast<float>(sumX) / sampleCount / 131.0f;
    gyroBiasY = static_cast<float>(sumY) / sampleCount / 131.0f;
    gyroBiasZ = static_cast<float>(sumZ) / sampleCount / 131.0f;
    saveNvs();
    Serial.printf("[Tilt IMU] Bias збережено: X=%.2f, Y=%.2f, Z=%.2f °/с\n",
                  gyroBiasX, gyroBiasY, gyroBiasZ);
    return true;
}

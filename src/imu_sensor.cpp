#include "imu_sensor.h"

#include <Preferences.h>
#include <Wire.h>
#include <math.h>

static const char* NVS_IMU_NAMESPACE = "table_imu";
static constexpr int DEFAULT_I2C_SDA = 21;
static constexpr int DEFAULT_I2C_SCL = 22;
static constexpr uint8_t DEFAULT_MPU6050_ADDRESS = 0x68;
static constexpr uint8_t DEFAULT_BAROMETER_ADDRESS = 0x77;
static constexpr float RAD_TO_DEG_F = 57.295779513f;

ImuSensor imuSensor;

ImuSensor::ImuSensor()
    : gyroBiasX(0.0f), gyroBiasY(0.0f), gyroBiasZ(0.0f),
      pitchOffsetDeg(0.0f), rollOffsetDeg(0.0f),
      filteredPitchDeg(0.0f), filteredRollDeg(0.0f),
      lastUpdateMs(0), lastSampleUs(0) {
    config = {DEFAULT_I2C_SDA, DEFAULT_I2C_SCL, DEFAULT_MPU6050_ADDRESS,
              DEFAULT_BAROMETER_ADDRESS};
    memset(&status, 0, sizeof(status));
}

void ImuSensor::loadConfig() {
    Preferences prefs;
    if (!prefs.begin(NVS_IMU_NAMESPACE, true)) {
        return;
    }
    config.sdaPin = prefs.getInt("sda", DEFAULT_I2C_SDA);
    config.sclPin = prefs.getInt("scl", DEFAULT_I2C_SCL);
    config.mpu6050Address =
        prefs.getUChar("mpu_addr", DEFAULT_MPU6050_ADDRESS);
    config.barometerAddress =
        prefs.getUChar("baro_addr", DEFAULT_BAROMETER_ADDRESS);
    gyroBiasX = prefs.getFloat("gyro_bias_x", 0.0f);
    gyroBiasY = prefs.getFloat("gyro_bias_y", 0.0f);
    gyroBiasZ = prefs.getFloat("gyro_bias_z", 0.0f);
    pitchOffsetDeg = prefs.getFloat("pitch_zero", 0.0f);
    rollOffsetDeg = prefs.getFloat("roll_zero", 0.0f);
    prefs.end();
}

void ImuSensor::saveConfig() {
    Preferences prefs;
    if (!prefs.begin(NVS_IMU_NAMESPACE, false)) {
        Serial.println("[IMU] Помилка відкриття NVS для запису!");
        return;
    }
    prefs.putInt("sda", config.sdaPin);
    prefs.putInt("scl", config.sclPin);
    prefs.putUChar("mpu_addr", config.mpu6050Address);
    prefs.putUChar("baro_addr", config.barometerAddress);
    prefs.putFloat("gyro_bias_x", gyroBiasX);
    prefs.putFloat("gyro_bias_y", gyroBiasY);
    prefs.putFloat("gyro_bias_z", gyroBiasZ);
    prefs.putFloat("pitch_zero", pitchOffsetDeg);
    prefs.putFloat("roll_zero", rollOffsetDeg);
    prefs.end();
}

bool ImuSensor::probe(uint8_t address) {
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
}

void ImuSensor::setError(const char* message) {
    strncpy(status.errorMessage, message, sizeof(status.errorMessage) - 1);
    status.errorMessage[sizeof(status.errorMessage) - 1] = '\0';
}

bool ImuSensor::begin() {
    loadConfig();
    memset(&status, 0, sizeof(status));
    Wire.begin(config.sdaPin, config.sclPin);
    Wire.setClock(400000);

    status.mpu6050Connected = probe(config.mpu6050Address);
    status.barometerConnected = probe(config.barometerAddress);

    if (!status.mpu6050Connected) {
        setError("MPU6050 не відповідає на вказаній I2C-адресі");
        Serial.printf("[IMU] Помилка: MPU6050 не знайдено за адресою 0x%02X\n",
                      config.mpu6050Address);
        return false;
    }

    // Wake MPU6050 and configure ±2 g / ±250 deg/s ranges.
    Wire.beginTransmission(config.mpu6050Address);
    Wire.write(0x6B);
    Wire.write(0x00);
    if (Wire.endTransmission() != 0) {
        setError("Не вдалося активувати MPU6050");
        return false;
    }
    Wire.beginTransmission(config.mpu6050Address);
    Wire.write(0x1C);
    Wire.write(0x00);
    Wire.endTransmission();
    Wire.beginTransmission(config.mpu6050Address);
    Wire.write(0x1B);
    Wire.write(0x00);
    Wire.endTransmission();

    status.initialized = true;
    status.errorMessage[0] = '\0';
    filteredPitchDeg = 0.0f;
    filteredRollDeg = 0.0f;
    status.pitchDeg = 0.0f;
    status.rollDeg = 0.0f;
    lastSampleUs = micros();
    Serial.printf("[IMU] MPU6050 підключено: 0x%02X, SDA=%d, SCL=%d\n",
                  config.mpu6050Address, config.sdaPin, config.sclPin);
    Serial.printf("[IMU] BMP180: %s\n",
                  status.barometerConnected ? "знайдено" : "не знайдено");
    return true;
}

bool ImuSensor::readMpuSample(int16_t& accelX, int16_t& accelY, int16_t& accelZ,
                              int16_t& gyroX, int16_t& gyroY, int16_t& gyroZ) {
    Wire.beginTransmission(config.mpu6050Address);
    Wire.write(0x3B);
    if (Wire.endTransmission(false) != 0 ||
        Wire.requestFrom(config.mpu6050Address, static_cast<uint8_t>(14)) != 14) {
        return false;
    }
    auto readWord = []() -> int16_t {
        return static_cast<int16_t>((Wire.read() << 8) | Wire.read());
    };
    accelX = readWord();
    accelY = readWord();
    accelZ = readWord();
    readWord(); // temperature
    gyroX = readWord();
    gyroY = readWord();
    gyroZ = readWord();
    return true;
}

void ImuSensor::update() {
    if (!status.initialized || millis() - lastUpdateMs < 10) {
        return;
    }
    lastUpdateMs = millis();

    int16_t ax, ay, az, gx, gy, gz;
    if (!readMpuSample(ax, ay, az, gx, gy, gz)) {
        status.mpu6050Connected = false;
        setError("Помилка читання даних MPU6050");
        return;
    }

    uint32_t nowUs = micros();
    float dt = (nowUs - lastSampleUs) / 1000000.0f;
    lastSampleUs = nowUs;
    if (dt <= 0.0f || dt > 0.5f) {
        dt = 0.01f;
    }

    const float accelPitch =
        atan2f(-static_cast<float>(ax),
               sqrtf(static_cast<float>(ay) * ay +
                     static_cast<float>(az) * az)) *
        RAD_TO_DEG_F;
    const float accelRoll =
        atan2f(static_cast<float>(ay), static_cast<float>(az)) * RAD_TO_DEG_F;
    status.gyroXDegS = gx / 131.0f - gyroBiasX;
    status.gyroYDegS = gy / 131.0f - gyroBiasY;
    status.gyroZDegS = gz / 131.0f - gyroBiasZ;

    // Complementary filter: gyro is responsive, accelerometer corrects drift.
    filteredPitchDeg =
        0.98f * (filteredPitchDeg + status.gyroYDegS * dt) +
        0.02f * accelPitch;
    filteredRollDeg =
        0.98f * (filteredRollDeg + status.gyroXDegS * dt) +
        0.02f * accelRoll;
    status.pitchDeg = filteredPitchDeg - pitchOffsetDeg;
    status.rollDeg = filteredRollDeg - rollOffsetDeg;
    status.updatedAtMs = millis();
    status.errorMessage[0] = '\0';
}

ImuConfig ImuSensor::getConfig() const { return config; }

ImuStatus ImuSensor::getStatus() const { return status; }

ImuScanResult ImuSensor::scanBus() {
    ImuScanResult result = {};
    Serial.printf("[IMU] I2C scan SDA=%d, SCL=%d:\n", config.sdaPin,
                  config.sclPin);
    for (uint8_t address = 0x03; address <= 0x77; ++address) {
        if (probe(address)) {
            if (result.count < sizeof(result.addresses)) {
                result.addresses[result.count++] = address;
            }

            Serial.printf("[IMU] I2C знайдено адресу 0x%02X\n", address);
        }
    }
    if (result.count == 0) {
        Serial.println("[IMU] I2C пристроїв не знайдено");
    }
    return result;
}

bool ImuSensor::calibrateGyro(uint16_t sampleCount) {
    if (!status.initialized || sampleCount == 0) {
        return false;
    }

    Serial.printf("[IMU] Початок калібрування гіроскопа, зразків: %u\n",
                  sampleCount);
    int64_t sumX = 0;
    int64_t sumY = 0;
    int64_t sumZ = 0;
    for (uint16_t i = 0; i < sampleCount; ++i) {
        int16_t ax, ay, az, gx, gy, gz;
        if (!readMpuSample(ax, ay, az, gx, gy, gz)) {
            setError("Помилка читання MPU6050 під час калібрування");
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
    saveConfig();
    status.gyroXDegS = 0.0f;
    status.gyroYDegS = 0.0f;
    status.gyroZDegS = 0.0f;
    status.errorMessage[0] = '\0';
    Serial.printf("[IMU] Bias: X=%.3f, Y=%.3f, Z=%.3f град/с\n",
                  gyroBiasX, gyroBiasY, gyroBiasZ);
    return true;
}

bool ImuSensor::zeroOrientation() {
    if (!status.initialized) {
        return false;
    }
    pitchOffsetDeg = filteredPitchDeg;
    rollOffsetDeg = filteredRollDeg;
    status.pitchDeg = 0.0f;
    status.rollDeg = 0.0f;
    saveConfig();
    Serial.println("[IMU] Поточне положення збережено як нульове");
    return true;
}

bool ImuSensor::applyConfig(const ImuConfig& newConfig, bool& rebootRequired) {
    rebootRequired =
        newConfig.sdaPin != config.sdaPin ||
        newConfig.sclPin != config.sclPin ||
        newConfig.mpu6050Address != config.mpu6050Address ||
        newConfig.barometerAddress != config.barometerAddress;
    config = newConfig;
    saveConfig();
    return true;
}

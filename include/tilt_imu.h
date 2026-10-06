#pragma once
#include <Arduino.h>

class TiltImu {
public:
    TiltImu();

    bool begin(int sdaPin, int sclPin, uint8_t mpuAddress, int tiltAxis);
    void update();

    // Отримання поточного виміряного кута нахилу (із врахуванням збереженого нуля в NVS)
    float getTiltAngle() const;
    float getPitch() const;
    float getRoll() const;
    float getGyroRate() const; // Кутова швидкість вздовж робочої осі (град/с)
    bool isConnected() const;

    // Скидання гіроскопа: збереження поточного фізичного положення як 0.0° у flash-пам'ять NVS
    bool zeroOrientation();

    // Калібрування нульового зміщення гіроскопа (bias)
    bool calibrateGyro(uint16_t sampleCount = 400);

    // Оновлення конфігурації
    void setAxis(int axis) { tiltAxis = axis; }

private:
    int sdaPin;
    int sclPin;
    uint8_t mpuAddress;
    int tiltAxis; // 0 = Pitch, 1 = Roll

    bool initialized;
    bool connected;

    float gyroBiasX;
    float gyroBiasY;
    float gyroBiasZ;

    float pitchOffsetDeg; // Збережений нуль Pitch у NVS
    float rollOffsetDeg;  // Збережений нуль Roll у NVS

    float filteredPitchDeg;
    float filteredRollDeg;
    float currentGyroRate;

    uint32_t lastUpdateMs;
    uint32_t lastSampleUs;

    void loadNvs();
    void saveNvs();
    bool probe(uint8_t address);
    bool readSample(int16_t& ax, int16_t& ay, int16_t& az,
                    int16_t& gx, int16_t& gy, int16_t& gz);
};

extern TiltImu tiltImu;

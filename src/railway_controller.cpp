#include "railway_controller.h"
#include <Preferences.h>
#include <soc/gpio_reg.h>

RailwayController railway;

namespace {

constexpr uint32_t MAX_RATE_SPS = RW_TICK_HZ / 2;   // Один тик HIGH + один тик LOW
constexpr float MIN_RAMP_SPS = 50.0f;
constexpr uint32_t CONTROL_PERIOD_MS = 5;

struct IsrAxis {
    volatile uint32_t rate = 0;        // Кроків за секунду
    volatile uint32_t remaining = 0;   // Кроків до зупинки
    volatile int32_t pos = 0;
    volatile int8_t dir = 1;
    volatile bool dirLevel = false;
    uint32_t acc = 0;
    bool high = false;
};

IsrAxis g_axis[2];
volatile bool g_driversEnabled = true;
uint32_t g_lastWord = 0xFFFFFFFFUL;
portMUX_TYPE g_mux = portMUX_INITIALIZER_UNLOCKED;
hw_timer_t* g_timer = nullptr;

const uint8_t STEP_BIT[2] = {RW_BIT_X_STEP, RW_BIT_Z_STEP};
const uint8_t DIR_BIT[2] = {RW_BIT_X_DIR, RW_BIT_Z_DIR};

inline void IRAM_ATTR pinHigh(uint8_t pin) { REG_WRITE(GPIO_OUT_W1TS_REG, 1UL << pin); }
inline void IRAM_ATTR pinLow(uint8_t pin) { REG_WRITE(GPIO_OUT_W1TC_REG, 1UL << pin); }

void IRAM_ATTR shiftWord(uint32_t word) {
    for (int i = 31; i >= 0; --i) {
        if ((word >> i) & 1U) pinHigh(RW_I2S_DATA_PIN); else pinLow(RW_I2S_DATA_PIN);
        pinHigh(RW_I2S_BCK_PIN);
        pinLow(RW_I2S_BCK_PIN);
    }
    pinHigh(RW_I2S_WS_PIN);
    pinLow(RW_I2S_WS_PIN);
}

void IRAM_ATTR onTick() {
    uint32_t word = 0;
    if (!g_driversEnabled) word |= 1UL << RW_BIT_ENABLE;

    for (uint8_t a = 0; a < 2; ++a) {
        IsrAxis& s = g_axis[a];
        if (s.high) {
            s.high = false;
        } else if (s.rate > 0 && s.remaining > 0) {
            s.acc += s.rate;
            if (s.acc >= RW_TICK_HZ) {
                s.acc -= RW_TICK_HZ;
                s.high = true;
                s.pos += s.dir;
                if (--s.remaining == 0) s.rate = 0;
            }
        }
        if (s.high) word |= 1UL << STEP_BIT[a];
        if (s.dirLevel) word |= 1UL << DIR_BIT[a];
    }

    if (word != g_lastWord) {
        g_lastWord = word;
        shiftWord(word);
    }
}

}  // namespace

bool RailwayController::begin() {
    mutex = xSemaphoreCreateMutex();
    if (!mutex) return false;

    loadSettings();

    pinMode(RW_I2S_BCK_PIN, OUTPUT);
    pinMode(RW_I2S_WS_PIN, OUTPUT);
    pinMode(RW_I2S_DATA_PIN, OUTPUT);
    digitalWrite(RW_I2S_BCK_PIN, LOW);
    digitalWrite(RW_I2S_WS_PIN, LOW);
    pinMode(RW_PIN_LIMIT_X, INPUT);
    pinMode(RW_PIN_LIMIT_X_FAR, INPUT);
    pinMode(RW_PIN_LIMIT_Z, INPUT);

    g_driversEnabled = true;
    g_timer = timerBegin(1, 80, true);   // 80 МГц / 80 = 1 МГц
    if (!g_timer) return false;
    timerAttachInterrupt(g_timer, &onTick, true);
    timerAlarmWrite(g_timer, 1000000UL / RW_TICK_HZ, true);
    timerAlarmEnable(g_timer);

    xTaskCreatePinnedToCore(taskEntry, "railway", 4096, this, 3, nullptr, 1);
    Serial.println("[RailWay] Контролер осей X/Z запущено.");
    return true;
}

void RailwayController::taskEntry(void* arg) {
    RailwayController* self = static_cast<RailwayController*>(arg);
    TickType_t last = xTaskGetTickCount();
    uint32_t lastMs = millis();
    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(CONTROL_PERIOD_MS));
        uint32_t now = millis();
        float dt = (now - lastMs) / 1000.0f;
        lastMs = now;
        if (dt <= 0.0f || dt > 0.1f) dt = CONTROL_PERIOD_MS / 1000.0f;
        xSemaphoreTake(self->mutex, portMAX_DELAY);
        self->controlStep(dt);
        xSemaphoreGive(self->mutex);
    }
}

void RailwayController::loadSettings() {
    Preferences p;
    p.begin("railway", true);
    cfg.stepsPerMmX = p.getFloat("spmx", cfg.stepsPerMmX);
    cfg.stepsPerMmZ = p.getFloat("spmz", cfg.stepsPerMmZ);
    cfg.xLengthMm = p.getFloat("xlen", cfg.xLengthMm);
    cfg.xLengthMeasured = p.getBool("xmeas", false);
    cfg.zHeightMm = p.getFloat("zlen", cfg.zHeightMm);
    cfg.xDirInvert = p.getBool("xinv", cfg.xDirInvert);
    cfg.zDirInvert = p.getBool("zinv", cfg.zDirInvert);
    cfg.limitActiveLow = p.getBool("lowact", cfg.limitActiveLow);
    cfg.zEndstopTop = p.getBool("ztop", cfg.zEndstopTop);
    cfg.autoHomeOnBoot = p.getBool("autohome", cfg.autoHomeOnBoot);
    cfg.ignoreHoming = p.getBool("ignhome", cfg.ignoreHoming);
    cfg.maxSpeedMmS = p.getFloat("vmax", cfg.maxSpeedMmS);
    cfg.accelMmS2 = p.getFloat("accel", cfg.accelMmS2);
    cfg.homingSpeedMmS = p.getFloat("vhome", cfg.homingSpeedMmS);
    cfg.homingBackoffMm = p.getFloat("backoff", cfg.homingBackoffMm);
    p.end();
}

RailwaySettings RailwayController::getSettings() {
    xSemaphoreTake(mutex, portMAX_DELAY);
    RailwaySettings s = cfg;
    xSemaphoreGive(mutex);
    return s;
}

bool RailwayController::saveSettings(const RailwaySettings& s, String& error) {
    auto inRange = [](float v, float lo, float hi) { return v >= lo && v <= hi; };
    if (!inRange(s.stepsPerMmX, RW_MIN_STEPS_PER_MM, RW_MAX_STEPS_PER_MM) ||
        !inRange(s.stepsPerMmZ, RW_MIN_STEPS_PER_MM, RW_MAX_STEPS_PER_MM)) {
        error = "Кроків на мм: допустимо 1–1000";
        return false;
    }
    if (!inRange(s.xLengthMm, 10.0f, RW_MAX_LENGTH_MM) ||
        !inRange(s.zHeightMm, 10.0f, RW_MAX_LENGTH_MM)) {
        error = "Довжина та висота: допустимо 10–" + String((int)RW_MAX_LENGTH_MM) + " мм";
        return false;
    }
    if (!inRange(s.maxSpeedMmS, 1.0f, RW_MAX_SPEED_LIMIT_MM_S) ||
        !inRange(s.homingSpeedMmS, 1.0f, RW_MAX_SPEED_LIMIT_MM_S)) {
        error = "Швидкість: допустимо 1–100 мм/с";
        return false;
    }
    if (!inRange(s.accelMmS2, 10.0f, 5000.0f)) {
        error = "Прискорення: допустимо 10–5000 мм/с²";
        return false;
    }
    if (!inRange(s.homingBackoffMm, 1.0f, 50.0f)) {
        error = "Відкат при homing: допустимо 1–50 мм";
        return false;
    }

    xSemaphoreTake(mutex, portMAX_DELAY);
    if (busy()) {
        xSemaphoreGive(mutex);
        error = "Зупиніть рух перед зміною налаштувань";
        return false;
    }
    const bool geometryChanged = s.stepsPerMmX != cfg.stepsPerMmX ||
                                 s.stepsPerMmZ != cfg.stepsPerMmZ ||
                                 s.xDirInvert != cfg.xDirInvert ||
                                 s.zDirInvert != cfg.zDirInvert;
    const bool xLengthEdited = s.xLengthMm != cfg.xLengthMm;
    const bool wasMeasured = cfg.xLengthMeasured;
    cfg = s;
    cfg.xLengthMeasured = wasMeasured && !xLengthEdited;
    if (geometryChanged) {
        homed[0] = homed[1] = false;
        cfg.xLengthMeasured = false;
    }

    Preferences p;
    p.begin("railway", false);
    p.putFloat("spmx", cfg.stepsPerMmX);
    p.putFloat("spmz", cfg.stepsPerMmZ);
    p.putFloat("xlen", cfg.xLengthMm);
    p.putBool("xmeas", cfg.xLengthMeasured);
    p.putFloat("zlen", cfg.zHeightMm);
    p.putBool("xinv", cfg.xDirInvert);
    p.putBool("zinv", cfg.zDirInvert);
    p.putBool("lowact", cfg.limitActiveLow);
    p.putBool("ztop", cfg.zEndstopTop);
    p.putBool("autohome", cfg.autoHomeOnBoot);
    p.putBool("ignhome", cfg.ignoreHoming);
    p.putFloat("vmax", cfg.maxSpeedMmS);
    p.putFloat("accel", cfg.accelMmS2);
    p.putFloat("vhome", cfg.homingSpeedMmS);
    p.putFloat("backoff", cfg.homingBackoffMm);
    p.end();
    xSemaphoreGive(mutex);
    return true;
}

float RailwayController::stepsPerMm(uint8_t a) const {
    return a == 0 ? cfg.stepsPerMmX : cfg.stepsPerMmZ;
}

float RailwayController::lengthMm(uint8_t a) const {
    return a == 0 ? cfg.xLengthMm : cfg.zHeightMm;
}

int32_t RailwayController::positionSteps(uint8_t a) const {
    return g_axis[a].pos;
}

bool RailwayController::busy() const {
    return homingActive || axes[0].moving || axes[1].moving;
}

bool RailwayController::limitActive(uint8_t a) const {
    return a == 0 ? limits[0] : limits[2];
}

bool RailwayController::farLimitActive(uint8_t a) const {
    return a == 0 && limits[1];
}

void RailwayController::fail(const char* msg) {
    stopAll();
    strlcpy(errorMsg, msg, sizeof(errorMsg));
    state = RailwayState::ERROR;
    Serial.printf("[RailWay] Помилка: %s\n", msg);
}

void RailwayController::hardStop(uint8_t a) {
    portENTER_CRITICAL(&g_mux);
    g_axis[a].rate = 0;
    g_axis[a].remaining = 0;
    portEXIT_CRITICAL(&g_mux);
    axes[a].moving = false;
    axes[a].speed = 0;
}

void RailwayController::stopAll() {
    hardStop(0);
    hardStop(1);
    homingActive = false;
    homePhase = HomePhase::NONE;
}

void RailwayController::startMove(uint8_t a, int32_t deltaSteps, float vmaxSps, float accelSps2) {
    if (deltaSteps == 0) return;
    const bool invert = (a == 0) ? cfg.xDirInvert : cfg.zDirInvert;
    const int8_t dir = deltaSteps > 0 ? 1 : -1;
    portENTER_CRITICAL(&g_mux);
    g_axis[a].rate = 0;
    g_axis[a].dir = dir;
    g_axis[a].dirLevel = (dir > 0) != invert;
    g_axis[a].remaining = (uint32_t)abs(deltaSteps);
    portEXIT_CRITICAL(&g_mux);
    Axis& A = axes[a];
    A.moving = true;
    A.dir = dir;
    A.speed = 0;
    A.vmax = constrain(vmaxSps, MIN_RAMP_SPS, (float)MAX_RATE_SPS);
    A.accel = max(accelSps2, 1.0f);
    A.settle = 1;
}

void RailwayController::updateLimits() {
    const bool raw[3] = {
        digitalRead(RW_PIN_LIMIT_X) == (cfg.limitActiveLow ? LOW : HIGH),
        digitalRead(RW_PIN_LIMIT_X_FAR) == (cfg.limitActiveLow ? LOW : HIGH),
        digitalRead(RW_PIN_LIMIT_Z) == (cfg.limitActiveLow ? LOW : HIGH)};
    for (uint8_t i = 0; i < 3; ++i) {
        // Стан змінюється лише після двох однакових вимірювань підряд
        if (raw[i] == limitsRaw[i]) limits[i] = raw[i];
        limitsRaw[i] = raw[i];
    }
}

void RailwayController::controlAxis(uint8_t a, float dt) {
    Axis& A = axes[a];
    if (!A.moving) return;

    const bool ignoreLimits = cfg.ignoreHoming && !homingActive;   // Налагодження: кінцевики не зупиняють звичайний рух
    if (!ignoreLimits && ((A.dir < 0 && limitActive(a)) || (A.dir > 0 && farLimitActive(a)))) {
        fail(a == 0 ? "Спрацював кінцевик осі X" : "Спрацював кінцевик осі Z");
        return;
    }
    if (A.settle > 0) {
        --A.settle;
        return;
    }

    const uint32_t remaining = g_axis[a].remaining;
    if (remaining == 0) {
        A.moving = false;
        A.speed = 0;
        g_axis[a].rate = 0;
        return;
    }

    const float stopDist = (A.speed * A.speed) / (2.0f * A.accel);
    if ((float)remaining <= stopDist) {
        A.speed -= A.accel * dt;
    } else {
        A.speed += A.accel * dt;
    }
    A.speed = constrain(A.speed, min(MIN_RAMP_SPS, A.vmax), A.vmax);
    g_axis[a].rate = (uint32_t)A.speed;
}

void RailwayController::beginHomePhase(uint8_t a, HomePhase phase) {
    homePhase = phase;
    homePhaseStartedMs = millis();
    const float spm = stepsPerMm(a);
    const float fastSps = cfg.homingSpeedMmS * spm;
    const float slowMmS = max(1.0f, cfg.homingSpeedMmS / 4.0f);
    const int32_t backoffSteps = (int32_t)(cfg.homingBackoffMm * spm);
    const float accel = cfg.accelMmS2 * spm;

    switch (phase) {
    case HomePhase::MEASURE:
        homePhaseTimeoutMs = (uint32_t)((RW_MAX_LENGTH_MM / cfg.homingSpeedMmS + 10.0f) * 1000.0f);
        startMove(a, (int32_t)(RW_MAX_LENGTH_MM * spm), fastSps, accel);
        break;
    case HomePhase::FAR_BACKOFF:
        homePhaseTimeoutMs = (uint32_t)((cfg.homingBackoffMm / cfg.homingSpeedMmS + 5.0f) * 1000.0f);
        startMove(a, -backoffSteps, fastSps, accel);
        break;
    case HomePhase::RELEASE:
    case HomePhase::BACKOFF:
        homePhaseTimeoutMs = (uint32_t)((cfg.homingBackoffMm / cfg.homingSpeedMmS + 5.0f) * 1000.0f);
        startMove(a, backoffSteps, fastSps, accel);
        break;
    case HomePhase::SEEK: {
        // Поки довжину X не виміряно, вона невідома: шукати на повний максимальний хід
        const float seekMm = (a == 0 && !cfg.xLengthMeasured)
                                 ? RW_MAX_LENGTH_MM
                                 : min(1.2f * lengthMm(a), RW_MAX_LENGTH_MM);
        homePhaseTimeoutMs = (uint32_t)((seekMm / cfg.homingSpeedMmS + 10.0f) * 1000.0f);
        startMove(a, -(int32_t)(seekMm * spm), fastSps, accel);
        break;
    }
    case HomePhase::SLOW:
        homePhaseTimeoutMs = (uint32_t)((cfg.homingBackoffMm * 3.0f / slowMmS + 5.0f) * 1000.0f);
        startMove(a, -(int32_t)(cfg.homingBackoffMm * 3.0f * spm), slowMmS * spm, accel);
        break;
    default:
        break;
    }
}

void RailwayController::updateHoming() {
    if (!homingActive) return;
    const uint8_t a = homingIndex;   // Спочатку X (з вимірюванням довжини), потім Z

    if (millis() - homePhaseStartedMs > homePhaseTimeoutMs) {
        fail("Таймаут homing: кінцевик не знайдено");
        return;
    }

    switch (homePhase) {
    case HomePhase::RELEASE:
        if (!axes[a].moving) {
            if (limitActive(a)) { fail("Кінцевик не відпускається після відкату"); return; }
            beginHomePhase(a, HomePhase::SEEK);
        }
        break;
    case HomePhase::SEEK:
        if (limitActive(a)) {
            hardStop(a);
            beginHomePhase(a, HomePhase::BACKOFF);
        } else if (!axes[a].moving) {
            fail("Кінцевик не знайдено в межах довжини осі");
        }
        break;
    case HomePhase::BACKOFF:
        if (!axes[a].moving) {
            if (limitActive(a)) { fail("Кінцевик не відпускається після відкату"); return; }
            beginHomePhase(a, HomePhase::SLOW);
        }
        break;
    case HomePhase::SLOW:
        if (limitActive(a)) {
            hardStop(a);
            portENTER_CRITICAL(&g_mux);
            g_axis[a].pos = 0;
            portEXIT_CRITICAL(&g_mux);
            homed[a] = true;
            targetMm[a] = 0;
            if (a == 0) {
                beginHomePhase(0, HomePhase::MEASURE);
            } else {
                homingActive = false;
                homePhase = HomePhase::NONE;
            }
        } else if (!axes[a].moving) {
            fail("Кінцевик не знайдено під час повільного заходу");
        }
        break;
    case HomePhase::MEASURE:
        if (farLimitActive(0)) {
            hardStop(0);
            measuredTriggerSteps = positionSteps(0);
            beginHomePhase(0, HomePhase::FAR_BACKOFF);
        } else if (!axes[0].moving) {
            fail("Другий кінцевик X не знайдено в межах максимальної довжини");
        }
        break;
    case HomePhase::FAR_BACKOFF:
        if (!axes[0].moving) {
            if (farLimitActive(0)) { fail("Другий кінцевик X не відпускається після відкату"); return; }
            // Робоча довжина — від першого кінцевика до відкату від другого
            const float length = measuredTriggerSteps / cfg.stepsPerMmX - cfg.homingBackoffMm;
            if (length < 10.0f) { fail("Виміряна довжина X надто мала. Перевірте кінцевики та кроки/мм"); return; }
            cfg.xLengthMm = length;
            cfg.xLengthMeasured = true;
            Preferences p;
            p.begin("railway", false);
            p.putFloat("xlen", cfg.xLengthMm);
            p.putBool("xmeas", true);
            p.end();
            targetMm[0] = positionSteps(0) / cfg.stepsPerMmX;
            homingIndex = 1;
            updateLimits();
            beginHomePhase(1, limitActive(1) ? HomePhase::RELEASE : HomePhase::SEEK);
        }
        break;
    default:
        break;
    }
}
void RailwayController::controlStep(float dt) {
    updateLimits();
    if (homingActive) updateHoming();
    controlAxis(0, dt);
    controlAxis(1, dt);

    if (state == RailwayState::ERROR || state == RailwayState::STOPPED) return;
    if (homingActive) state = RailwayState::HOMING;
    else if (axes[0].moving || axes[1].moving) state = RailwayState::MOVING;
    else state = RailwayState::IDLE;
}

RailwayStatus RailwayController::getStatus() {
    RailwayStatus st;
    xSemaphoreTake(mutex, portMAX_DELAY);
    st.state = state;
    switch (state) {
    case RailwayState::IDLE: st.stateStr = "IDLE"; break;
    case RailwayState::MOVING: st.stateStr = "MOVING"; break;
    case RailwayState::HOMING: st.stateStr = "HOMING"; break;
    case RailwayState::STOPPED: st.stateStr = "STOPPED"; break;
    case RailwayState::ERROR: st.stateStr = "ERROR"; break;
    }
    st.xMm = positionSteps(0) / cfg.stepsPerMmX;
    st.zMm = positionSteps(1) / cfg.stepsPerMmZ;
    st.targetXMm = targetMm[0];
    st.targetZMm = targetMm[1];
    const float vx = axes[0].moving ? axes[0].speed / cfg.stepsPerMmX : 0.0f;
    const float vz = axes[1].moving ? axes[1].speed / cfg.stepsPerMmZ : 0.0f;
    st.speedMmS = sqrtf(vx * vx + vz * vz);
    st.homedX = homed[0];
    st.homedZ = homed[1];
    st.limitX = limits[0];
    st.limitXFar = limits[1];
    st.limitZ = limits[2];
    st.enabled = outputsEnabled;
    st.xLengthMeasured = cfg.xLengthMeasured;
    strlcpy(st.errorMessage, errorMsg, sizeof(st.errorMessage));
    xSemaphoreGive(mutex);
    return st;
}

bool RailwayController::startHoming(String& error) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (busy()) {
        xSemaphoreGive(mutex);
        error = "Каретка вже рухається";
        return false;
    }
    if (!outputsEnabled) {
        xSemaphoreGive(mutex);
        error = "Драйвери вимкнено";
        return false;
    }
    errorMsg[0] = 0;
    homed[0] = homed[1] = false;
    homingActive = true;
    homingIndex = 0;
    state = RailwayState::HOMING;
    updateLimits();
    beginHomePhase(0, limitActive(0) ? HomePhase::RELEASE : HomePhase::SEEK);
    xSemaphoreGive(mutex);
    return true;
}

bool RailwayController::moveTo(float xMm, float zMm, float speedMmS, String& error) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    bool ok = false;
    do {
        if (busy()) { error = "Каретка вже рухається"; break; }
        if (!outputsEnabled) { error = "Драйвери вимкнено"; break; }
        if (!cfg.ignoreHoming && (!homed[0] || !homed[1])) { error = "Спочатку виконайте homing"; break; }
        if (!isfinite(xMm) || !isfinite(zMm) || xMm < 0 || xMm > cfg.xLengthMm ||
            zMm < 0 || zMm > cfg.zHeightMm) {
            error = "Координати поза межами стіни";
            break;
        }
        const float v = constrain(speedMmS > 0 ? speedMmS : cfg.maxSpeedMmS, 1.0f, RW_MAX_SPEED_LIMIT_MM_S);
        const float dxMm = xMm - positionSteps(0) / cfg.stepsPerMmX;
        const float dzMm = zMm - positionSteps(1) / cfg.stepsPerMmZ;
        const float dist = sqrtf(dxMm * dxMm + dzMm * dzMm);
        errorMsg[0] = 0;
        targetMm[0] = xMm;
        targetMm[1] = zMm;
        ok = true;
        if (dist < 0.001f) { state = RailwayState::IDLE; break; }
        const int32_t sx = (int32_t)lroundf(xMm * cfg.stepsPerMmX) - positionSteps(0);
        const int32_t sz = (int32_t)lroundf(zMm * cfg.stepsPerMmZ) - positionSteps(1);
        updateLimits();
        const float kx = fabsf(dxMm) / dist;
        const float kz = fabsf(dzMm) / dist;
        const float accel = cfg.accelMmS2;
        if (sx != 0) startMove(0, sx, v * kx * cfg.stepsPerMmX, accel * kx * cfg.stepsPerMmX);
        if (sz != 0) startMove(1, sz, v * kz * cfg.stepsPerMmZ, accel * kz * cfg.stepsPerMmZ);
        state = RailwayState::MOVING;
    } while (false);
    xSemaphoreGive(mutex);
    return ok;
}

bool RailwayController::jog(char axis, float distanceMm, float speedMmS, String& error) {
    const uint8_t a = (axis == 'x' || axis == 'X') ? 0 : 1;
    if (axis != 'x' && axis != 'X' && axis != 'z' && axis != 'Z') {
        error = "Невідома вісь (очікується x або z)";
        return false;
    }
    xSemaphoreTake(mutex, portMAX_DELAY);
    bool ok = false;
    do {
        if (busy()) { error = "Каретка вже рухається"; break; }
        if (!outputsEnabled) { error = "Драйвери вимкнено"; break; }
        if (!isfinite(distanceMm) || fabsf(distanceMm) < 0.01f || fabsf(distanceMm) > lengthMm(a)) {
            error = "Некоректна відстань";
            break;
        }
        const float spm = stepsPerMm(a);
        const float curMm = positionSteps(a) / spm;
        if (homed[a] && (curMm + distanceMm < -0.001f || curMm + distanceMm > lengthMm(a) + 0.001f)) {
            error = "Рух вийде за межі осі";
            break;
        }
        const float v = constrain(speedMmS > 0 ? speedMmS : cfg.maxSpeedMmS, 1.0f, RW_MAX_SPEED_LIMIT_MM_S);
        errorMsg[0] = 0;
        updateLimits();
        if (homed[a]) targetMm[a] = curMm + distanceMm;
        startMove(a, (int32_t)lroundf(distanceMm * spm), v * spm, cfg.accelMmS2 * spm);
        state = RailwayState::MOVING;
        ok = true;
    } while (false);
    xSemaphoreGive(mutex);
    return ok;
}

void RailwayController::stop() {
    xSemaphoreTake(mutex, portMAX_DELAY);
    stopAll();
    errorMsg[0] = 0;
    state = RailwayState::STOPPED;
    // Після зупинки очікуваною вважається поточна позиція
    targetMm[0] = positionSteps(0) / cfg.stepsPerMmX;
    targetMm[1] = positionSteps(1) / cfg.stepsPerMmZ;
    xSemaphoreGive(mutex);
}

bool RailwayController::zeroHere(String& error) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (busy()) {
        xSemaphoreGive(mutex);
        error = "Спочатку зупиніть рух";
        return false;
    }
    portENTER_CRITICAL(&g_mux);
    g_axis[0].pos = 0;
    g_axis[1].pos = 0;
    portEXIT_CRITICAL(&g_mux);
    homed[0] = homed[1] = true;
    targetMm[0] = targetMm[1] = 0;
    errorMsg[0] = 0;
    state = RailwayState::IDLE;
    xSemaphoreGive(mutex);
    return true;
}

void RailwayController::setEnabled(bool enabled) {
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (!enabled) {
        stopAll();
        homed[0] = homed[1] = false;   // Без утримання позиція не гарантується
        state = RailwayState::STOPPED;
    }
    outputsEnabled = enabled;
    g_driversEnabled = enabled;
    xSemaphoreGive(mutex);
}

# Прошивка для управления поворотным столом на ESP32 (PlatformIO)

Прошивка для ESP32, управляющая поворотным/вращающимся столом с шаговым двигателем и концевым выключателем (endstop) через HTTP REST API с форматом данных JSON, а также через встроенный адаптивный веб-интерфейс.

---

## Возможности

- **Калибровка 0° (Homing)**:
  - Автоматически при включении устройства (настраивается).
  - По команде через Web API (`POST /api/home`).
  - Поворот стола влево (против часовой стрелки) до упора в концевик с двухэтапным подходом (быстрый подход, отскок назад, медленный точный подход) для идеальной повторяемости нуля.
- **Управление поворотом**:
  - Поворот на заданный градус с заданной скоростью (`°/сек`).
  - Поддержка абсолютного угла (например, перейти в 90.0°) и относительного смещения (например, повернуть еще на +45.0°).
  - Плавный разгон и торможение с контролем ускорения.
- **Высокая плавность и надежность**:
  - Генерация импульсов шагового двигателя вынесена в отдельный высокоприоритетный поток FreeRTOS на Core 1, что исключает рывки при обработке HTTP-запросов.
  - Безопасный таймаут поиска концевика (защита от зацикливания при обрыве провода концевика).
- **Wi-Fi и mDNS**:
  - Подключение к домашней/рабочей сети Wi-Fi.
  - При отсутствии сети или сбоях — автоматический запуск собственной точки доступа `RotatingTable-ESP32` (IP: `192.168.4.1`).
  - Доступ по имени: `http://rotating-table.local`.
- **Встроенная веб-панель**:
  - Стильный dark-mode веб-интерфейс для управления со смартфона или ПК прямо из браузера (кнопки пресетов углов, ползунок скорости, статус концевика в реальном времени, аварийная остановка).

---

## Схема подключения пинов (По умолчанию)

Настройки пинов находятся в файле [`include/config.h`](file:///include/config.h):

| Назначение | Пин ESP32 | Подключение к драйверу / модулю |
| :--- | :--- | :--- |
| **STEP** | `GPIO 18` | Вход `STEP` драйвера шаговика (A4988 / TMC2209 / DRV8825) |
| **DIR** | `GPIO 19` | Вход `DIR` драйвера шаговика |
| **ENABLE** | `GPIO 5` | Вход `EN` драйвера (активный уровень LOW, можно отключить в конфиге) |
| **ENDSTOP** | `GPIO 4` | Концевик: один контакт на `GPIO 4`, второй на `GND` (используется `INPUT_PULLUP`) |

> **Примечание по концевику**: По умолчанию используется нормально-разомкнутый контакт (NO), замыкающий пин на `GND` при нажатии (`ENDSTOP_ACTIVE_LOW = true`). Если у вас оптический датчик или индуктивный датчик с активным HIGH, просто измените `ENDSTOP_ACTIVE_LOW = false` в `config.h`.

---

## Настройка параметров механики

В файле [`include/config.h`](file:///include/config.h) настройте кинематику вашего стола:

```cpp
constexpr float STEPS_PER_MOTOR_REV = 200.0f; // Шагов двигателя на 360° (200 для 1.8°, 400 для 0.9°)
constexpr float MICROSTEPS          = 16.0f;  // Микрошаг драйвера (1, 2, 4, 8, 16, 32...)
constexpr float GEAR_RATIO          = 1.0f;   // 1.0 = прямой привод; если редуктор 4:1, укажите 4.0
```

Формула расчета шагов на 1 градус:
$$\text{STEPS\_PER\_DEGREE} = \frac{\text{STEPS\_PER\_MOTOR\_REV} \times \text{MICROSTEPS} \times \text{GEAR\_RATIO}}{360}$$

---

## Документация REST Web API (JSON)

Базовый URL: `http://<ESP32_IP>` или `http://rotating-table.local`.

### 1. Поиск концевика (Homing)
Запускает вращение влево до концевика и обнуляет систему координат.

- **URL**: `/api/home`
- **Метод**: `POST`
- **Тело запроса**: пустое `{}`
- **Ответ**:
  ```json
  {
    "status": "ok",
    "message": "Homing initiated"
  }
  ```

Пример cURL:
```bash
curl -X POST http://rotating-table.local/api/home -H "Content-Type: application/json" -d "{}"
```

---

### 2. Поворот стола (Move)
Поворачивает стол на заданный угол с указанной скоростью.

- **URL**: `/api/move`
- **Метод**: `POST`
- **Тело запроса (JSON)**:
  ```json
  {
    "angle": 90.0,
    "speed": 45.0,
    "relative": false
  }
  ```
  - `angle` *(float, обязательно)*: угол в градусах.
  - `speed` *(float, опционально)*: скорость вращения в градусах в секунду (по умолчанию 30°/с, максимум 180°/с).
  - `relative` *(bool, опционально)*: `false` — абсолютная позиция от 0°, `true` — относительный поворот на указанный угол от текущего положения.

- **Ответ**:
  ```json
  {
    "status": "ok",
    "message": "Move command accepted",
    "target_angle": 90.0,
    "speed": 45.0,
    "relative": false
  }
  ```

Пример cURL:
```bash
curl -X POST http://rotating-table.local/api/move \
  -H "Content-Type: application/json" \
  -d '{"angle": 180.0, "speed": 60.0, "relative": false}'
```

---

### 3. Опрос состояния (Status)
Возвращает текущие координаты стола, статус калибровки и состояние концевика.

- **URL**: `/api/status`
- **Метод**: `GET`
- **Ответ**:
  ```json
  {
    "status": "ok",
    "state": "IDLE",
    "current_angle": 90.0,
    "target_angle": 90.0,
    "speed": 60.0,
    "is_homed": true,
    "endstop_triggered": false
  }
  ```
  Возможные значения `state`:
  - `IDLE` — стол неподвижен, готов к командам.
  - `HOMING` — идет процесс поиска концевика.
  - `MOVING` — стол выполняет поворот.
  - `STOPPED` — аварийная остановка.
  - `ERROR` — ошибка (например, таймаут концевика).

Пример cURL:
```bash
curl http://rotating-table.local/api/status
```

---

### 4. Экстренная остановка (Stop)
Немедленно прерывает любое движение стола.

- **URL**: `/api/stop`
- **Метод**: `POST`
- **Ответ**:
  ```json
  {
    "status": "ok",
    "message": "Motor stopped"
  }
  ```

Пример cURL:
```bash
curl -X POST http://rotating-table.local/api/stop
```

---

### 5. Ручной сброс в 0° (Zero)
Устанавливает текущее положение стола как 0.0° без движения двигателя.

- **URL**: `/api/zero`
- **Метод**: `POST`

---

## Пример управления на Python

```python
import requests
import time

BASE_URL = "http://rotating-table.local"

def home_table():
    print("Калибровка нуля...")
    requests.post(f"{BASE_URL}/api/home")
    while True:
        st = requests.get(f"{BASE_URL}/api/status").json()
        if st["state"] != "HOMING":
            print(f"Готово! Состояние: {st['state']}")
            break
        time.sleep(0.5)

def rotate_to(angle, speed=45.0):
    print(f"Поворот на {angle}° со скоростью {speed}°/с...")
    requests.post(f"{BASE_URL}/api/move", json={
        "angle": angle,
        "speed": speed,
        "relative": False
    })
    while True:
        st = requests.get(f"{BASE_URL}/api/status").json()
        if st["state"] == "IDLE":
            print(f"Достигнут угол: {st['current_angle']}°")
            break
        time.sleep(0.3)

if __name__ == "__main__":
    home_table()
    # Пример 360-градусной фотосъемки с шагом 45 градусов:
    for deg in range(0, 360, 45):
        rotate_to(deg)
        print(f"Делаем снимок на {deg}°...")
        time.sleep(1)
```

---

## Сборка и прошивка через PlatformIO

1. Откройте папку проекта в **VS Code** с установленным расширением **PlatformIO IDE**.
2. В файле [`include/config.h`](file:///include/config.h) укажите ваши имя сети и пароль Wi-Fi:
   ```cpp
   #define WIFI_SSID     "Ваш_WiFi"
   #define WIFI_PASSWORD "Ваш_Пароль"
   ```
3. Подключите ESP32 к компьютеру по USB.
4. Нажмите кнопку **Upload** (стрелочка внизу) или выполните в терминале:
   ```bash
   pio run -t upload
   ```
5. Откройте монитор порта на скорости **115200**:
   ```bash
   pio device monitor
   ```

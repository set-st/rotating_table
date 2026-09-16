#!/usr/bin/env python3
"""
Пример клиентского скрипта для управления поворотным столом через Web API.
Требуется: pip install requests
"""

import time
import requests

BASE_URL = "http://rotating-table.local"  # Или укажите IP адрес: "http://192.168.1.100"

def get_status():
    """Получить статус устройства."""
    try:
        r = requests.get(f"{BASE_URL}/api/status", timeout=2)
        return r.json()
    except Exception as e:
        print(f"Ошибка запроса статуса: {e}")
        return None

def wait_for_idle(interval=0.3):
    """Ожидание завершения движения/калибровки."""
    while True:
        st = get_status()
        if not st:
            time.sleep(interval)
            continue
        
        state = st.get("state")
        angle = st.get("current_angle", 0.0)
        print(f"  [Статус: {state}] Текущий угол: {angle:.1f}°")

        if state == "IDLE":
            return True
        elif state in ("STOPPED", "ERROR"):
            print(f"Внимание: стол перешел в состояние {state} ({st.get('error', '')})")
            return False

        time.sleep(interval)

def home_table():
    """Калибровка 0°: поиск концевика влево до упора."""
    print("=== Запуск калибровки нуля (Homing)... ===")
    r = requests.post(f"{BASE_URL}/api/home")
    print("Ответ:", r.json())
    return wait_for_idle()

def rotate_to(angle, speed=45.0, relative=False):
    """Поворот стола на указанный угол со скоростью."""
    mode_str = "относительный шаг" if relative else "абсолютная позиция"
    print(f"=== Поворот на {angle}° ({mode_str}), скорость {speed}°/с ===")
    payload = {
        "angle": angle,
        "speed": speed,
        "relative": relative
    }
    r = requests.post(f"{BASE_URL}/api/move", json=payload)
    print("Ответ:", r.json())
    return wait_for_idle()

def emergency_stop():
    """Экстренная остановка."""
    print("=== Экстренная остановка! ===")
    r = requests.post(f"{BASE_URL}/api/stop")
    print("Ответ:", r.json())

if __name__ == "__main__":
    print(f"Подключение к {BASE_URL}...")
    st = get_status()
    if not st:
        print("Не удалось подключиться к столу. Проверьте сеть и питание ESP32.")
        exit(1)

    print(f"Подключено! Состояние: {st['state']}, Угол: {st['current_angle']}°")

    # 1. Поиск концевика (калибровка 0°)
    home_table()

    # 2. Пример съемки объекта на 360° с шагом 90°
    for step_angle in [0, 90, 180, 270, 360]:
        rotate_to(step_angle, speed=40.0)
        print(f"-> Стол зафиксирован на {step_angle}°. Делаем снимок...")
        time.sleep(1.0)

    print("Цикл завершен!")

#!/usr/bin/env python3
"""
Приклад клієнтського скрипта для керування поворотним столом через Web API.
Потрібно встановити: pip install requests
"""

import time
import requests

BASE_URL = "http://rotating-table.local"  # Або вкажіть IP адресу: "http://192.168.1.100"

def get_status():
    """Отримати статус пристрою."""
    try:
        r = requests.get(f"{BASE_URL}/api/status", timeout=2)
        return r.json()
    except Exception as e:
        print(f"Помилка запиту статусу: {e}")
        return None

def wait_for_idle(interval=0.3):
    """Очікування завершення руху/калібрування."""
    while True:
        st = get_status()
        if not st:
            time.sleep(interval)
            continue
        
        state = st.get("state")
        angle = st.get("current_angle", 0.0)
        print(f"  [Статус: {state}] Поточний кут: {angle:.1f}°")

        if state == "IDLE":
            return True
        elif state in ("STOPPED", "ERROR"):
            print(f"Увага: стіл перейшов у стан {state} ({st.get('error', '')})")
            return False

        time.sleep(interval)

def home_table():
    """Калібрування 0°: пошук кінцевика вліво до упору."""
    print("=== Запуск калібрування нуля (Homing)... ===")
    r = requests.post(f"{BASE_URL}/api/home")
    print("Відповідь:", r.json())
    return wait_for_idle()

def rotate_to(angle, speed=45.0, relative=False):
    """Поворот столу на вказаний кут зі швидкістю."""
    mode_str = "відносний крок" if relative else "абсолютна позиція"
    print(f"=== Поворот на {angle}° ({mode_str}), швидкість {speed}°/с ===")
    payload = {
        "angle": angle,
        "speed": speed,
        "relative": relative
    }
    r = requests.post(f"{BASE_URL}/api/move", json=payload)
    print("Відповідь:", r.json())
    return wait_for_idle()

def emergency_stop():
    """Екстрена зупинка."""
    print("=== Екстрена зупинка! ===")
    r = requests.post(f"{BASE_URL}/api/stop")
    print("Відповідь:", r.json())

if __name__ == "__main__":
    print(f"Підключення до {BASE_URL}...")
    st = get_status()
    if not st:
        print("Не вдалося підключитися до столу. Перевірте мережу та живлення ESP32.")
        exit(1)

    print(f"Підключено! Стан: {st['state']}, Кут: {st['current_angle']}°")

    # 1. Пошук кінцевика (калібрування 0°)
    home_table()

    # 2. Приклад зйомки об'єкта на 360° з кроком 90°
    for step_angle in [0, 90, 180, 270, 360]:
        rotate_to(step_angle, speed=40.0)
        print(f"-> Стіл зафіксовано на {step_angle}°. Робимо знімок...")
        time.sleep(1.0)

    print("Цикл завершено!")

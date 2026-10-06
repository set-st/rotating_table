#!/usr/bin/env python3
"""Local simulator for the Rotating Table and Tilt Platform web interface."""

from __future__ import annotations

import json
import math
import re
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any
from urllib.parse import urlparse


ROOT = Path(__file__).resolve().parent
WEB_UI = ROOT / "include" / "web_ui.h"
HOST = "127.0.0.1"
PORT = 8080
FIRMWARE_VERSION = "simulator-v1.1"


def load_page() -> str:
    source = WEB_UI.read_text(encoding="utf-8")
    match = re.search(r'R"rawliteral\((.*)\)rawliteral";', source, re.DOTALL)
    if not match:
        raise RuntimeError(f"PAGE_INDEX was not found in {WEB_UI}")
    return match.group(1)


class SimulatorState:
    def __init__(self) -> None:
        self.lock = threading.RLock()
        # Поворотний стіл
        self.current_angle = 0.0
        self.target_angle = 0.0
        self.speed = 0.0
        self.state = "IDLE"
        self.is_homed = False
        self.endstop_triggered = False
        self.error = ""
        self.automatic_enabled = False
        self.automatic_angle = 10.0
        self.automatic_speed = 3.0
        self.automatic_interval_ms = 5000
        self.next_automatic_at = 0.0
        self.imu_zero_pitch = 0.0
        self.imu_zero_roll = 0.0
        self.settings: dict[str, Any] = {
            "motor_teeth": 20,
            "table_teeth": 60,
            "steps_per_rev": 200,
            "microsteps": 16,
            "invert_dir": False,
            "dir_positive_high": True,
            "step_active_low": False,
            "enable_active_high": False,
            "pin_step": 18,
            "pin_dir": 19,
            "pin_enable": 5,
            "pin_endstop": 4,
            "pin_button_left": 25,
            "pin_button_right": 26,
            "pin_button_stop": 27,
            "button_left_inverted": False,
            "button_right_inverted": False,
            "button_stop_inverted": False,
            "endstop_inverted": False,
            "endstop_debounce_ms": 10,
            "homing_direction": -1,
            "auto_home_on_boot": False,
            "max_speed": 180.0,
            "acceleration": 90.0,
            "default_move_speed": 30.0,
            "default_move_angle": 10.0,
            "homing_fast_speed": 25.0,
            "homing_backoff_speed": 3.0,
            "homing_slow_speed": 5.0,
            "rotation_limit_deg": 180.0,
        }
        self.wifi = {
            "sta_ssid": "",
            "sta_password": "",
            "ap_ssid": "RotatingTable-ESP32",
            "ap_password": "12345678",
            "mode": "SIMULATOR",
            "ip": "127.0.0.1",
        }

        # Платформа нахилу (Tilt Platform)
        self.tilt_current_angle = 0.0
        self.tilt_motor_angle = 0.0
        self.tilt_target_angle = 0.0
        self.tilt_speed = 0.0
        self.tilt_state = "IDLE"
        self.tilt_is_homed = False
        self.tilt_endstop = False
        self.tilt_hold = False
        self.tilt_connected = True
        self.tilt_rssi = -62
        self.tilt_target_address = "TiltTable-ESP32"
        self.tilt_auto_connect = True
        self.tilt_zero_pitch = 0.0
        self.tilt_zero_roll = 0.0
        self.tilt_error = ""

        self.tilt_settings: dict[str, Any] = {
            "pin_step": 18,
            "pin_dir": 19,
            "pin_enable": 5,
            "step_active_low": True,
            "dir_positive_high": False,
            "enable_active_high": False,
            "pin_endstop": 4,
            "endstop_inverted": False,
            "endstop_debounce_ms": 10,
            "pin_sda": 21,
            "pin_scl": 22,
            "mpu_addr": 104,
            "tilt_axis": 0,
            "steps_per_rev": 200.0,
            "microsteps": 16.0,
            "gear_ratio": 3.0,
            "min_angle": -45.0,
            "max_angle": 45.0,
            "def_speed": 5.0,
            "max_speed": 30.0,
            "accel": 25.0,
            "homing_dir": -1,
            "homing_fast_speed": 8.0,
            "homing_slow_speed": 2.0,
            "homing_backoff_deg": 3.0,
            "auto_home": False,
            "hold_deadband": 0.2,
            "hold_kp": 2.5,
        }

    def update(self) -> None:
        now = time.monotonic()
        with self.lock:
            # 1. Оновлення поворотного столу
            if self.state == "MOVING":
                delta = self.target_angle - self.current_angle
                step = max(self.speed, 1.0) * 0.05
                if abs(delta) <= step:
                    self.current_angle = self.target_angle
                    self.speed = 0.0
                    self.state = "IDLE"
                else:
                    self.current_angle += step if delta > 0 else -step
            if self.automatic_enabled and self.state == "IDLE" and now >= self.next_automatic_at:
                next_angle = self.current_angle + self.automatic_angle
                if self.is_homed:
                    limit = float(self.settings["rotation_limit_deg"])
                    if next_angle >= limit:
                        self.automatic_angle = -abs(self.automatic_angle)
                    elif next_angle <= -limit:
                        self.automatic_angle = abs(self.automatic_angle)
                    next_angle = max(-limit, min(limit, next_angle))
                self.target_angle = next_angle
                self.speed = self.automatic_speed
                self.state = "MOVING"
                self.next_automatic_at = now + self.automatic_interval_ms / 1000.0

            # 2. Оновлення платформи нахилу
            if self.tilt_state == "MOVING":
                t_delta = self.tilt_target_angle - self.tilt_motor_angle
                t_step = max(self.tilt_speed, 1.0) * 0.05
                if abs(t_delta) <= t_step:
                    self.tilt_motor_angle = self.tilt_target_angle
                    self.tilt_speed = 0.0
                    self.tilt_state = "HOLDING" if self.tilt_hold else "IDLE"
                else:
                    self.tilt_motor_angle += t_step if t_delta > 0 else -t_step
                self.tilt_current_angle = self.tilt_motor_angle
            elif self.tilt_hold:
                self.tilt_current_angle = self.tilt_target_angle + 0.02 * math.sin(now * 3)

    def status(self) -> dict[str, Any]:
        self.update()
        with self.lock:
            return {
                "status": "ok",
                "firmware_version": FIRMWARE_VERSION,
                "state": self.state,
                "current_angle": self.current_angle,
                "target_angle": self.target_angle,
                "speed": self.speed,
                "is_homed": self.is_homed,
                "endstop_triggered": self.endstop_triggered,
                "automatic_enabled": self.automatic_enabled,
                "automatic_angle": self.automatic_angle,
                "automatic_speed": self.automatic_speed,
                "automatic_interval_ms": self.automatic_interval_ms,
                "imu_initialized": True,
                "imu_mpu6050_connected": True,
                "imu_barometer_connected": True,
                "imu_pitch_deg": 0.8 * math.sin(time.monotonic() / 2) - self.imu_zero_pitch,
                "imu_roll_deg": 0.6 * math.cos(time.monotonic() / 2) - self.imu_zero_roll,
                "imu_gyro_x_dps": 0.0,
                "imu_gyro_y_dps": 0.0,
                "imu_gyro_z_dps": 0.0,
                "imu_updated_ms": int(time.monotonic() * 1000),
                "error": self.error,
            }

    def tilt_status(self) -> dict[str, Any]:
        self.update()
        with self.lock:
            pitch = self.tilt_current_angle + 0.1 * math.sin(time.monotonic()) - self.tilt_zero_pitch
            roll = 0.05 * math.cos(time.monotonic()) - self.tilt_zero_roll
            return {
                "status": "ok",
                "connected": self.tilt_connected,
                "firmware_version": "simulator",
                "rssi": self.tilt_rssi,
                "target_address": self.tilt_target_address,
                "auto_connect": self.tilt_auto_connect,
                "state": self.tilt_state,
                "angle": round(self.tilt_current_angle, 2),
                "motor_angle": round(self.tilt_motor_angle, 2),
                "target_angle": round(self.tilt_target_angle, 2),
                "speed": round(self.tilt_speed, 1),
                "is_homed": self.tilt_is_homed,
                "endstop_triggered": self.tilt_endstop,
                "hold_active": self.tilt_hold,
                "gyro_connected": True,
                "pitch": round(pitch, 2),
                "roll": round(roll, 2),
                "error": self.tilt_error,
            }

    def move(self, body: dict[str, Any]) -> None:
        with self.lock:
            angle = float(body.get("angle", 0))
            speed = max(float(body.get("speed", self.settings["default_move_speed"])), 1.0)
            self.target_angle = self.current_angle + angle if body.get("relative") else angle
            if self.is_homed:
                limit = float(self.settings["rotation_limit_deg"])
                self.target_angle = max(-limit, min(limit, self.target_angle))
            self.speed = speed
            self.state = "MOVING"

    def move_tilt(self, body: dict[str, Any]) -> None:
        with self.lock:
            angle = float(body.get("angle", 0))
            speed = max(float(body.get("speed", self.tilt_settings["def_speed"])), 1.0)
            min_a = float(self.tilt_settings["min_angle"])
            max_a = float(self.tilt_settings["max_angle"])
            target = self.tilt_motor_angle + angle if body.get("relative") else angle
            self.tilt_target_angle = max(min_a, min(max_a, target))
            self.tilt_speed = speed
            self.tilt_state = "MOVING"


STATE = SimulatorState()


class Handler(BaseHTTPRequestHandler):
    def send_json(self, payload: Any, status: int = 200) -> None:
        data = json.dumps(payload).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()
        self.wfile.write(data)

    def do_OPTIONS(self) -> None:
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def body(self) -> dict[str, Any]:
        length = int(self.headers.get("Content-Length", 0))
        if length == 0:
            return {}
        try:
            return json.loads(self.rfile.read(length).decode("utf-8"))
        except Exception:
            return {}

    def do_GET(self) -> None:
        path = urlparse(self.path).path
        if path in ("/", "/index.html"):
            page = load_page().encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(page)))
            self.end_headers()
            self.wfile.write(page)
        elif path == "/api/status":
            self.send_json(STATE.status())
        elif path == "/api/settings":
            self.send_json({"status": "ok", **STATE.settings})
        elif path == "/api/tilt/status":
            self.send_json(STATE.tilt_status())
        elif path == "/api/tilt/settings":
            self.send_json({"status": "ok", **STATE.tilt_settings})
        elif path == "/api/tilt/bt/scan":
            self.send_json({
                "status": "ok",
                "devices": [
                    {"name": "TiltTable-ESP32", "address": "24:6F:28:B1:A2:34", "rssi": -62},
                    {"name": "ESP32-BLE-Peripheral", "address": "30:AE:A4:05:78:12", "rssi": -78},
                ]
            })
        elif path == "/api/imu/scan":
            self.send_json({"status": "ok", "addresses": [104, 119]})
        elif path == "/api/wifi/config":
            self.send_json({"status": "ok", **STATE.wifi})
        elif path == "/api/wifi/scan":
            self.send_json({"status": "ok", "networks": []})
        elif path == "/api/ota/latest":
            self.send_json({
                "status": "ok",
                "current_version": FIRMWARE_VERSION,
                "tag": FIRMWARE_VERSION,
                "asset": "rotating_table.bin",
                "size": 0,
            })
        elif path == "/api/ota/tilt/latest":
            self.send_json({
                "status": "ok",
                "current_version": "simulator",
                "connected": STATE.tilt_connected,
                "tag": FIRMWARE_VERSION,
                "asset": "tilt_platform.bin",
                "size": 0,
            })
        else:
            self.send_json({"status": "error", "error": "Маршрут не знайдено"}, 404)

    def do_POST(self) -> None:
        path = urlparse(self.path).path
        body = self.body()
        # Поворотний стіл
        if path == "/api/move":
            STATE.move(body)
            self.send_json({"status": "ok"})
        elif path == "/api/home":
            with STATE.lock:
                STATE.current_angle = 0.0
                STATE.target_angle = 0.0
                STATE.speed = 0.0
                STATE.state = "IDLE"
                STATE.is_homed = True
                STATE.endstop_triggered = True
            self.send_json({"status": "ok"})
        elif path == "/api/stop":
            with STATE.lock:
                STATE.target_angle = STATE.current_angle
                STATE.speed = 0.0
                STATE.state = "IDLE"
                STATE.automatic_enabled = False
            self.send_json({"status": "ok", "message": "Двигун зупинено"})
        elif path == "/api/zero":
            with STATE.lock:
                STATE.current_angle = 0.0
                STATE.target_angle = 0.0
            self.send_json({"status": "ok"})
        elif path == "/api/automatic/start":
            with STATE.lock:
                STATE.automatic_angle = float(body.get("angle", 10))
                STATE.automatic_speed = float(body.get("speed", 3))
                STATE.automatic_interval_ms = max(int(body.get("interval_ms", 5000)), 100)
                STATE.automatic_enabled = True
                STATE.next_automatic_at = time.monotonic()
            self.send_json({"status": "ok"})
        elif path == "/api/automatic/stop":
            with STATE.lock:
                STATE.automatic_enabled = False
            self.send_json({"status": "ok"})
        elif path == "/api/settings":
            with STATE.lock:
                STATE.settings.update(body)
            self.send_json({"status": "ok", "reboot_required": False})
        elif path == "/api/imu/calibrate":
            self.send_json({"status": "ok"})
        elif path == "/api/imu/zero":
            with STATE.lock:
                STATE.imu_zero_pitch = 0.8 * math.sin(time.monotonic() / 2)
                STATE.imu_zero_roll = 0.6 * math.cos(time.monotonic() / 2)
            self.send_json({"status": "ok"})

        # Платформа нахилу (Tilt Platform)
        elif path == "/api/tilt/move":
            STATE.move_tilt(body)
            self.send_json({"status": "ok", "message": "Команду нахилу прийнято"})
        elif path == "/api/tilt/home":
            with STATE.lock:
                min_a = float(STATE.tilt_settings["min_angle"])
                STATE.tilt_current_angle = min_a
                STATE.tilt_motor_angle = min_a
                STATE.tilt_target_angle = min_a
                STATE.tilt_speed = 0.0
                STATE.tilt_state = "IDLE"
                STATE.tilt_is_homed = True
                STATE.tilt_endstop = True
            self.send_json({"status": "ok", "message": "Пошук кінцевика виконано"})
        elif path == "/api/tilt/stop":
            with STATE.lock:
                STATE.tilt_target_angle = STATE.tilt_motor_angle
                STATE.tilt_speed = 0.0
                STATE.tilt_state = "IDLE"
                STATE.tilt_hold = False
            self.send_json({"status": "ok", "message": "Нахил зупинено"})
        elif path == "/api/tilt/zero":
            with STATE.lock:
                STATE.tilt_current_angle = 0.0
                STATE.tilt_motor_angle = 0.0
                STATE.tilt_target_angle = 0.0
                STATE.tilt_is_homed = True
            self.send_json({"status": "ok", "message": "Встановлено 0.0°"})
        elif path == "/api/tilt/gyro/zero":
            with STATE.lock:
                STATE.tilt_zero_pitch = STATE.tilt_current_angle
                STATE.tilt_zero_roll = 0.0
                STATE.tilt_current_angle = 0.0
            self.send_json({"status": "ok", "message": "Нуль гіроскопа збережено"})
        elif path == "/api/tilt/gyro/calibrate":
            self.send_json({"status": "ok", "message": "Гіроскоп відкалібровано"})
        elif path == "/api/tilt/hold":
            with STATE.lock:
                STATE.tilt_hold = bool(body.get("enabled", False))
                STATE.tilt_state = "HOLDING" if STATE.tilt_hold else "IDLE"
            self.send_json({"status": "ok", "message": "Режим стабілізації оновлено"})
        elif path == "/api/tilt/settings":
            with STATE.lock:
                STATE.tilt_settings.update(body)
            self.send_json({"status": "ok", "message": "Налаштування збережено"})
        elif path == "/api/tilt/bt/connect":
            addr = body.get("address", "")
            disconn = body.get("disconnect", False)
            with STATE.lock:
                if disconn:
                    STATE.tilt_connected = False
                    STATE.tilt_state = "OFFLINE"
                else:
                    STATE.tilt_connected = True
                    if addr:
                        STATE.tilt_target_address = addr
                    STATE.tilt_state = "IDLE"
            self.send_json({"status": "ok", "message": "Статус Bluetooth оновлено"})

        # Wi-Fi та система
        elif path == "/api/wifi/save":
            with STATE.lock:
                STATE.wifi.update(body)
            self.send_json({"status": "ok", "reboot_required": False})
        elif path == "/api/wifi/reset":
            self.send_json({"status": "ok"})
        elif path == "/api/ota/update":
            self.send_json({"status": "error", "message": "OTA недоступно в симуляторі"}, 409)
        elif path == "/api/ota/tilt/update":
            self.send_json({"status": "error", "message": "BLE OTA недоступно в симуляторі"}, 409)
        else:
            self.send_json({"status": "error", "error": "Маршрут не знайдено"}, 404)


def main() -> None:
    server = ThreadingHTTPServer((HOST, PORT), Handler)
    print(f"Симулятор поворотного та нахильного столів запущено: http://{HOST}:{PORT}/")
    print("Для зупинки натисніть Ctrl+C.")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nСимулятор зупинено.")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()

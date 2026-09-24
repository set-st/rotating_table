#!/usr/bin/env python3
"""Local simulator for the Rotating Table web interface."""

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


def load_page() -> str:
    source = WEB_UI.read_text(encoding="utf-8")
    match = re.search(r'R"rawliteral\((.*)\)rawliteral";', source, re.DOTALL)
    if not match:
        raise RuntimeError(f"PAGE_INDEX was not found in {WEB_UI}")
    return match.group(1)


class SimulatorState:
    def __init__(self) -> None:
        self.lock = threading.RLock()
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
            "pin_step": 25,
            "pin_dir": 26,
            "pin_enable": 27,
            "pin_endstop": 33,
            "pin_button_left": 34,
            "pin_button_right": 35,
            "pin_button_stop": 32,
            "button_left_inverted": False,
            "button_right_inverted": False,
            "button_stop_inverted": False,
            "endstop_inverted": False,
            "endstop_debounce_ms": 30,
            "homing_direction": -1,
            "auto_home_on_boot": False,
            "max_speed": 180.0,
            "acceleration": 90.0,
            "default_move_speed": 30.0,
            "default_move_angle": 10.0,
            "homing_fast_speed": 25.0,
            "homing_backoff_speed": 3.0,
            "homing_slow_speed": 5.0,
        }
        self.wifi = {
            "sta_ssid": "",
            "sta_password": "",
            "ap_ssid": "RotatingTable-ESP32",
            "ap_password": "12345678",
            "mode": "SIMULATOR",
            "ip": "127.0.0.1",
        }

    def update(self) -> None:
        now = time.monotonic()
        with self.lock:
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
                self.target_angle = self.current_angle + self.automatic_angle
                self.speed = self.automatic_speed
                self.state = "MOVING"
                self.next_automatic_at = now + self.automatic_interval_ms / 1000.0

    def status(self) -> dict[str, Any]:
        self.update()
        with self.lock:
            return {
                "status": "ok",
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

    def move(self, body: dict[str, Any]) -> None:
        with self.lock:
            angle = float(body.get("angle", 0))
            speed = max(float(body.get("speed", self.settings["default_move_speed"])), 1.0)
            self.target_angle = self.current_angle + angle if body.get("relative") else angle
            self.speed = min(speed, float(self.settings["max_speed"]))
            self.state = "MOVING"
            self.error = ""


STATE = SimulatorState()


class Handler(BaseHTTPRequestHandler):
    server_version = "RotatingTableSimulator/1.0"

    def log_message(self, fmt: str, *args: Any) -> None:
        print(f"[sim] {self.address_string()} - {fmt % args}")

    def send_json(self, payload: dict[str, Any], status: int = 200) -> None:
        data = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def body(self) -> dict[str, Any]:
        length = int(self.headers.get("Content-Length", "0"))
        if length == 0:
            return {}
        return json.loads(self.rfile.read(length).decode("utf-8"))

    def do_OPTIONS(self) -> None:
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def do_GET(self) -> None:
        path = urlparse(self.path).path
        if path == "/":
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
        elif path == "/api/imu/scan":
            self.send_json({"status": "ok", "addresses": [104, 119]})
        elif path == "/api/wifi/config":
            self.send_json({"status": "ok", **STATE.wifi})
        elif path == "/api/wifi/scan":
            self.send_json({"status": "ok", "networks": []})
        elif path == "/api/ota/latest":
            self.send_json({"status": "ok", "tag": "simulator", "asset": "simulator", "size": 0})
        else:
            self.send_json({"status": "error", "error": "Маршрут не знайдено"}, 404)

    def do_POST(self) -> None:
        path = urlparse(self.path).path
        body = self.body()
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
        elif path == "/api/wifi/save":
            with STATE.lock:
                STATE.wifi.update(body)
            self.send_json({"status": "ok", "reboot_required": False})
        elif path == "/api/wifi/reset":
            self.send_json({"status": "ok"})
        elif path == "/api/ota/update":
            self.send_json({"status": "error", "message": "OTA недоступно в симуляторі"}, 409)
        else:
            self.send_json({"status": "error", "error": "Маршрут не знайдено"}, 404)


def main() -> None:
    server = ThreadingHTTPServer((HOST, PORT), Handler)
    print(f"Симулятор запущено: http://{HOST}:{PORT}/")
    print("Для зупинки натисніть Ctrl+C.")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nСимулятор зупинено.")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()

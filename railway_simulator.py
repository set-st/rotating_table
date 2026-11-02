#!/usr/bin/env python3
"""Local simulator for the RailWay web interface (X/Z carriage on MKS DLC32).

Usage: python railway_simulator.py  ->  http://127.0.0.1:8081/
"""

from __future__ import annotations

import json
import math
import re
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parent
WEB_UI = ROOT / "include" / "railway_ui.h"
HOST = "127.0.0.1"
PORT = 8081
FIRMWARE_VERSION = "railway-simulator"
REAL_RAIL_LENGTH_MM = 1200.0   # "Фізична" довжина рейки, яку виміряє homing
BACKOFF_DEFAULT = 5.0


def load_page() -> str:
    source = WEB_UI.read_text(encoding="utf-8")
    match = re.search(r'R"rawliteral\((.*)\)rawliteral";', source, re.DOTALL)
    if not match:
        raise RuntimeError(f"PAGE_RAILWAY was not found in {WEB_UI}")
    return match.group(1)


class Sim:
    def __init__(self) -> None:
        self.lock = threading.RLock()
        self.pos = [REAL_RAIL_LENGTH_MM / 2, 100.0]   # X, Z (мм від кінцевиків; поки без homing — довільно)
        self.homed = [False, False]
        self.state = "IDLE"
        self.error = ""
        self.enabled = True
        self.speed = 0.0
        self.target = [0.0, 0.0]
        self.queue: list[tuple[str, float, float]] = []   # (axis, target_mm, speed)
        self.homing_steps: list[tuple[str, float, float]] = []
        self.on_done = None
        self.settings: dict[str, Any] = {
            "steps_per_mm_x": 80.0, "steps_per_mm_z": 80.0,
            "x_length_mm": 1000.0, "x_length_measured": False,
            "z_height_mm": 500.0, "x_dir_invert": False, "z_dir_invert": False,
            "limit_active_low": True, "z_endstop_top": True, "auto_home_on_boot": False,
            "max_speed_mm_s": 50.0, "accel_mm_s2": 200.0,
            "homing_speed_mm_s": 20.0, "homing_backoff_mm": BACKOFF_DEFAULT,
        }
        self.last = time.monotonic()
        threading.Thread(target=self.loop, daemon=True).start()

    def busy(self) -> bool:
        return self.state in ("MOVING", "HOMING")

    def loop(self) -> None:
        while True:
            time.sleep(0.03)
            with self.lock:
                now = time.monotonic()
                dt = now - self.last
                self.last = now
                self.step(dt)

    def step(self, dt: float) -> None:
        if not self.queue:
            self.speed = 0.0
            return
        axis, tgt, spd = self.queue[0]
        i = 0 if axis == "x" else 1
        d = tgt - self.pos[i]
        move = spd * dt
        self.speed = spd
        if abs(d) <= move:
            self.pos[i] = tgt
            self.queue.pop(0)
            if not self.queue:
                self.speed = 0.0
                if self.on_done:
                    cb, self.on_done = self.on_done, None
                    cb()
                elif self.state == "MOVING":
                    self.state = "IDLE"
        else:
            self.pos[i] += math.copysign(move, d)

    def limits(self) -> tuple[bool, bool, bool]:
        return (self.pos[0] <= 0.01, self.pos[0] >= REAL_RAIL_LENGTH_MM - 0.01, self.pos[1] <= 0.01)

    def status(self) -> dict[str, Any]:
        with self.lock:
            lx, lxf, lz = self.limits()
            s = self.settings
            out = {
                "status": "ok", "firmware_version": FIRMWARE_VERSION, "state": self.state,
                "x_mm": self.pos[0], "z_mm": self.pos[1],
                "target_x_mm": self.target[0], "target_z_mm": self.target[1],
                "speed_mm_s": self.speed,
                "is_homed": all(self.homed), "homed_x": self.homed[0], "homed_z": self.homed[1],
                "limit_x": lx, "limit_x_far": lxf, "limit_z": lz,
                "drivers_enabled": self.enabled,
                "x_length_mm": s["x_length_mm"], "x_length_measured": s["x_length_measured"],
                "z_height_mm": s["z_height_mm"], "z_endstop_top": s["z_endstop_top"],
                "max_speed_mm_s": s["max_speed_mm_s"],
            }
            if self.error:
                out["error"] = self.error
            return out

    def home(self) -> str | None:
        with self.lock:
            if self.busy():
                return "Каретка вже рухається"
            if not self.enabled:
                return "Драйвери вимкнено"
            self.error = ""
            self.homed = [False, False]
            self.state = "HOMING"
            v = self.settings["homing_speed_mm_s"]
            back = self.settings["homing_backoff_mm"]

            def after_far() -> None:
                length = REAL_RAIL_LENGTH_MM - back
                self.settings["x_length_mm"] = round(length, 1)
                self.settings["x_length_measured"] = True
                self.queue = [("z", 0.0, v)]
                self.on_done = after_z

            def after_zero_x() -> None:
                self.homed[0] = True
                self.target[0] = 0.0
                self.queue = [("x", REAL_RAIL_LENGTH_MM, v)]
                self.on_done = after_far_touch

            def after_far_touch() -> None:
                self.queue = [("x", REAL_RAIL_LENGTH_MM - back, v)]
                self.on_done = after_far

            def after_z() -> None:
                self.homed[1] = True
                self.target[1] = 0.0
                self.state = "IDLE"

            self.queue = [("x", 0.0, v)]
            self.on_done = after_zero_x
            return None

    def move_to(self, x: float, z: float, speed: float) -> str | None:
        with self.lock:
            if self.busy():
                return "Каретка вже рухається"
            if not self.enabled:
                return "Драйвери вимкнено"
            if not all(self.homed):
                return "Спочатку виконайте homing"
            s = self.settings
            if not (0 <= x <= s["x_length_mm"] and 0 <= z <= s["z_height_mm"]):
                return "Координати поза межами стіни"
            v = min(max(speed or s["max_speed_mm_s"], 1.0), 100.0)
            self.target = [x, z]
            self.queue = [("x", x, v), ("z", z, v)]
            self.state = "MOVING"
            return None

    def jog(self, axis: str, dist: float) -> str | None:
        with self.lock:
            if axis not in ("x", "z"):
                return "Невідома вісь (очікується x або z)"
            if self.busy():
                return "Каретка вже рухається"
            i = 0 if axis == "x" else 1
            limit = self.settings["x_length_mm"] if i == 0 else self.settings["z_height_mm"]
            if self.homed[i] and not 0 <= self.pos[i] + dist <= limit:
                return "Рух вийде за межі осі"
            self.queue = [(axis, self.pos[i] + dist, self.settings["max_speed_mm_s"])]
            self.state = "MOVING"
            return None


sim = Sim()
PAGE = load_page().encode("utf-8")


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *args: Any) -> None:
        pass

    def send_json(self, payload: Any, status: int = 200) -> None:
        raw = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(raw)))
        self.end_headers()
        self.wfile.write(raw)

    def result(self, err: str | None, **extra: Any) -> None:
        if err:
            self.send_json({"status": "error", "error": err}, 409)
        else:
            self.send_json({"status": "ok", **extra})

    def body(self) -> dict[str, Any]:
        n = int(self.headers.get("Content-Length", 0))
        try:
            return json.loads(self.rfile.read(n) or b"{}")
        except json.JSONDecodeError:
            return {}

    def do_GET(self) -> None:
        path = self.path.split("?")[0]
        if path == "/":
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.end_headers()
            self.wfile.write(PAGE)
        elif path == "/api/status":
            self.send_json(sim.status())
        elif path == "/api/settings":
            self.send_json({"status": "ok", **sim.settings})
        elif path == "/api/wifi/config":
            self.send_json({"status": "ok", "mode": "STA", "ip": HOST, "connected": True,
                            "current_ssid": "simulator", "sta_ssid": "simulator", "ap_ssid": "RailWay-ESP32"})
        elif path == "/api/wifi/scan":
            self.send_json({"status": "ok", "networks": [{"ssid": "simulator", "rssi": -50, "secure": True}]})
        elif path == "/api/ota/latest":
            self.send_json({"status": "ok", "current_version": FIRMWARE_VERSION,
                            "tag": "v0.0.0-sim", "asset": "railway.bin", "size": 1048576})
        else:
            self.send_json({"status": "error", "error": "Ендпоінт не знайдено"}, 404)

    def do_POST(self) -> None:
        path = self.path.split("?")[0]
        b = self.body()
        if path == "/api/home":
            self.result(sim.home())
        elif path == "/api/move":
            if not all(isinstance(b.get(k), (int, float)) for k in ("x", "z")):
                self.send_json({"status": "error", "error": "Потрібні числові поля x та z (мм)"}, 400)
            else:
                self.result(sim.move_to(float(b["x"]), float(b["z"]), float(b.get("speed", 0))))
        elif path == "/api/jog":
            if not isinstance(b.get("distance"), (int, float)):
                self.send_json({"status": "error", "error": "Потрібні поля axis та distance"}, 400)
            else:
                self.result(sim.jog(str(b.get("axis", "")).lower(), float(b["distance"])))
        elif path == "/api/stop":
            with sim.lock:
                sim.queue, sim.on_done, sim.state = [], None, "STOPPED"
            self.send_json({"status": "ok"})
        elif path == "/api/zero":
            with sim.lock:
                if sim.busy():
                    return self.result("Спочатку зупиніть рух")
                sim.pos, sim.homed, sim.state = [0.0, 0.0], [True, True], "IDLE"
            self.result(None)
        elif path == "/api/enable":
            with sim.lock:
                sim.enabled = bool(b.get("enabled", True))
                if not sim.enabled:
                    sim.queue, sim.on_done, sim.homed, sim.state = [], None, [False, False], "STOPPED"
            self.result(None)
        elif path == "/api/settings":
            with sim.lock:
                if sim.busy():
                    return self.result("Зупиніть рух перед зміною налаштувань")
                old = dict(sim.settings)
                for k, v in b.items():
                    if k in sim.settings and k != "x_length_measured":
                        sim.settings[k] = v
                geometry = any(old[k] != sim.settings[k] for k in
                               ("steps_per_mm_x", "steps_per_mm_z", "x_dir_invert", "z_dir_invert"))
                if geometry:
                    sim.homed = [False, False]
                if geometry or old["x_length_mm"] != sim.settings["x_length_mm"]:
                    sim.settings["x_length_measured"] = False
            self.result(None, message="Налаштування збережено")
        elif path.startswith("/api/wifi/") or path == "/api/ota/update":
            self.send_json({"status": "ok", "message": "Симулятор: дію ігноровано"})
        else:
            self.send_json({"status": "error", "error": "Ендпоінт не знайдено"}, 404)

    def do_OPTIONS(self) -> None:
        self.send_response(204)
        self.end_headers()


def main() -> None:
    server = ThreadingHTTPServer((HOST, PORT), Handler)
    print(f"Симулятор RailWay запущено: http://{HOST}:{PORT}/")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()

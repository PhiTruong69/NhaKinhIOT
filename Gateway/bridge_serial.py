"""Kết nối USB Serial tới Bridge: đọc từng dòng JSON (Bridge chỉ gửi lên, không nhận lệnh).

Định dạng bản tin xem Firmware/Bridge/README.md.
"""
import json
import logging
import threading
import time

import serial

log = logging.getLogger("bridge")


class BridgeSerial:
    def __init__(self, port, baud, on_message):
        """on_message(dict) được gọi cho mỗi dòng JSON hợp lệ nhận từ Bridge (chạy trong thread đọc)."""
        self.port = port
        self.baud = baud
        self.on_message = on_message

    def start(self):
        """Chạy thread đọc Serial nền. Thread tự mở lại cổng nếu Bridge bị rút ra cắm lại."""
        threading.Thread(target=self._run, name="bridge-serial", daemon=True).start()

    def _open(self):
        """Mở cổng mà không kéo DTR/RTS để không làm reset ESP32."""
        ser = serial.Serial()
        ser.port = self.port
        ser.baudrate = self.baud
        ser.timeout = 1
        ser.dtr = False
        ser.rts = False
        ser.open()
        return ser

    def _run(self):
        """Vòng lặp: mở cổng, đọc từng dòng, đổi sang dict và gọi on_message."""
        while True:
            ser = None
            try:
                ser = self._open()
                log.info("opened %s", self.port)

                while True:
                    raw = ser.readline()
                    if not raw:
                        continue
                    text = raw.decode(errors="replace").strip()
                    if not text.startswith("{"):
                        log.debug("skip: %s", text)  # log khởi động của ROM, không phải JSON
                        continue
                    try:
                        msg = json.loads(text)
                    except json.JSONDecodeError:
                        log.warning("bad json: %s", text)
                        continue
                    log.debug("<- %s", text)
                    self.on_message(msg)

            except serial.SerialException as e:
                log.warning("%s: %s, retry in 3 s", self.port, e)
                if ser is not None:
                    ser.close()
                time.sleep(3)

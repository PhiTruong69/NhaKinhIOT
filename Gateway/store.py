"""SQLite cục bộ của gateway (T15).

- outbox  : telemetry chờ gửi lên CoreIOT. Mọi telemetry đều ghi vào đây trước, thread uplink gửi
            và chỉ xoá khi CoreIOT đã nhận (PUBACK). Mất mạng thì dữ liệu nằm lại, có mạng gửi bù
            kèm ts gốc nên biểu đồ không bị đứt đoạn.
- settings: ngưỡng đồng bộ từ CoreIOT (soil_min, temp_max), giữ lại qua các lần khởi động.
"""
import json
import sqlite3
import threading


class Store:
    def __init__(self, path):
        """Mở (hoặc tạo) file SQLite và các bảng."""
        self._db = sqlite3.connect(path, check_same_thread=False)
        self._lock = threading.Lock()
        with self._lock, self._db:
            self._db.execute("CREATE TABLE IF NOT EXISTS outbox ("
                             "id INTEGER PRIMARY KEY AUTOINCREMENT, device TEXT NOT NULL, "
                             "ts INTEGER NOT NULL, data TEXT NOT NULL)")
            self._db.execute("CREATE TABLE IF NOT EXISTS settings ("
                             "key TEXT PRIMARY KEY, value TEXT NOT NULL)")

    def add_telemetry(self, device, ts_ms, values):
        """Thêm một bản ghi telemetry vào outbox."""
        with self._lock, self._db:
            self._db.execute("INSERT INTO outbox (device, ts, data) VALUES (?, ?, ?)",
                             (device, ts_ms, json.dumps(values)))

    def oldest_telemetry(self, limit):
        """Lấy tối đa `limit` bản ghi cũ nhất: list (id, device, ts, values)."""
        with self._lock:
            rows = self._db.execute("SELECT id, device, ts, data FROM outbox ORDER BY id LIMIT ?",
                                    (limit,)).fetchall()
        return [(i, d, ts, json.loads(data)) for i, d, ts, data in rows]

    def delete_telemetry(self, ids):
        """Xoá các bản ghi đã gửi thành công."""
        with self._lock, self._db:
            self._db.executemany("DELETE FROM outbox WHERE id = ?", [(i,) for i in ids])

    def pending_count(self):
        """Số bản ghi đang chờ gửi."""
        with self._lock:
            return self._db.execute("SELECT COUNT(*) FROM outbox").fetchone()[0]

    def save_setting(self, key, value):
        """Lưu một giá trị cấu hình (ghi đè)."""
        with self._lock, self._db:
            self._db.execute("INSERT OR REPLACE INTO settings (key, value) VALUES (?, ?)",
                             (key, json.dumps(value)))

    def load_settings(self):
        """Đọc toàn bộ settings thành dict."""
        with self._lock:
            rows = self._db.execute("SELECT key, value FROM settings").fetchall()
        return {k: json.loads(v) for k, v in rows}

# Bridge – ESP-NOW → USB Serial

Firmware cho NodeMCU ESP32-S3 (N16R8) cắm USB vào PC chạy Gateway Python (mã T06):

- Nhận telemetry ESP-NOW từ các node, **trả ACK** (node gửi lại nếu không có ACK), lọc gói trùng.
- In mỗi gói thành **một dòng JSON** qua USB Serial cho Gateway.
- Không kết nối WiFi.

Bridge chỉ chuyển dữ liệu **lên**. Lệnh điều khiển quạt và bơm không đi qua đây, vì Node 2 nối thẳng CoreIOT qua WiFi.

## Cấu trúc thư mục

```
Bridge/
├─ src/main.cpp              khởi tạo, tạo queue và 2 task FreeRTOS
├─ include/bridge_config.h   baud, số node tối đa, FreeRTOS
├─ lib/
│  ├─ BridgeLink/            ESP-NOW: nhận telemetry, trả ACK, lọc gói trùng, học MAC node
│  └─ HostLink/              Serial JSON: in boot, in telemetry
└─ ../common/protocol.h      định dạng gói ESP-NOW, dùng chung với Node
```

Thư viện ngoài duy nhất là `ArduinoJson` 7. PlatformIO tự tải thư viện này qua `lib_deps`.

## Task FreeRTOS

Mỗi phần cứng chỉ do **một** task dùng. Cả hai task chạy trên core 1. `loop()` không dùng tới và tự xoá.

```
           ESP-NOW                 telQueue (TelemetryPkt)
Node 1 ───────────────▶ linkTask ──────────────────────────▶ hostTask ──USB Serial──▶ Gateway
       ◀──── ACK ─────  (ESP-NOW)                            (Serial)
```

| Task | Ưu tiên | Phần cứng | Việc |
|---|---|---|---|
| `linkTask` | 3 | ESP-NOW | Nhận telemetry, trả ACK ngay, lọc gói trùng, đẩy vào `telQueue` |
| `hostTask` | 2 | USB Serial | In telemetry thành JSON |

In JSON chậm hơn trả ACK nhiều, nên việc in được tách sang `hostTask`; nhờ vậy ACK không bị trễ. `hostTask` cần stack 6 KB vì dùng ArduinoJson.

## Build và nạp

```bash
pio run                 # build
pio run -t upload       # nạp
pio device monitor      # xem JSON, 115200
```

Board có 2 cổng USB-C. Với `-DARDUINO_USB_CDC_ON_BOOT=1`, JSON đi ra cổng **USB** (native), và Gateway phải mở cổng COM này. Muốn dùng cổng **UART** (COM qua chip USB-UART) thì xoá dòng đó trong `platformio.ini`.

## Giao thức Serial (cho Gateway – TV4)

Mỗi bản tin là một dòng JSON kết thúc bằng `\n`, tốc độ 115200.

| `type` | Ví dụ | Khi nào |
|---|---|---|
| `boot` | `{"type":"boot","mac":"34:85:18:..","channel":1}` | Bridge vừa khởi động |
| `telemetry` | `{"type":"telemetry","node":1,"seq":12,"temperature":27.3,"humidity":65.1,"soil_raw":2100,"soil":47}` | Có dữ liệu mới từ node (đã lọc gói trùng) |

- `temperature` và `humidity` **không có** trong bản tin khi node đọc SHT20 lỗi.
- `seq` tăng dần theo từng gói. Nếu `seq` bị nhảy cóc thì đó là gói đã mất hẳn: node gửi lại 3 lần mà vẫn không có ACK. Dùng số này để đo tỉ lệ mất gói theo khoảng cách.

## Thử nhanh không cần Gateway

1. Nạp Node 1 và Bridge, cắm Bridge vào PC.
2. Mở `pio device monitor`, nhấn RST trên Bridge: dòng `boot` hiện ra.
3. Sau khoảng 10 s sẽ thấy dòng `telemetry` của node 1, rồi cứ 10 s có thêm một dòng.

## Cấu hình (`include/bridge_config.h`)

| Hằng số | Mặc định | Ý nghĩa |
|---|---|---|
| `HOST_BAUD` | 115200 | Tốc độ Serial tới PC |
| `MAX_NODES` | 4 | nodeId hợp lệ từ 1 đến 4 |

Kênh ESP-NOW và phiên bản giao thức (`PROTO_VERSION = 2`) nằm trong `../common/protocol.h`. Node và Bridge phải nạp cùng một bản `protocol.h`; nếu khác phiên bản, Bridge bỏ qua gói của node.

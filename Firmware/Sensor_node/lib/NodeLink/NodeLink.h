// NodeLink – Liên lạc ESP-NOW với Bridge theo protocol.h (không kết nối WiFi).
//  - Gửi telemetry, chờ ACK, gửi lại tối đa MAX_RETRIES lần.
//  - Ban đầu gửi broadcast, tự học MAC của Bridge từ gói đầu tiên nhận được.
// Các hàm link_* (trừ link_lostCount) chỉ được gọi từ một task duy nhất (linkTask).
#pragma once
#include <Arduino.h>
#include "protocol.h"

// Bật radio ở chế độ STA, đặt kênh ESPNOW_CHANNEL, khởi tạo ESP-NOW.
void link_begin();

// Gửi telemetry. Hàm tự điền header (seq) và CRC, chỉ cần điền phần dữ liệu.
void link_sendTelemetry(TelemetryPkt &pkt);

// Xử lý các ACK đã nhận từ Bridge, chờ tối đa `wait` tick nếu queue đang trống.
void link_poll(TickType_t wait = 0);

// Gọi lặp trong task: gửi lại telemetry nếu quá ACK_TIMEOUT_MS chưa có ACK.
void link_update();

// Số gói telemetry bị mất (hết số lần gửi lại mà không có ACK).
uint32_t link_lostCount();

// BridgeLink – Phía ESP-NOW của Bridge theo protocol.h (không kết nối WiFi).
//  - Nhận telemetry từ node, tự trả ACK, bỏ gói trùng (node gửi lại khi mất ACK).
//  - Tự học MAC của từng node từ gói telemetry đầu tiên (để gửi ACK unicast).
// Các hàm link_* chỉ được gọi từ một task duy nhất (linkTask).
#pragma once
#include <Arduino.h>
#include "protocol.h"

// Bật radio ở chế độ STA, đặt kênh ESPNOW_CHANNEL, khởi tạo ESP-NOW.
void link_begin();

// Xử lý gói nhận được, chờ tối đa `wait` tick nếu chưa có gói nào.
// Trả về true và điền tel khi có telemetry mới (đã ACK, không trùng). Node gửi nằm ở tel.h.nodeId.
bool link_receiveTelemetry(TelemetryPkt &tel, TickType_t wait = 0);

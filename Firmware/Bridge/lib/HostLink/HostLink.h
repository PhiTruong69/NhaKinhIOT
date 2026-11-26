// HostLink – Gửi dữ liệu lên Gateway PC qua USB Serial, mỗi bản tin là một dòng JSON.
// Các hàm host_* chỉ được gọi từ một task duy nhất (hostTask) để các dòng JSON không bị lẫn.
//
// Bridge -> PC:
//   {"type":"boot","mac":"AA:BB:..","channel":1}
//   {"type":"telemetry","node":1,"seq":12,"temperature":27.3,"humidity":65.1,
//    "soil_raw":2100,"soil":47}          (temperature/humidity vắng mặt nếu Modbus lỗi)
#pragma once
#include <Arduino.h>
#include "protocol.h"

// Mở Serial tới Gateway.
void host_begin();

// Báo Bridge vừa khởi động (kèm MAC và kênh ESP-NOW).
void host_printBoot();

// In telemetry của node thành một dòng JSON.
void host_printTelemetry(const TelemetryPkt &t);

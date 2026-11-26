// bridge_config.h – Cấu hình Bridge (ESP-NOW -> USB Serial).
#pragma once
#include <Arduino.h>

// ---------- Serial tới Gateway PC ----------
#define HOST_BAUD 115200

// ---------- ESP-NOW ----------
#define MAX_NODES 4  // nodeId hợp lệ: 1..MAX_NODES

// ---------- FreeRTOS ----------
#define APP_CORE      1    // core 1; core 0 dành cho WiFi/ESP-NOW
#define STACK_LINK    4096
#define STACK_HOST    6144 // ArduinoJson cần nhiều stack hơn
#define PRIO_LINK     3    // ACK phải nhanh
#define PRIO_HOST     2
#define TEL_QUEUE_LEN 16

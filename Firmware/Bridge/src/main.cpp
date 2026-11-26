// Bridge (NodeMCU ESP32-S3 N16R8, cắm USB vào PC Gateway)
// Nhận telemetry ESP-NOW từ node (trả ACK), chuyển thành JSON qua USB Serial cho Gateway.
//
// FreeRTOS – 2 task, mỗi phần cứng chỉ do một task dùng:
//   linkTask : sở hữu ESP-NOW; nhận telemetry, trả ACK ngay -> telQueue
//   hostTask : sở hữu Serial; in telemetry thành JSON (tách riêng để in chậm không làm trễ ACK)
#include <Arduino.h>
#include "bridge_config.h"
#include "BridgeLink.h"
#include "HostLink.h"

static QueueHandle_t telQueue;  // linkTask -> hostTask (TelemetryPkt)

// Nhận telemetry, ACK và chuyển sang hostTask.
static void linkTask(void *) {
  for (;;) {
    TelemetryPkt t;
    if (link_receiveTelemetry(t, portMAX_DELAY)) xQueueSend(telQueue, &t, 0);
  }
}

// In telemetry ra Serial (task duy nhất ghi Serial).
static void hostTask(void *) {
  for (;;) {
    TelemetryPkt t;
    if (xQueueReceive(telQueue, &t, portMAX_DELAY) == pdTRUE) host_printTelemetry(t);
  }
}

void setup() {
  host_begin();
  link_begin();
  host_printBoot();

  telQueue = xQueueCreate(TEL_QUEUE_LEN, sizeof(TelemetryPkt));

  xTaskCreatePinnedToCore(linkTask, "link", STACK_LINK, nullptr, PRIO_LINK, nullptr, APP_CORE);
  xTaskCreatePinnedToCore(hostTask, "host", STACK_HOST, nullptr, PRIO_HOST, nullptr, APP_CORE);
}

// Không dùng loop(): xoá task loop của Arduino, mọi việc chạy trong các task trên.
void loop() {
  vTaskDelete(nullptr);
}

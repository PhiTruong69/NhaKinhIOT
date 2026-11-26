// Node 1 – Vườn (ESP32-S3 DevKitC-1) – chỉ đo, không điều khiển
// Đọc SHT20 RS485 + độ ẩm đất, gửi lên Bridge qua ESP-NOW mỗi 10 s (Bridge trả ACK).
//
// FreeRTOS – 2 task, mỗi phần cứng chỉ do một task dùng:
//   sensorTask : đọc cảm biến mỗi TELEMETRY_PERIOD_MS -> telemetryQueue
//   linkTask   : sở hữu ESP-NOW; gửi telemetry, nhận ACK, gửi lại khi chưa có ACK
#include <Arduino.h>
#include "node_config.h"
#include "SHT20.h"
#include "SoilSensor.h"
#include "NodeLink.h"

static QueueHandle_t telemetryQueue;  // sensorTask -> linkTask (TelemetryPkt)

// Đọc cảm biến theo chu kỳ và đẩy số liệu mới nhất cho linkTask.
static void sensorTask(void *) {
  for (;;) {
    TelemetryPkt p = {};
    int16_t  t10 = 0;
    uint16_t h10 = 0;
    if (sht20_read(t10, h10)) {
      p.airTemp_x10 = t10;
      p.airHum_x10 = h10;
      p.status |= STATUS_MODBUS_OK;
    }
    p.soilRaw = soil_readRaw();
    p.soilPct = soil_toPercent(p.soilRaw);
    xQueueOverwrite(telemetryQueue, &p);  // chỉ giữ số liệu mới nhất

    Serial.printf("[SENSOR] T=%.1f H=%.1f soilRaw=%u soil=%u%% modbus=%s lost=%lu\n",
                  t10 / 10.0, h10 / 10.0, p.soilRaw, p.soilPct,
                  (p.status & STATUS_MODBUS_OK) ? "ok" : "FAIL", (unsigned long)link_lostCount());

    vTaskDelay(pdMS_TO_TICKS(TELEMETRY_PERIOD_MS));
  }
}

// Giao tiếp ESP-NOW: gửi telemetry, nhận ACK, gửi lại khi chưa có ACK.
static void linkTask(void *) {
  for (;;) {
    TelemetryPkt p;
    if (xQueueReceive(telemetryQueue, &p, 0) == pdTRUE) link_sendTelemetry(p);
    link_poll(pdMS_TO_TICKS(LINK_POLL_MS));
    link_update();
  }
}

void setup() {
  Serial.begin(115200);
  soil_begin();
  sht20_begin();
  link_begin();

  telemetryQueue = xQueueCreate(1, sizeof(TelemetryPkt));

  xTaskCreatePinnedToCore(sensorTask, "sensor", STACK_SENSOR, nullptr, PRIO_SENSOR, nullptr, APP_CORE);
  xTaskCreatePinnedToCore(linkTask, "link", STACK_LINK, nullptr, PRIO_LINK, nullptr, APP_CORE);
}

// Không dùng loop(): xoá task loop của Arduino, mọi việc chạy trong các task trên.
void loop() {
  vTaskDelete(nullptr);
}

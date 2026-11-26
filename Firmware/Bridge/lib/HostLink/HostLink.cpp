#include "HostLink.h"
#include "bridge_config.h"
#include <ArduinoJson.h>
#include <WiFi.h>

// Gửi một document JSON thành một dòng.
static void printLine(JsonDocument &doc) {
  serializeJson(doc, Serial);
  Serial.println();
}

void host_begin() {
  Serial.begin(HOST_BAUD);
}

void host_printBoot() {
  JsonDocument doc;
  doc["type"] = "boot";
  doc["mac"] = WiFi.macAddress();
  doc["channel"] = ESPNOW_CHANNEL;
  printLine(doc);
}

void host_printTelemetry(const TelemetryPkt &t) {
  JsonDocument doc;
  doc["type"] = "telemetry";
  doc["node"] = t.h.nodeId;
  doc["seq"] = t.h.seq;
  if (t.status & STATUS_MODBUS_OK) {
    doc["temperature"] = t.airTemp_x10 / 10.0;
    doc["humidity"] = t.airHum_x10 / 10.0;
  }
  doc["soil_raw"] = t.soilRaw;
  doc["soil"] = t.soilPct;
  printLine(doc);
}

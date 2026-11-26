#include "BridgeLink.h"
#include "bridge_config.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// Một gói nhận được, chuyển từ callback ESP-NOW sang loop() qua queue
struct RxItem {
  uint8_t mac[6];
  uint8_t len;
  uint8_t data[32];
};

// Thông tin của từng node, đánh chỉ số theo nodeId
struct NodeInfo {
  bool    known;
  uint8_t mac[6];
  int32_t lastTelSeq;  // seq telemetry gần nhất, dùng để bỏ gói trùng
};

static QueueHandle_t rxQueue;
static NodeInfo      nodes[MAX_NODES + 1];
static uint16_t      txSeq = 0;

// Thêm một peer ESP-NOW (nếu chưa có).
static void addPeer(const uint8_t *mac) {
  if (esp_now_is_peer_exist(mac)) return;
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = ESPNOW_CHANNEL;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
}

// Ghi nhớ (hoặc cập nhật) MAC của node.
static void learnNode(uint8_t nodeId, const uint8_t *mac) {
  NodeInfo &n = nodes[nodeId];
  if (n.known && memcmp(n.mac, mac, 6) == 0) return;
  memcpy(n.mac, mac, 6);
  n.known = true;
  addPeer(mac);
}

// Callback chạy trong task WiFi: chỉ copy gói vào queue.
#if ESP_ARDUINO_VERSION_MAJOR >= 3
static void onRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  const uint8_t *mac = info->src_addr;
#else
static void onRecv(const uint8_t *mac, const uint8_t *data, int len) {
#endif
  if (len <= 0 || len > (int)sizeof(RxItem::data)) return;
  RxItem it;
  memcpy(it.mac, mac, 6);
  it.len = len;
  memcpy(it.data, data, len);
  xQueueSend(rxQueue, &it, 0);
}

// Gửi ACK cho telemetry của node.
static void sendAck(uint8_t nodeId, uint16_t ackSeq) {
  AckPkt a = {};
  a.h = {PROTO_VERSION, MSG_ACK, nodeId, ++txSeq};
  a.ackSeq = ackSeq;
  a.result = ACK_OK;
  proto_seal(a);
  esp_now_send(nodes[nodeId].mac, (const uint8_t *)&a, sizeof(a));
}

// Xử lý một gói nhận được. Trả về true nếu là telemetry mới.
static bool handlePacket(const RxItem &it, TelemetryPkt &tel) {
  if (it.len != sizeof(TelemetryPkt)) return false;
  memcpy(&tel, it.data, sizeof(tel));
  const PktHeader &h = tel.h;
  if (h.version != PROTO_VERSION || h.type != MSG_TELEMETRY || h.nodeId < 1 || h.nodeId > MAX_NODES) return false;
  if (!proto_check(tel)) return false;

  // Luôn ACK (kể cả gói trùng, vì node gửi lại khi ACK trước bị mất), chỉ báo lên nếu không trùng
  learnNode(h.nodeId, it.mac);
  sendAck(h.nodeId, h.seq);
  NodeInfo &n = nodes[h.nodeId];
  if ((int32_t)h.seq == n.lastTelSeq) return false;
  n.lastTelSeq = h.seq;
  return true;
}

void link_begin() {
  rxQueue = xQueueCreate(16, sizeof(RxItem));
  for (auto &n : nodes) n = {false, {0}, -1};

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) {
    delay(1000);
    ESP.restart();
  }
  esp_now_register_recv_cb(onRecv);
}

bool link_receiveTelemetry(TelemetryPkt &tel, TickType_t wait) {
  RxItem it;
  while (xQueueReceive(rxQueue, &it, wait) == pdTRUE) {
    wait = 0;  // chỉ chờ gói đầu tiên, các gói sau lấy ngay nếu có
    if (handlePacket(it, tel)) return true;
  }
  return false;
}

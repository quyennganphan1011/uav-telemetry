#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

typedef struct __attribute__((packed)) {
  uint32_t seq;
  uint32_t t_send_us;
  uint16_t payload_len;
  uint8_t  payload[64];
} test_pkt_t;

static uint8_t peer_mac[6] = {0xF0,0x24,0xF9,0x0D,0xD3,0x20}; // <-- MAC receiver

static volatile uint32_t sent_ok = 0, sent_fail = 0;
static uint32_t seq_id = 0;

void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) sent_ok++;
  else sent_fail++;
}

void setup() {
  Serial.begin(115200);
  delay(300);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);

  // set Wi-Fi channel (sender & receiver must match)
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (true) delay(1000);
  }
  esp_now_register_send_cb(onSent);

  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, peer_mac, 6);
  peer.channel = 6;
  peer.encrypt = false;

  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("Add peer failed");
    while (true) delay(1000);
  }

  Serial.println("Sender ready");
}

void loop() {
  static uint32_t last_report_ms = 0;

  test_pkt_t pkt{};
  pkt.seq = seq_id++;
  pkt.t_send_us = micros();
  pkt.payload_len = sizeof(pkt.payload);
  for (int i = 0; i < (int)sizeof(pkt.payload); i++) pkt.payload[i] = (uint8_t)i;

  esp_err_t r = esp_now_send(peer_mac, (uint8_t*)&pkt, sizeof(pkt));
  (void)r;

  delay(20); // 50 packets/sec

  uint32_t now = millis();
  if (now - last_report_ms >= 1000) {
    last_report_ms = now;
    uint32_t total = sent_ok + sent_fail;
    Serial.printf("pps=%lu ok=%lu fail=%lu loss=%.2f%%\n",
                  total, sent_ok, sent_fail,
                  total ? (100.0f * sent_fail / total) : 0.0f);
    sent_ok = sent_fail = 0;
  }
}

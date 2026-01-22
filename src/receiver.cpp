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

void onRecv(const uint8_t *mac, const uint8_t *data, int len) {
  test_pkt_t pkt;
  memcpy(&pkt, data, sizeof(pkt));

  uint32_t latency = micros() - pkt.t_send_us;

  Serial.printf(
    "RX seq=%lu len=%u latency=%lu us\n",
    pkt.seq, pkt.payload_len, latency
  );
}

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  esp_now_init();
  esp_now_register_recv_cb(onRecv);

  Serial.println("Receiver ready");
}

void loop() {
  delay(1000);
}

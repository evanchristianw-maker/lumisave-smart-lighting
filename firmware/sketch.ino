#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include "secrets.h"

// ---------- Pin ----------
#define PIN_LDR    34
#define PIN_PIR    27
#define PIN_RELAY  26   // LED simulasi via resistor 220 Ohm ke GND
#define PIN_BUZZER 25   // Buzzer 1 (+) ke D25, (-) ke GND

#define RELAY_ON   HIGH
#define RELAY_OFF  LOW

// ---------- Parameter ----------
const float    LUX_DARK      = 50.0;    // Ambang batas gelap
const uint32_t HOLD_MS       = 10000;    // Lampu OFF setelah 5 dtk tanpa gerak
const uint32_t PUBLISH_MS    = 5000;    // Interval kirim MQTT
const uint32_t ANOMALY_MS    = 300000;  // Lampu ON kontinu > 5 menit = anomali
const uint32_t PIR_WARMUP_MS = 15000;   // PIR diabaikan 15 dtk pertama (pemanasan)
const float    LAMP_WATT     = 9.0;     // Daya lampu (Watt)
const float    TARIF_RP_KWH  = 1444.70; // Tarif listrik per kWh

// ---------- Kalibrasi LDR ----------
const int   CAL_N = 5;
const int   CAL_ADC[CAL_N] = {100, 800, 1800, 2800, 3800};
const float CAL_LUX[CAL_N] = {900, 300, 80, 20, 2};

WiFiClient wifi;
PubSubClient mqtt(wifi);

bool lampOn = false, anomaly = false, pirLatched = false;
uint32_t lastMotion = 0, lampOnSince = 0, lastTick = 0, lastPub = 0, lastReconnect = 0, seq = 0;
double energyWh = 0, baselineWh = 0;

String tData = "v1/devices/me/telemetry";

int readAdc() {
  long s = 0;
  for (int i = 0; i < 16; i++) { s += analogRead(PIN_LDR); delay(2); }
  return s / 16;
}

float adcToLux(int adc) {
  if (adc <= CAL_ADC[0]) return CAL_LUX[0];
  for (int i = 1; i < CAL_N; i++) {
    if (adc <= CAL_ADC[i]) {
      float f = (adc - CAL_ADC[i - 1]) / (float)(CAL_ADC[i] - CAL_ADC[i - 1]);
      return CAL_LUX[i - 1] + f * (CAL_LUX[i] - CAL_LUX[i - 1]);
    }
  }
  return CAL_LUX[CAL_N - 1];
}

void connectWifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 15000) delay(250);
  configTime(0, 0, "pool.ntp.org");
}

void reconnectMqtt() {
  if (mqtt.connected() || millis() - lastReconnect < 5000) return;
  lastReconnect = millis();
  if (WiFi.status() != WL_CONNECTED) { connectWifi(); return; }
  if (mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, nullptr)) {
    Serial.println("MQTT terhubung");
  } else {
    Serial.printf("MQTT gagal, state=%d\n", mqtt.state());
  }
}

void publishData(int adc, float lux, bool pir, bool dark) {
  JsonDocument d;
  d["seq"] = ++seq;
  d["ts"] = (uint32_t)time(nullptr);
  d["adc"] = adc;
  d["lux"] = round(lux * 10) / 10.0;
  d["pir"] = pir;
  d["motion_status"] = pir ? 1 : 0;
  d["dark"] = dark;
  d["relay"] = lampOn;
  d["energy_wh"] = round(energyWh * 1000) / 1000.0;
  d["baseline_wh"] = round(baselineWh * 1000) / 1000.0;
  d["saving_pct"] = baselineWh > 0 ? round((1.0 - energyWh / baselineWh) * 1000) / 10.0 : 0;
  d["anomaly"] = anomaly;
  d["rssi"] = WiFi.RSSI();
  d["cost_rp"] = round((energyWh / 1000.0) * TARIF_RP_KWH * 10000) / 10000.0;
  d["baseline_cost_rp"] = round((baselineWh / 1000.0) * TARIF_RP_KWH * 10000) / 10000.0;

  char buf[450];
  size_t n = serializeJson(d, buf);
  mqtt.publish(tData.c_str(), (const uint8_t*)buf, n, false);
  Serial.println(buf);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_RELAY, RELAY_OFF);
  digitalWrite(PIN_BUZZER, LOW);

  connectWifi();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setBufferSize(512);
  lastTick = millis();
}

void loop() {
  reconnectMqtt();
  mqtt.loop();

  uint32_t now = millis();
  int adc = readAdc();
  float lux = adcToLux(adc);
  bool dark = lux < LUX_DARK;
  bool pir = digitalRead(PIN_PIR) && (now > PIR_WARMUP_MS);

  if (pir) pirLatched = true;

  // Logika: Gelap DAN Gerak -> ON; OFF jika Terang atau tanpa gerak > 5 detik
  if (dark && pir) {
    lastMotion = now;
    if (!lampOn) { lampOn = true; lampOnSince = now; }
  }
  if (lampOn && (!dark || now - lastMotion > HOLD_MS)) {
    lampOn = false;
    anomaly = false;
  }

  anomaly = lampOn && (now - lampOnSince > ANOMALY_MS);
  digitalWrite(PIN_RELAY, lampOn ? RELAY_ON : RELAY_OFF);

  // Buzzer bunyi singkat sekali (200 ms) saat lampu baru menyala
  if (lampOn && (now - lampOnSince < 200)) {
    digitalWrite(PIN_BUZZER, HIGH);
  } else {
    digitalWrite(PIN_BUZZER, LOW);
  }

  // Akumulasi energi (Wh)
  double dtH = (now - lastTick) / 3600000.0;
  lastTick = now;
  if (lampOn) energyWh += LAMP_WATT * dtH;
  if (dark)   baselineWh += LAMP_WATT * dtH;

  if (now - lastPub >= PUBLISH_MS) {
    lastPub = now;
    if (mqtt.connected()) publishData(adc, lux, pirLatched, dark);
    pirLatched = false;
  }
  delay(50);
}

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>
#include "secrets.h"

// ---------- Pin ----------
#define PIN_LDR    34
#define PIN_PIR    27
#define PIN_RELAY  26
#define PIN_BUZZER 25

// ---------- Parameter (justifikasi di laporan) ----------
const float    LUX_DARK       = 50.0;    // lampu boleh ON jika lux < nilai ini (ganti dengan hasil ukur lapangan)
const uint32_t HOLD_MS        = 30000;   // lampu OFF setelah 30 dtk tanpa gerak
const uint32_t PUBLISH_MS     = 5000;    // interval kirim MQTT
const uint32_t ANOMALY_MS     = 300000;  // lampu ON kontinu > 5 menit = anomali
const float    LAMP_WATT      = 9.0;     // daya lampu dari label

// Konstanta LDR Wokwi (datasheet LDR module)
const float GAMMA = 0.7;
const float RL10  = 50;

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
WiFiClient wifi;
PubSubClient mqtt(wifi);

bool lampOn = false, anomaly = false;
uint32_t lastMotion = 0, lampOnSince = 0, lastTick = 0, lastPub = 0, lastReconnect = 0, seq = 0;
double energyWh = 0, baselineWh = 0;  // energi dengan kontrol vs lampu tanpa kontrol (ON sepanjang gelap)

String tData, tStatus;

float readLux() {
  int adc = analogRead(PIN_LDR);
  float v = adc / 4095.0 * 5.0;
  if (v >= 4.99) v = 4.99;
  float r = 2000.0 * v / (1.0 - v / 5.0);
  if (r <= 0) return 100000;
  return pow(RL10 * 1e3 * pow(10, GAMMA) / r, 1.0 / GAMMA);
}

void connectWifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS, 6);
  uint32_t t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 15000) delay(250);
  configTime(0, 0, "pool.ntp.org");
}

void reconnectMqtt() {
  if (mqtt.connected() || millis() - lastReconnect < 5000) return;
  lastReconnect = millis();
  if (WiFi.status() != WL_CONNECTED) { connectWifi(); return; }
  const char* user = strlen(MQTT_USER) ? MQTT_USER : nullptr;
  const char* pass = strlen(MQTT_PASS) ? MQTT_PASS : nullptr;
  if (mqtt.connect(MQTT_CLIENT_ID, user, pass, tStatus.c_str(), 0, true, "offline")) {
    mqtt.publish(tStatus.c_str(), "online", true);
  }
}

void beep(int ms) { digitalWrite(PIN_BUZZER, HIGH); delay(ms); digitalWrite(PIN_BUZZER, LOW); }

void drawOled(float lux, bool pir) {
  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.printf("LumiSave\nLux : %.0f\nPIR : %s\nLamp: %s\nE   : %.3f Wh\nBase: %.3f Wh\n%s",
              lux, pir ? "GERAK" : "-", lampOn ? "ON" : "OFF", energyWh, baselineWh,
              anomaly ? "!! ANOMALI" : (mqtt.connected() ? "MQTT OK" : "MQTT --"));
  oled.display();
}

void publishData(float lux, bool pir, bool dark) {
  JsonDocument d;
  d["seq"] = ++seq;
  d["ts"] = (uint32_t)time(nullptr);
  d["lux"] = round(lux * 10) / 10.0;
  d["pir"] = pir;
  d["dark"] = dark;
  d["relay"] = lampOn;
  d["energy_wh"] = round(energyWh * 1000) / 1000.0;
  d["baseline_wh"] = round(baselineWh * 1000) / 1000.0;
  d["saving_pct"] = baselineWh > 0 ? round((1.0 - energyWh / baselineWh) * 1000) / 10.0 : 0;
  d["anomaly"] = anomaly;
  d["rssi"] = WiFi.RSSI();
  char buf[320];
  size_t n = serializeJson(d, buf);
  mqtt.publish(tData.c_str(), (const uint8_t*)buf, n, false);
  Serial.println(buf);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  tData = String(MQTT_TOPIC_PREFIX) + "/data";
  tStatus = String(MQTT_TOPIC_PREFIX) + "/status";
  connectWifi();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  lastTick = millis();
}

void loop() {
  reconnectMqtt();
  mqtt.loop();

  uint32_t now = millis();
  float lux = readLux();
  bool dark = lux < LUX_DARK;
  bool pir = digitalRead(PIN_PIR);

  // Logika: gelap DAN gerak -> ON; OFF jika terang atau tanpa gerak > HOLD_MS
  if (dark && pir) {
    lastMotion = now;
    if (!lampOn) { lampOn = true; lampOnSince = now; beep(80); }
  }
  if (lampOn && (!dark || now - lastMotion > HOLD_MS)) { lampOn = false; anomaly = false; }
  anomaly = lampOn && (now - lampOnSince > ANOMALY_MS);
  digitalWrite(PIN_RELAY, lampOn ? HIGH : LOW);
  if (anomaly && (now / 1000) % 2 == 0) digitalWrite(PIN_BUZZER, HIGH); else digitalWrite(PIN_BUZZER, LOW);

  // Akumulasi energi (Wh)
  double dtH = (now - lastTick) / 3600000.0;
  lastTick = now;
  if (lampOn) energyWh += LAMP_WATT * dtH;
  if (dark)   baselineWh += LAMP_WATT * dtH;

  if (now - lastPub >= PUBLISH_MS) {
    lastPub = now;
    if (mqtt.connected()) publishData(lux, pir, dark);
    drawOled(lux, pir);
  }
  delay(50);
}

// Salin file ini menjadi secrets.h (secrets.h TIDAK di-commit)
#pragma once
#define WIFI_SSID  "Wokwi-GUEST"
#define WIFI_PASS  ""
#define MQTT_HOST  "broker.hivemq.com"
#define MQTT_PORT  1883
#define MQTT_USER  ""   // isi jika memakai Mosquitto dengan autentikasi
#define MQTT_PASS  ""
#define MQTT_CLIENT_ID     "lumisave-esp32-uns25evan"
#define MQTT_TOPIC_PREFIX  "lumisave/uns25evan"   // ganti unik agar tidak bentrok di broker publik

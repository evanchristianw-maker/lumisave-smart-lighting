# LumiSave Smart Lighting

Sistem kendali lampu otomatis hemat energi berbasis IoT dengan sensor cahaya (LDR) dan gerak (PIR).  
Proyek mata kuliah **Internet of Things** — Tema C: Manajemen Energi dan Lingkungan Perkotaan untuk Fasilitas Kota dan UMKM.

## Cara Kerja
- **Otomatisasi Lampu:** Lampu ON jika kondisi ruangan gelap **dan** terdeteksi gerakan.
- **Pemadaman Otomatis:** Lampu OFF setelah **10 detik** tanpa gerakan, atau langsung mati saat kondisi ruangan kembali terang.
- **Indikator Audio & Anomali:** Buzzer berbunyi singkat (`200 ms`) saat lampu mulai menyala, dan berbunyi sebagai peringatan apabila lampu menyala terus-menerus lebih dari **5 menit** (ditandai dengan flag `anomaly: true`).
- **Kalkulasi Energi:** Energi dihitung dari asumsi daya lampu (`9 W`) × durasi ON (`energy_wh`), lalu dibandingkan dengan lampu tanpa kontrol saat kondisi gelap (`baseline_wh`) untuk mendapatkan persentase penghematan (`saving_pct`) dan estimasi biaya listrik (`cost_rp`, `baseline_cost_rp`).

## Alur Sistem
`LDR + PIR` -> `ESP32` -> `Wi-Fi` -> `MQTT` -> `ThingsBoard Cloud (Penyimpanan Time-Series & Dashboard)`

## Komponen Perangkat Keras
- Mikrokontroler **ESP32 DevKit V1**
- Modul Sensor Cahaya **LDR (LM393)** — `GPIO 34` (ADC rata-rata 16 sampel + tabel kalibrasi lux)
- Sensor Gerak **PIR HC-SR501** — `GPIO 27` (masa pemanasan / *warm-up* 15 detik)
- **LED** (simulasi lampu) + **Resistor 220Ω** — `GPIO 26`
- **Buzzer Aktif** — `GPIO 25`

Prototipe diuji pada simulator **Wokwi** dan diimplementasikan secara langsung pada **perangkat fisik (*breadboard*)**.

## Menjalankan Firmware
1. Buat entitas *Device* baru di **ThingsBoard Cloud** dan salin *Access Token*-nya.
2. Salin `firmware/secrets.example.h` menjadi `firmware/secrets.h`, lalu isi `WIFI_SSID`, `WIFI_PASS`, dan `MQTT_USER` (dengan *Access Token* perangkat).
3. Buka proyek pada simulator **Wokwi** (impor `firmware/diagram.json` dan `firmware/sketch.ino`) atau buka `firmware/sketch.ino` melalui **Arduino IDE**.
4. Pasang library yang tercantum di `firmware/libraries.txt`:
   - `PubSubClient`
   - `ArduinoJson`
5. Unggah program ke ESP32 atau jalankan simulasi, lalu buka **Serial Monitor (`115200 bps`)**. Pesan `MQTT terhubung` menandakan data telemetri mulai dikirimkan.

> **Catatan Keamanan:** Berkas `secrets.h` dan kredensial *Access Token* dikecualikan dari repositori melalui `.gitignore`.

## Konfigurasi MQTT

| Item | Nilai |
| :--- | :--- |
| **Broker** | `mqtt.thingsboard.cloud` (Port `1883`) |
| **Topik Telemetri** | `v1/devices/me/telemetry` |
| **Autentikasi** | *Device Access Token* (disimpan di `secrets.h`, tidak di-*commit*) |
| **Interval Kirim** | 5 detik |

**Contoh Payload JSON:**
```json
{
  "seq": 13,
  "ts": 1794749097,
  "lux": 13.2,
  "pir": true,
  "dark": true,
  "relay": true,
  "energy_wh": 0,
  "baseline_wh": 0.099,
  "saving_pct": 99.9,
  "anomaly": false,
  "rssi": -63,
  "cost_rp": 0,
  "baseline_cost_rp": 0.1430
}

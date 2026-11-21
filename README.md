# LumiSave Smart Lighting

Sistem kendali lampu otomatis hemat energi berbasis IoT dengan sensor cahaya (LDR) dan gerak (PIR).
Project mata kuliah Internet of Things, Tema C: Manajemen Energi dan Lingkungan Perkotaan untuk Fasilitas Kota dan UMKM.

## Cara kerja
- Lampu ON jika kondisi gelap **dan** ada gerak.
- Lampu OFF setelah 5 detik tanpa gerak, atau saat kondisi terang.
- Energi dihitung dari daya lampu x durasi ON, lalu dibandingkan dengan lampu tanpa kontrol (`baseline_wh`) untuk mendapatkan persentase penghematan (`saving_pct`) dan estimasi biaya (`cost_rp`, `baseline_cost_rp`).
- Anomali: lampu ON terus-menerus lebih dari 5 menit, ditandai buzzer dan flag `anomaly`.

## Alur sistem
LDR + PIR -> ESP32 -> WiFi -> MQTT -> ThingsBoard Cloud (penyimpanan riwayat + dashboard)

## Komponen
ESP32, LDR module, PIR HC-SR501, LED (mewakili lampu) + resistor 220Ω, buzzer.
Prototipe dijalankan di simulator Wokwi, serta implementasi menggunakan real device

## Menjalankan firmware
1. Buat device di ThingsBoard Cloud dan salin access token-nya.
2. Salin `firmware/secrets.example.h` menjadi `firmware/secrets.h`, lalu isi `MQTT_USER` dengan access token.
3. Buka proyek di Wokwi (impor `firmware/diagram.json` dan `firmware/sketch.ino`) atau di Arduino IDE.
4. Pasang library di `firmware/libraries.txt`: PubSubClient, ArduinoJson, Adafruit GFX Library, Adafruit SSD1306.
5. Jalankan simulasi dan buka Serial Monitor. Pesan `MQTT terhubung` menandakan data terkirim.

`secrets.h` dan access token tidak di-commit (lihat `.gitignore`).

## MQTT
| Item | Nilai |
|---|---|
| Broker | `mqtt.thingsboard.cloud:1883` |
| Topik telemetri | `v1/devices/me/telemetry` |
| Autentikasi | Access token device (di `secrets.h`, tidak di-commit) |
| Interval | 5 detik |

Contoh payload:
```json
{"seq":13,"ts":1794749097,"lux":13.2,"pir":true,"dark":true,"relay":true,"energy_wh":0,"baseline_wh":0.099,"saving_pct":99.9,"anomaly":false,"rssi":-63,"cost_rp":0,"baseline_cost_rp":0.1430}
```
Kolom:
- `seq`: nomor urut, untuk menghitung packet loss
- `ts`: epoch detik, untuk menghitung latensi
- `lux`, `pir`, `dark`, `relay`: kondisi sensor dan lampu
- `energy_wh`, `baseline_wh`, `saving_pct`: energi dengan kontrol, tanpa kontrol, dan persentase penghematan
- `cost_rp`, `baseline_cost_rp`: estimasi biaya listrik (tarif di `sketch.ino`)
- `anomaly`, `rssi`: status anomali dan kekuatan sinyal WiFi

## Struktur repositori
- `firmware/`: kode ESP32 dan rangkaian Wokwi
- `dashboard/`: ekspor dashboard ThingsBoard (JSON)
- `data/`: data ukur lux dan okupansi
- `docs/`: diagram, laporan, pitch deck

## Tim
Evan, Daffa, Dito, Devo

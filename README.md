# LumiSave Smart Lighting

Sistem kendali lampu otomatis hemat energi berbasis IoT dengan sensor cahaya (LDR) dan gerak (PIR).
Project mata kuliah Internet of Things, Tema C: Manajemen Energi dan Lingkungan Perkotaan untuk Fasilitas Kota dan UMKM.

## Cara kerja
- Lampu ON jika kondisi gelap **dan** ada gerak.
- Lampu OFF setelah 30 detik tanpa gerak, atau saat kondisi terang.
- Energi dihitung dari daya lampu x durasi ON, lalu dibandingkan dengan lampu tanpa kontrol (`baseline_wh`) untuk mendapatkan persentase penghematan (`saving_pct`).
- Anomali: lampu ON terus-menerus lebih dari 5 menit, ditandai buzzer dan flag `anomaly`.

## Alur sistem
LDR + PIR -> ESP32 -> WiFi -> MQTT -> Node-RED -> InfluxDB -> Grafana

## Komponen
ESP32, LDR module, PIR HC-SR501, relay module, OLED SSD1306, LED (mewakili lampu), buzzer.

## Menjalankan firmware
1. Salin `firmware/secrets.example.h` menjadi `firmware/secrets.h`, lalu sesuaikan isinya.
2. Buka proyek di Wokwi (impor `firmware/diagram.json` dan `firmware/sketch.ino`) atau di Arduino IDE.
3. Pasang library di `firmware/libraries.txt`: PubSubClient, ArduinoJson, Adafruit GFX Library, Adafruit SSD1306.
4. Jalankan simulasi dan buka Serial Monitor.

`secrets.h` tidak di-commit (lihat `.gitignore`).

## MQTT
| Item | Nilai |
|---|---|
| Broker | `broker.hivemq.com:1883` |
| Topik data | `lumisave/uns25evan/data` |
| Topik status | `lumisave/uns25evan/status` |
| Interval | 5 detik |

Contoh payload:
```json
{"seq":13,"ts":1794749097,"lux":13.2,"pir":true,"dark":true,"relay":true,"energy_wh":0,"baseline_wh":0.099,"saving_pct":99.9,"anomaly":false,"rssi":-63}
```
Kolom: `seq` (nomor urut, untuk packet loss), `ts` (epoch detik, untuk latensi), `lux`, `pir`, `dark`, `relay`, `energy_wh`, `baseline_wh`, `saving_pct`, `anomaly`, `rssi`.

## Struktur repositori
- `firmware/`: kode ESP32 dan rangkaian Wokwi
- `dashboard/`: flow Node-RED dan konfigurasi dashboard
- `data/`: data ukur lux dan okupansi
- `docs/`: diagram, laporan, pitch deck

## Tim
Evan, Daffa, Dito, Devo

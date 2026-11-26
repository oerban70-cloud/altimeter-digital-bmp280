# Altimeter Digital — ESP32 + BMP280 + OLED SSD1306

Alat kecil pengukur ketinggian: sensor BMP280 membaca tekanan udara,
ESP32 menghitung ketinggian dari tekanan itu dengan rumus barometrik,
lalu hasilnya tampil di layar OLED 128x64 — ketinggian dalam meter,
lengkap dengan tekanan dan suhu. Cocok untuk pendakian, paralayang,
atau sekadar tahu rumahmu berapa meter di atas permukaan laut.

Video penjelasan (60 detik):
link menyusul setelah upload

## Komponen

| Bagian | Keterangan |
|---|---|
| ESP32 DevKit | Otak alat |
| BMP280 | Sensor tekanan + suhu (I2C 0x76) |
| OLED SSD1306 128x64 | Tampil ketinggian + tekanan + suhu (I2C 0x3C) |

## Wiring

| Dari | Ke |
|---|---|
| BMP280 SDA | ESP32 GPIO21 |
| BMP280 SCL | ESP32 GPIO22 |
| BMP280 VCC | ESP32 3V3 |
| OLED SDA / SCL | Bus I2C yang sama (GPIO21 / GPIO22) |
| OLED VCC | ESP32 3V3 |
| Semua GND | Digabung ke ESP32 GND |

Skema lengkap: [schematic.png](schematic.png)

## Cara kerja

1. BMP280 mengukur tekanan udara sampai seperseratus hektopaskal.
2. ESP32 menghitung ketinggian dengan rumus barometrik: tekanan
   dibandingkan dengan tekanan acuan permukaan laut.
3. OLED menampilkan ketinggian (meter, satu desimal), tekanan (hPa),
   dan suhu (derajat Celsius), diperbarui tiap 500 milidetik.
4. Kalau sensor tidak ditemukan (alamat 0x76 maupun 0x77), OLED
   menampilkan pesan error dan alat berhenti — cek wiring I2C.

## Konfigurasi (di awal sketch)

- `TEKANAN_PERMUKAAN_LAUT_HPA` (1013,25) — tekanan acuan permukaan
  laut. Ganti dengan tekanan acuan lokal (mis. dari BMKG / bandara
  terdekat) supaya ketinggian lebih akurat; selisih 1 hPa kira-kira
  setara 8 meter.
- `PIN_I2C_SDA` (21) / `PIN_I2C_SCL` (22) — pin bus I2C.
- `INTERVAL_TAMPIL_MS` (500) — seberapa sering layar diperbarui.
- `ADDR_BMP280_UTAMA` (0x76) / `ADDR_BMP280_CADANGAN` (0x77) —
  sketch mencoba keduanya otomatis.

## Library yang dibutuhkan

| Library | Versi | Keterangan |
|---|---|---|
| Adafruit BMP280 Library | 3.0.0 | Sensor BMP280 |
| Adafruit SSD1306 | 2.5.17 | Layar OLED |
| Adafruit GFX Library | 1.13.0 | Grafis OLED |
| Adafruit Unified Sensor | 1.1.15 | Sensor abstraction |
| Adafruit BusIO | 1.17.4 | Bus I2C/SPI |

Semua library di atas juga tersedia sebagai ZIP di folder [`libraries/`](libraries/)
— install lewat Arduino IDE: Sketch → Include Library → Add .ZIP Library.

## Compile

```sh
arduino-cli compile --fqbn esp32:esp32:esp32 AltimeterBMP280
```

Terverifikasi 26 Nov 2026: sketch 337.437 byte (25% flash),
RAM 24.840 byte (7%), ESP32 core 4.0.0.

## Lisensi

MIT — lihat [LICENSE](LICENSE).

// Altimeter Digital — ESP32 + BMP280 + OLED SSD1306
//
// Alat kecil pengukur ketinggian: sensor BMP280 membaca tekanan udara,
// ESP32 menghitung ketinggian dari tekanan itu dengan rumus barometrik,
// lalu hasilnya tampil di layar OLED 128x64 (ketinggian, tekanan, suhu).
//
// Wiring (semua lewat bus I2C yang sama):
//   BMP280 VIN  -> ESP32 3V3
//   BMP280 GND  -> ESP32 GND
//   BMP280 SDA  -> ESP32 GPIO21
//   BMP280 SCL  -> ESP32 GPIO22
//   OLED   VIN  -> ESP32 3V3
//   OLED   GND  -> ESP32 GND
//   OLED   SDA  -> ESP32 GPIO21
//   OLED   SCL  -> ESP32 GPIO22
//
// Library yang dibutuhkan (Arduino IDE / arduino-cli):
//   Adafruit BMP280 Library 3.0.0
//   Adafruit SSD1306 2.5.17
//   Adafruit GFX Library 1.13.0
//   Adafruit Unified Sensor 1.1.15
//   Adafruit BusIO 1.17.4
//
// Compile:
//   arduino-cli compile --fqbn esp32:esp32:esp32 AltimeterBMP280
//
// Skema + repo: https://github.com/oerban70-cloud/altimeter-digital-bmp280

#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------------------------------------------------------------------------
// Konfigurasi — semua angka yang bisa diubah ada di sini, bukan di tengah kode
// ---------------------------------------------------------------------------

// Pin bus I2C (pin default ESP32 DevKit)
const uint8_t PIN_I2C_SDA = 21;
const uint8_t PIN_I2C_SCL = 22;

// Alamat I2C perangkat (cek dengan I2C scanner bila ragu)
const uint8_t ADDR_BMP280_UTAMA   = 0x76;  // modul BMP280 umumnya 0x76
const uint8_t ADDR_BMP280_CADANGAN = 0x77; // sebagian modul memakai 0x77
const uint8_t ADDR_OLED           = 0x3C;  // OLED SSD1306 128x64

// Ukuran layar OLED
const uint8_t OLED_LEBAR  = 128;
const uint8_t OLED_TINGGI = 64;

// Tekanan di permukaan laut (hPa) — acuan rumus barometrik.
// 1013,25 hPa adalah tekanan standar; ganti dengan tekanan acuan lokal
// (mis. dari BMKG / bandara terdekat) supaya ketinggian lebih akurat.
const float TEKANAN_PERMUKAAN_LAUT_HPA = 1013.25;

// Seberapa sering layar diperbarui (milidetik)
const unsigned long INTERVAL_TAMPIL_MS = 500;

// ---------------------------------------------------------------------------
// Objek perangkat
// ---------------------------------------------------------------------------

Adafruit_BMP280 bmp;  // sensor tekanan + suhu
Adafruit_SSD1306 oled(OLED_LEBAR, OLED_TINGGI, &Wire, -1);  // -1 = tanpa pin reset

// Satu paket hasil ukur, supaya loop tidak memanggil sensor berkali-kali.
struct Pengukuran {
  float suhuC;        // suhu udara (derajat Celsius)
  float tekananHPa;   // tekanan udara (hektopaskal)
  float ketinggianM;  // ketinggian di atas permukaan laut (meter)
};

// Deklarasi awal fungsi (prototipe) — wajib karena bacaPengukuran()
// mengembalikan struct buatan sendiri, yang tidak bisa diprototipe
// otomatis oleh Arduino bila definisinya ada di bawah loop().
Pengukuran bacaPengukuran();
void tampilkanJudul();
void tampilkanPengukuran(const Pengukuran &p);
void tampilkanError(const char *baris1, const char *baris2, const char *baris3);

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(100);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

  // Nyalakan OLED dulu supaya pesan error sensor tetap bisa tampil di layar.
  if (!oled.begin(SSD1306_SWITCHCAPVCC, ADDR_OLED)) {
    Serial.println("OLED tidak ditemukan di alamat 0x3C. Cek wiring SDA/SCL.");
    while (true) {
      delay(1000);  // berhenti di sini — tanpa layar alat tidak ada gunanya
    }
  }
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);

  // Sensor BMP280: coba alamat utama dulu, lalu alamat cadangan.
  if (!bmp.begin(ADDR_BMP280_UTAMA)) {
    Serial.println("BMP280 tidak ditemukan di 0x76, mencoba 0x77...");
    if (!bmp.begin(ADDR_BMP280_CADANGAN)) {
      tampilkanError("BMP280 TIDAK", "DITEMUKAN!", "Cek wiring I2C");
      Serial.println("BMP280 tidak ditemukan di 0x76 maupun 0x77. Cek wiring.");
      while (true) {
        delay(1000);  // berhenti — tanpa sensor tidak ada yang diukur
      }
    }
  }

  // Mode pengukuran: normal, oversampling tinggi untuk hasil yang stabil.
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,    // suhu
                  Adafruit_BMP280::SAMPLING_X16,   // tekanan
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_500);

  tampilkanJudul();
  Serial.println("Altimeter siap.");
}

// ---------------------------------------------------------------------------
// Loop utama
// ---------------------------------------------------------------------------

void loop() {
  static unsigned long waktuTerakhir = 0;
  unsigned long sekarang = millis();

  if (sekarang - waktuTerakhir >= INTERVAL_TAMPIL_MS) {
    waktuTerakhir = sekarang;
    tampilkanPengukuran(bacaPengukuran());
  }
}

// ---------------------------------------------------------------------------
// Pembacaan sensor
// ---------------------------------------------------------------------------

Pengukuran bacaPengukuran() {
  Pengukuran hasil;
  hasil.suhuC = bmp.readTemperature();
  // readPressure() mengembalikan Pascal; 1 hPa = 100 Pa.
  hasil.tekananHPa = bmp.readPressure() / 100.0;
  // Rumus barometrik: ketinggian dari tekanan relatif terhadap acuan laut.
  hasil.ketinggianM = bmp.readAltitude(TEKANAN_PERMUKAAN_LAUT_HPA);
  return hasil;
}

// ---------------------------------------------------------------------------
// Tampilan OLED
// ---------------------------------------------------------------------------

void tampilkanJudul() {
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setCursor(8, 10);
  oled.println("ALTIMETER");
  oled.setTextSize(1);
  oled.setCursor(20, 38);
  oled.println("BMP280 + OLED");
  oled.display();
  delay(1500);
}

void tampilkanPengukuran(const Pengukuran &p) {
  oled.clearDisplay();

  // Baris 1: ketinggian, besar — ini angka yang paling dicari.
  oled.setTextSize(2);
  oled.setCursor(0, 0);
  oled.print(p.ketinggianM, 1);
  oled.print(" m");

  // Baris 2: tekanan udara.
  oled.setTextSize(1);
  oled.setCursor(0, 24);
  oled.print("Tekanan: ");
  oled.print(p.tekananHPa, 1);
  oled.print(" hPa");

  // Baris 3: suhu udara.
  oled.setCursor(0, 38);
  oled.print("Suhu: ");
  oled.print(p.suhuC, 1);
  oled.print(" C");

  // Baris 4: acuan tekanan laut yang dipakai.
  oled.setCursor(0, 52);
  oled.print("Acuan laut: ");
  oled.print(TEKANAN_PERMUKAAN_LAUT_HPA, 2);
  oled.print(" hPa");

  oled.display();

  // Cermin ke Serial Monitor untuk debug.
  Serial.print("Ketinggian: ");
  Serial.print(p.ketinggianM, 1);
  Serial.print(" m | Tekanan: ");
  Serial.print(p.tekananHPa, 1);
  Serial.print(" hPa | Suhu: ");
  Serial.print(p.suhuC, 1);
  Serial.println(" C");
}

void tampilkanError(const char *baris1, const char *baris2, const char *baris3) {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 10);
  oled.println(baris1);
  oled.setCursor(0, 26);
  oled.println(baris2);
  oled.setCursor(0, 42);
  oled.println(baris3);
  oled.display();
}

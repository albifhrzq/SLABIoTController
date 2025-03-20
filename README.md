# Aquarium LED Controller

Sebuah pengontrol lampu akuarium berbasis ESP32 yang memungkinkan pengaturan intensitas LED berdasarkan waktu menggunakan RTC DS3231. Kontroler dapat diatur melalui aplikasi Flutter via koneksi Bluetooth.

## Fitur

- Kontrol 7 warna LED terpisah (Royal Blue, Biru, UV, Violet, Merah, Hijau, Putih)
- Pengaturan intensitas berdasarkan waktu (pagi, siang, malam)
- Penggunaan RTC DS3231 untuk penghitungan waktu yang akurat
- Perpindahan halus antara berbagai profil pencahayaan
- Koneksi Bluetooth untuk mengontrol dari aplikasi Flutter
- Mode otomatis dan manual
- Penyesuaian profil pencahayaan dan rentang waktu melalui aplikasi

## Kebutuhan Hardware

- ESP32 (CH340)
- RTC DS3231
- 7 Channel PWM LED Driver
- LED Strip/Module dengan warna:
  - Royal Blue
  - Biru
  - UV
  - Violet
  - Merah
  - Hijau
  - Putih
- Power Supply sesuai kebutuhan LED

## Pengkabelan

Sambungan pin:

| Komponen | Pin ESP32 |
|----------|-----------|
| Royal Blue LED | GPIO25 |
| Blue LED | GPIO26 |
| UV LED | GPIO27 |
| Violet LED | GPIO14 |
| Red LED | GPIO12 |
| Green LED | GPIO13 |
| White LED | GPIO15 |
| RTC DS3231 SDA | GPIO21 |
| RTC DS3231 SCL | GPIO22 |

## Instalasi

1. Pasang PlatformIO di VS Code
2. Buka projek ini
3. Unduh semua dependensi yang diperlukan
4. Kompilasi dan unggah ke perangkat ESP32

## Penggunaan

### Mode Otomatis

Setelah diunggah, sistem akan secara otomatis menjalankan profil pencahayaan berdasarkan waktu dari RTC:

- **Pagi (07:00-10:00)**: Pencahayaan lembut dengan dominasi warna putih dan biru
- **Siang (10:00-17:00)**: Pencahayaan terang dengan intensitas penuh
- **Sore (17:00-21:00)**: Pencahayaan hangat dengan dominasi warna merah
- **Malam (21:00-07:00)**: Pencahayaan redup dengan dominasi warna biru

### Kontrol via Aplikasi Flutter

Sistem dapat dikontrol melalui aplikasi Flutter yang terhubung via Bluetooth. Fitur aplikasi meliputi:

1. **Kontrol Manual**: Mengatur intensitas setiap LED secara terpisah
2. **Mode Otomatis**: Menjalankan siklus pencahayaan otomatis berdasarkan waktu
3. **Kustomisasi Profil**: Mengubah profil pencahayaan untuk setiap waktu (pagi, siang, sore, malam)
4. **Kustomisasi Waktu**: Mengatur rentang waktu untuk setiap profil pencahayaan

### Menghubungkan Aplikasi

1. Pastikan Bluetooth aktif di perangkat Android/iOS
2. Buka aplikasi Flutter Aquarium Controller
3. Scan perangkat Bluetooth terdekat
4. Pilih "Aquarium LED Controller" dari daftar
5. Setelah terhubung, aplikasi akan menampilkan status dan pengaturan saat ini

## Kustomisasi

Untuk pengembang, kode dapat dikustomisasi dengan cara berikut:

1. **Profil Pencahayaan**: Edit nilai default di fungsi `setDefaultProfiles()` di `LedController.cpp`
2. **Rentang Waktu**: Ubah nilai waktu default di konstruktor `LedController`
3. **UUID Bluetooth**: Sesuaikan UUID di `BluetoothService.h` jika diperlukan

## Aplikasi Flutter

Kode sumber untuk aplikasi Flutter pendamping tersedia di: [Aquarium Controller Flutter App](https://github.com/username/aquarium-flutter-app)

Fitur aplikasi:
- Antarmuka yang intuitif untuk kontrol lampu
- Pengatur intensitas dengan slider
- Penjadwalan dan pengaturan profil
- Visualisasi warna dan pencahayaan
- Penyimpanan pengaturan lokal 
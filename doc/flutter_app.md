# Panduan Pengembangan Aplikasi Flutter untuk Kontroler Lampu Akuarium

Dokumen ini berisi petunjuk untuk membuat aplikasi Flutter yang dapat mengontrol lampu akuarium melalui koneksi Bluetooth LE.

## Struktur Aplikasi

Aplikasi Flutter ini terdiri dari beberapa komponen utama:

1. **Bluetooth Manager**: Menangani koneksi BLE dan komunikasi dengan ESP32
2. **UI Halaman Utama**: Tampilan utama untuk kontrol lampu
3. **UI Pengaturan Profil**: Untuk mengkonfigurasi profil cahaya
4. **UI Pengaturan Waktu**: Untuk mengatur rentang waktu
5. **Penyimpanan Lokal**: Untuk menyimpan pengaturan dan preferensi

## Paket Flutter yang Dibutuhkan

```yaml
dependencies:
  flutter:
    sdk: flutter
  flutter_blue_plus: ^1.5.2
  shared_preferences: ^2.1.1
  provider: ^6.0.5
  json_annotation: ^4.8.1
  intl: ^0.18.1
  flutter_colorpicker: ^1.0.3
  charts_flutter: ^0.12.0
```

## Komunikasi Bluetooth

### Karakteristik dan UUID

Aplikasi Flutter perlu menggunakan UUID yang sama dengan yang ditentukan di controller ESP32:

```dart
// UUID yang sama dengan di ESP32
const String SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
const String PROFILE_CHAR_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
const String TIME_RANGES_CHAR_UUID = "43f128bd-b9d2-4a83-b9e6-355b7b55a82f";
const String MANUAL_CONTROL_CHAR_UUID = "218b3c65-22e0-4ed8-9df5-77c4e79a60d3";
const String CURRENT_TIME_CHAR_UUID = "1c95d5e3-d8f7-413a-bf3d-9002ecc1c188";
const String MODE_CHAR_UUID = "49c3dc32-1b9c-4dab-9ff5-bd11a6f6086d";
```

### Format Data JSON

Komunikasi dengan ESP32 menggunakan format JSON. Berikut format yang digunakan:

#### Pengaturan Profil:
```json
{
  "type": "morning|midday|evening|night|current",
  "profile": {
    "royalBlue": 100,
    "blue": 150,
    "uv": 50,
    "violet": 50,
    "red": 150,
    "green": 100,
    "white": 200
  }
}
```

#### Kontrol Manual:
```json
{
  "led": "royalBlue|blue|uv|violet|red|green|white",
  "value": 0-255
}
```

#### Rentang Waktu:
```json
{
  "morningStart": 7,
  "middayStart": 10,
  "eveningStart": 17,
  "nightStart": 21
}
```

#### Mode Operasi:
```
"manual" atau "auto"
```

## Contoh Kode

### Mencari dan Menghubungkan ke Perangkat

```dart
FlutterBluePlus flutterBlue = FlutterBluePlus.instance;

Future<void> scanAndConnect() async {
  // Mulai scan
  flutterBlue.startScan(timeout: Duration(seconds: 4));
  
  // Dengarkan hasil scan
  flutterBlue.scanResults.listen((results) {
    for (ScanResult r in results) {
      if (r.device.name == "Aquarium LED Controller") {
        // Berhenti scan
        flutterBlue.stopScan();
        // Hubungkan ke perangkat
        connectToDevice(r.device);
        break;
      }
    }
  });
}

Future<void> connectToDevice(BluetoothDevice device) async {
  await device.connect();
  
  // Discover services
  List<BluetoothService> services = await device.discoverServices();
  for (BluetoothService service in services) {
    if (service.uuid.toString() == SERVICE_UUID) {
      // Temukan karakteristik yang dibutuhkan
      for (BluetoothCharacteristic c in service.characteristics) {
        if (c.uuid.toString() == PROFILE_CHAR_UUID) {
          profileCharacteristic = c;
        } else if (c.uuid.toString() == TIME_RANGES_CHAR_UUID) {
          timeRangesCharacteristic = c;
        }
        // ... lanjutkan dengan karakteristik lainnya
      }
    }
  }
}
```

### Mengirim Perintah Kontrol Manual

```dart
Future<void> setLedIntensity(String led, int value) async {
  if (manualControlCharacteristic != null) {
    // Format perintah JSON
    Map<String, dynamic> command = {
      "led": led,
      "value": value
    };
    
    // Konversi ke string JSON
    String jsonCommand = jsonEncode(command);
    
    // Kirim perintah
    await manualControlCharacteristic.write(utf8.encode(jsonCommand));
  }
}
```

### Mengganti Mode

```dart
Future<void> setMode(String mode) async {
  if (modeCharacteristic != null) {
    // Kirim mode ("manual" atau "auto")
    await modeCharacteristic.write(utf8.encode(mode));
  }
}
```

## Desain UI

### Halaman Utama

Halaman utama aplikasi harus mencakup:

1. Indikator status koneksi Bluetooth
2. Toggle switch untuk memilih mode manual/otomatis
3. Tampilan profil cahaya saat ini
4. Slider untuk menyesuaikan intensitas setiap LED dalam mode manual
5. Tombol untuk membuka halaman pengaturan profil dan waktu

### Halaman Pengaturan Profil

Halaman ini memungkinkan pengguna untuk:

1. Memilih profil (pagi, siang, sore, malam)
2. Menyesuaikan intensitas setiap LED untuk profil yang dipilih
3. Visualisasi warna resultan 
4. Menyimpan profil ke controller

### Halaman Pengaturan Waktu

Halaman ini memungkinkan pengguna untuk:

1. Mengatur jam mulai untuk setiap profil (pagi, siang, sore, malam)
2. Visualisasi rentang waktu dalam bentuk grafik
3. Menyimpan pengaturan waktu ke controller

## Tips Pengembangan

1. **Responsif**: Pastikan UI bekerja dengan baik di berbagai ukuran layar
2. **Caching**: Simpan pengaturan terakhir di penyimpanan lokal
3. **Error Handling**: Tangani kasus koneksi terputus dan error komunikasi
4. **Battery Optimization**: Optimalkan penggunaan Bluetooth untuk menghemat baterai
5. **Testing**: Uji aplikasi dengan perangkat ESP32 yang sebenarnya

## Deployment

Untuk men-deploy aplikasi ke perangkat:

1. Build APK/App Bundle untuk Android:
   ```
   flutter build apk --release
   ```

2. Build IPA untuk iOS:
   ```
   flutter build ios --release
   ```

## Troubleshooting

- **Koneksi Gagal**: Pastikan ESP32 menyala dan dalam jangkauan
- **Perintah Tidak Direspon**: Verifikasi UUID dan format JSON yang digunakan
- **Delay/Lag**: Pertimbangkan untuk mengurangi frekuensi komunikasi
- **iOS Permission**: Tinjau Info.plist untuk izin Bluetooth yang tepat 
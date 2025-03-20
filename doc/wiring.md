# Pengkabelan Sistem Kontrol Lampu Akuarium

Dokumen ini berisi petunjuk pengkabelan untuk sistem kontrol lampu akuarium menggunakan ESP32 dan RTC DS3231.

## Skema Pengkabelan

### ESP32 ke RTC DS3231

| ESP32 | RTC DS3231 |
|-------|------------|
| 3.3V  | VCC        |
| GND   | GND        |
| GPIO21 (SDA) | SDA  |
| GPIO22 (SCL) | SCL  |

### ESP32 ke LED Driver

Gunakan driver LED PWM yang sesuai untuk mengontrol LED. Driver LED harus mampu menyediakan arus yang cukup untuk strip/modul LED Anda.

| ESP32 | LED Driver |
|-------|------------|
| GPIO25 | Channel 1 (Royal Blue) |
| GPIO26 | Channel 2 (Blue) |
| GPIO27 | Channel 3 (UV) |
| GPIO14 | Channel 4 (Violet) |
| GPIO12 | Channel 5 (Red) |
| GPIO13 | Channel 6 (Green) |
| GPIO15 | Channel 7 (White) |
| GND   | GND        |

### Catatan Tambahan

1. **Power Supply**:
   - ESP32 dapat ditenagai melalui USB atau sumber eksternal 5V
   - LED driver membutuhkan power supply terpisah dengan arus yang sesuai (biasanya 12V atau 24V)
   - Pastikan ground semua komponen dihubungkan bersama

2. **Level Shifter**:
   - Jika driver LED memerlukan sinyal kontrol >3.3V, gunakan level shifter untuk mengkonversi sinyal ESP32 (3.3V) ke level yang dibutuhkan

3. **Resistor Pull-up**:
   - Jika komunikasi I2C dengan RTC tidak stabil, tambahkan resistor pull-up 4.7kΩ ke line SDA dan SCL
   
4. **Kapasitor**:
   - Tambahkan kapasitor 100µF dan 100nF di dekat ESP32 dan driver LED untuk mengurangi noise

## Rekomendasi Tambahan

1. **Isolasi**: Selalu isolasi semua sambungan dengan heat shrink atau selotip listrik
2. **Pengujian**: Uji setiap kanal secara terpisah sebelum menjalankan sistem lengkap
3. **Pendinginan**: Pastikan driver LED dan power supply memiliki pendinginan yang cukup

## Diagram Kabel

```
ESP32                RTC DS3231
┌────────┐           ┌─────────┐
│        │           │         │
│    3V3 ├───────────┤ VCC     │
│    GND ├───────────┤ GND     │
│ GPIO21 ├───────────┤ SDA     │
│ GPIO22 ├───────────┤ SCL     │
│        │           │         │
└────────┘           └─────────┘


ESP32                LED DRIVER           LED STRIPS
┌────────┐           ┌─────────┐          ┌─────────┐
│        │           │         │          │         │
│ GPIO25 ├───────────┤ CH1     ├──────────┤ R.BLUE  │
│ GPIO26 ├───────────┤ CH2     ├──────────┤ BLUE    │
│ GPIO27 ├───────────┤ CH3     ├──────────┤ UV      │
│ GPIO14 ├───────────┤ CH4     ├──────────┤ VIOLET  │
│ GPIO12 ├───────────┤ CH5     ├──────────┤ RED     │
│ GPIO13 ├───────────┤ CH6     ├──────────┤ GREEN   │
│ GPIO15 ├───────────┤ CH7     ├──────────┤ WHITE   │
│    GND ├───────────┤ GND     │          │         │
│        │           │         │          │         │
└────────┘           └─────────┘          └─────────┘
                          │
                          │
                     ┌────┴────┐
                     │         │
                     │ POWER   │
                     │ SUPPLY  │
                     │         │
                     └─────────┘
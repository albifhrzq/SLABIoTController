# Aquarium LED Controller

An ESP32-based aquarium lighting controller that allows LED intensity adjustment based on time using the RTC DS3231 module. The controller can be configured via a Flutter application through Bluetooth connection.

## Features

- Control 7 separate LED colors (Royal Blue, Blue, UV, Violet, Red, Green, White)  
- Time-based intensity control (morning, afternoon, evening, night)  
- Uses **RTC DS3231** for accurate timekeeping  
- Smooth transition between different lighting profiles  
- Bluetooth connection for control via a Flutter app  
- Automatic and manual modes  
- Lighting profile and time range customization through the app  

## Hardware Requirements

- ESP32 (CH340)  
- RTC DS3231  
- 7-Channel PWM LED Driver  
- LED Strip/Modules with the following colors:
  - Royal Blue  
  - Blue  
  - UV  
  - Violet  
  - Red  
  - Green  
  - White  
- Power supply suitable for the LED setup  

## Wiring

| Component        | ESP32 Pin |
|------------------|-----------|
| Royal Blue LED   | GPIO25    |
| Blue LED         | GPIO26    |
| UV LED           | GPIO27    |
| Violet LED       | GPIO14    |
| Red LED          | GPIO12    |
| Green LED        | GPIO13    |
| White LED        | GPIO15    |
| RTC DS3231 SDA   | GPIO21    |
| RTC DS3231 SCL   | GPIO22    |

## Installation

1. Install PlatformIO on VS Code  
2. Open this project in PlatformIO  
3. Download all required dependencies  
4. Compile and upload the code to your ESP32 device  

## Usage

### Automatic Mode

Once uploaded, the system will automatically run lighting profiles based on the RTC's time:

- **Morning (07:00–10:00)**: Soft lighting dominated by white and blue LEDs  
- **Afternoon (10:00–17:00)**: Bright lighting with full intensity  
- **Evening (17:00–21:00)**: Warm lighting with red-dominated tones  
- **Night (21:00–07:00)**: Dim lighting with a blue-dominated atmosphere  

### Control via Flutter App

The system can be controlled via a Flutter app connected through Bluetooth. App features include:

1. **Manual Control**: Adjust intensity of each LED individually  
2. **Automatic Mode**: Run time-based lighting cycles  
3. **Profile Customization**: Modify lighting profiles for each time period  
4. **Time Customization**: Set custom time ranges for each lighting profile  

### Connecting the App

1. Ensure Bluetooth is enabled on your Android/iOS device  
2. Open the Flutter Aquarium Controller app  
3. Scan for nearby Bluetooth devices  
4. Select **"Aquarium LED Controller"** from the list  
5. Once connected, the app will display current status and settings  

## Customization

For developers, the code can be customized as follows:

- **Lighting Profiles**: Edit default values in `setDefaultProfiles()` inside `LedController.cpp`  
- **Time Ranges**: Modify default times in the `LedController` constructor  
- **Bluetooth UUID**: Adjust UUID in `BluetoothService.h` if needed  

## Flutter App

The source code for the companion Flutter application is available at:  
[**Aquarium Controller Flutter App**](https://github.com/username/aquarium-flutter-app)

### App Features

- Intuitive interface for light control  
- Intensity sliders for each LED  
- Scheduling and profile management  
- Lighting color visualization  
- Local settings storage  

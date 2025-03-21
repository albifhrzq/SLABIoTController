# SLAB IoT Aquarium Controller

An ESP32-based aquarium LED controller that allows users to set lighting profiles for different times throughout the day. This system uses a Real-Time Clock (RTC) for time tracking and ESP32 for LED control via PWM.

## Features

- Control of 7 separate LED channels (Royal Blue, Blue, UV, Violet, Red, Green, White)
- Time-based automatic operation mode with 4 lighting profiles (morning, midday, evening, night)
- Manual operation mode for direct LED intensity adjustment
- WiFi interface for setup and monitoring
- RESTful web API interface for client application integration
- Settings storage using ESP32 preferences

## Transition from Bluetooth to WiFi

This project originally used Bluetooth Low Energy (BLE) for communication with client applications. Now, the project has been converted to use WiFi and HTTP protocol instead of BLE. Key changes include:

1. Replacement of `BluetoothService` with `WiFiService`
2. Implementation of an asynchronous web server using ESPAsyncWebServer
3. Exposing RESTful API for all the same functionality that was available through BLE

## Required Hardware

- ESP32 Development Board
- RTC DS3231
- MOSFET or LED driver for each channel
- Suitable power supply for LEDs
- Aquarium LEDs (Royal Blue, Blue, UV, Violet, Red, Green, White)

## Hardware Connections

```
ESP32 GPIO25 -> MOSFET Gate for Royal Blue LED
ESP32 GPIO26 -> MOSFET Gate for Blue LED
ESP32 GPIO27 -> MOSFET Gate for UV LED
ESP32 GPIO14 -> MOSFET Gate for Violet LED
ESP32 GPIO12 -> MOSFET Gate for Red LED
ESP32 GPIO13 -> MOSFET Gate for Green LED
ESP32 GPIO23 -> MOSFET Gate for White LED
ESP32 SDA (GPIO21) -> SDA on RTC DS3231
ESP32 SCL (GPIO22) -> SCL on RTC DS3231
```

## Software Setup

1. Clone this repository
2. Open the project in PlatformIO
3. Edit WIFI_SSID and WIFI_PASSWORD in src/main.cpp with your WiFi credentials
4. Build and upload to ESP32

## API Endpoints

The server provides several RESTful API endpoints:

- `GET /api/profile?type=[morning|midday|evening|night]` - Get lighting profile by type
- `POST /api/profile` - Set lighting profile (JSON format)
- `GET /api/timeranges` - Get time ranges for each profile
- `POST /api/timeranges` - Set time ranges (JSON format)
- `POST /api/manual` - Control LEDs manually (JSON format)
- `GET /api/time` - Get current time from RTC
- `GET /api/mode` - Get current operation mode (manual/auto)
- `POST /api/mode` - Set operation mode (JSON format)

## API Request Examples

### Get Current Profile

```
GET /api/profile
```

### Set Profile

```json
POST /api/profile
{
  "type": "morning",
  "profile": {
    "royalBlue": 100,
    "blue": 120,
    "uv": 50,
    "violet": 40,
    "red": 200,
    "green": 180,
    "white": 150
  }
}
```

### Control LED Manually

```json
POST /api/manual
{
  "led": "royalBlue",
  "value": 200
}
```

### Change Operation Mode

```json
POST /api/mode
{
  "mode": "auto"
}
```

## Flutter App Integration

The Flutter application needs to be updated to use HTTP protocol instead of BLE. Use the `http` package to make API requests to the endpoints provided by the ESP32.

## License

MIT

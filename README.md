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

### Profile Management
- `GET /api/profile` - Get current lighting profile
- `GET /api/profile?type={type}` - Get profile by type (morning, midday, evening, night)
- `POST /api/profile` - Save lighting profile

### Time Management
- `GET /api/time` - Get current time from RTC controller
- `POST /api/time` - Synchronize time with controller
- `GET /api/timeranges` - Get time ranges for each profile
- `POST /api/timeranges` - Save time ranges for profiles

### LED Control
- `POST /api/manual` - Control individual LED
  ```json
  {
    "led": "royalBlue",
    "value": 150
  }
  ```
- `POST /api/manual/all` - Control all LEDs simultaneously
  ```json
  {
    "royalBlue": 150,
    "blue": 200,
    "uv": 50,
    "violet": 100,
    "red": 120,
    "green": 180,
    "white": 220
  }
  ```
  Or with wrapper:
  ```json
  {
    "leds": {
      "royalBlue": 150,
      "blue": 200,
      "uv": 50,
      "violet": 100,
      "red": 120,
      "green": 180,
      "white": 220
    }
  }
  ```

### Mode Control
- `GET /api/mode` - Get current operation mode (manual/auto)
- `POST /api/mode` - Change operation mode

### Connection & Diagnostics
- `GET /api/ping` - Check connection to controller
- `GET /api/status` - Get system status

## API Request Examples

### Get Current Profile

## License

MIT License

Copyright (c) 2024 SLAB IoT Aquarium Controller

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
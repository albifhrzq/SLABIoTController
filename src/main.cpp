#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <ArduinoJson.h>
#include "LedController.h"
#include "BluetoothService.h"

// Pin definitions
#define PIN_ROYAL_BLUE 25  // Royal Blue LED
#define PIN_BLUE       26  // Blue LED
#define PIN_UV         27  // UV LED
#define PIN_VIOLET     14  // Violet LED
#define PIN_RED        12  // Red LED
#define PIN_GREEN      13  // Green LED
#define PIN_WHITE      23  // White LED

// PWM channels (0-15)
#define CHANNEL_ROYAL_BLUE 0
#define CHANNEL_BLUE       1
#define CHANNEL_UV         2
#define CHANNEL_VIOLET     3
#define CHANNEL_RED        4
#define CHANNEL_GREEN      5
#define CHANNEL_WHITE      6

// PWM properties
#define PWM_FREQ      5000  // Frequency in Hz
#define PWM_RESOLUTION    8  // 8-bit resolution (0-255)

// Global objects
RTC_DS3231 rtc;  // RTC instance
LedController* ledController;  // LED controller
BluetoothService* bluetoothService;  // Bluetooth service

void setup() {
  // Start serial communication
  Serial.begin(115200);
  Serial.println("Initializing SLAB IoT Aquarium Controller...");
  
  // Initialize I2C communication for RTC
  Wire.begin();
  
  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC! Check wiring and try again.");
    while (1);
  }
  
  // Set RTC time if it was lost (e.g., when the battery was removed)
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time to compile time!");
    // Following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  // Create LED controller instance
  ledController = new LedController(
    PIN_ROYAL_BLUE, PIN_BLUE, PIN_UV, PIN_VIOLET, PIN_RED, PIN_GREEN, PIN_WHITE,
    CHANNEL_ROYAL_BLUE, CHANNEL_BLUE, CHANNEL_UV, CHANNEL_VIOLET, CHANNEL_RED, CHANNEL_GREEN, CHANNEL_WHITE,
    PWM_FREQ, PWM_RESOLUTION,
    &rtc
  );
  
  // Initialize LED controller
  ledController->begin();
  
  // Create and initialize Bluetooth service
  bluetoothService = new BluetoothService(ledController);
  bluetoothService->begin();
  
  Serial.println("Setup complete.");
  
  // Print current time
  DateTime now = rtc.now();
  Serial.print("Current time: ");
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
}

void loop() {
  // Update LED controller
  ledController->update();
  
  // Update BLE characteristics (typically happens only when connected)
  bluetoothService->updateCharacteristics();
  
  // Short delay to prevent overwhelming the system
  delay(1000);
}

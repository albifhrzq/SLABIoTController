#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <ArduinoJson.h>
#include "LedController.h"
#include "WiFiService.h"

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

// WiFi AP mode settings
#define AP_SSID "SLAB-Aquarium-LED"
#define AP_PASSWORD "12345678"

// Global objects
RTC_DS3231 rtc;  // RTC instance
LedController* ledController;  // LED controller
WiFiService* wifiService;  // WiFi service

// Flag to indicate if WiFi initialization was successful
bool wifiInitialized = false;

// Function to initialize WiFi with retry
void initializeWiFi() {
  for (int attempt = 0; attempt < 3; attempt++) {
    Serial.print("WiFi initialization attempt ");
    Serial.print(attempt + 1);
    Serial.println("/3...");
    
    // Create and initialize WiFi service in AP mode
    wifiService = new WiFiService(ledController, AP_SSID, AP_PASSWORD);
    wifiService->begin();
    
    // Wait longer for AP to initialize properly
    for (int i = 0; i < 5; i++) {
      Serial.print(".");
      delay(500);
    }
    Serial.println();
    
    // Reset WiFi hardware jika ini adalah percobaan kedua atau ketiga
    if (attempt > 0) {
      Serial.println("Resetting WiFi hardware...");
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      delay(1000);
    }
    
    // Check if AP is running
    if (wifiService->isConnected()) {
      wifiInitialized = true;
      Serial.println("WiFi initialized successfully");
      
      IPAddress ip = wifiService->getIP();
      Serial.print("AP IP Address: ");
      Serial.println(ip);
      
      Serial.println("Connect to WiFi network named '" AP_SSID "' with password '" AP_PASSWORD "'");
      Serial.println("Then access the control panel at http://" + ip.toString() + "/");
      break;
    } else {
      Serial.println("WiFi initialization failed, retrying...");
      delete wifiService;
      wifiService = nullptr;
      
      // Wait longer before retry to give WiFi hardware waktu untuk reset sepenuhnya
      Serial.println("Waiting for WiFi hardware to reset...");
      delay(5000);
    }
  }
  
  if (!wifiInitialized) {
    Serial.println("WARNING: WiFi initialization failed after multiple attempts!");
    Serial.println("LED controller will still function, but remote control won't be available.");
    Serial.println("Device will auto-restart in 1 hour to try again.");
  }
}

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
  
  // Initialize WiFi with retry
  initializeWiFi();
  
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
  
  Serial.println("Setup complete.");
}

void loop() {
  // Update LED controller
  ledController->update();
  
  // Update WiFi service if initialized
  if (wifiInitialized && wifiService != nullptr) {
    wifiService->update();
  } else {
    // Check if it's time to retry WiFi (every hour)
    static unsigned long lastWiFiRetry = 0;
    if (millis() - lastWiFiRetry > 3600000) { // 1 hour
      lastWiFiRetry = millis();
      Serial.println("Restarting device to retry WiFi initialization...");
      delay(1000);
      ESP.restart();
    }
  }
  
  // Short delay to prevent overwhelming the system
  delay(1000);
}

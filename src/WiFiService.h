#ifndef WIFI_SERVICE_H
#define WIFI_SERVICE_H

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "LedController.h"

class WiFiService {
private:
  LedController* ledController;
  AsyncWebServer* server;
  bool deviceConnected;
  
  // WiFi credentials
  const char* ssid;
  const char* password;
  
  // Preferences untuk menyimpan konfigurasi
  Preferences preferences;
  
  // Variabel untuk melacak status WiFi
  unsigned long lastConnectAttempt;
  int reconnectAttempts;
  bool apActive;
  
  // Method for handling API endpoints
  void setupApiEndpoints();
  void handleCors(AsyncWebServerRequest* request);
  void handleNotFound(AsyncWebServerRequest* request);
  
  // API handlers
  void handleManualControl(AsyncWebServerRequest* request, uint8_t* data, size_t len);
  void handleManualControlAll(AsyncWebServerRequest* request, uint8_t* data, size_t len);
  void handleGetCurrentTime(AsyncWebServerRequest* request);
  void handleSetTime(AsyncWebServerRequest* request, uint8_t* data, size_t len);
  void handleSetMode(AsyncWebServerRequest* request, uint8_t* data, size_t len);
  void handleGetMode(AsyncWebServerRequest* request);
  void handlePing(AsyncWebServerRequest* request);
  
  // Hourly schedule handlers
  void handleGetHourlySchedule(AsyncWebServerRequest* request);
  void handleSetHourlySchedule(AsyncWebServerRequest* request, uint8_t* data, size_t len);
  void handleGetHourProfile(AsyncWebServerRequest* request);
  void handleSetHourProfile(AsyncWebServerRequest* request, uint8_t* data, size_t len);
  
  // Helper methods untuk WiFi
  void startAP();
  void checkWiFiStatus();
  void restartWiFi();
  
public:
  // Constructor
  WiFiService(LedController* ledController, const char* ssid, const char* password);
  
  // Destructor
  ~WiFiService();
  
  // Initialize WiFi service
  void begin();
  
  // Status methods
  bool isConnected();
  IPAddress getIP();
  
  // Update WiFi service (if needed for periodic tasks)
  void update();
};

#endif // WIFI_SERVICE_H
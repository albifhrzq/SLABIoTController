#ifndef WIFI_SERVICE_H
#define WIFI_SERVICE_H

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "LedController.h"

class WiFiService {
private:
  LedController* ledController;
  AsyncWebServer* server;
  bool deviceConnected;
  
  // WiFi credentials
  const char* ssid;
  const char* password;
  
  // Method for handling API endpoints
  void setupApiEndpoints();
  void handleCors(AsyncWebServerRequest* request);
  void handleNotFound(AsyncWebServerRequest* request);
  
  // API handlers
  void handleGetProfile(AsyncWebServerRequest* request);
  void handleSetProfile(AsyncWebServerRequest* request, uint8_t* data, size_t len);
  void handleGetTimeRanges(AsyncWebServerRequest* request);
  void handleSetTimeRanges(AsyncWebServerRequest* request, uint8_t* data, size_t len);
  void handleManualControl(AsyncWebServerRequest* request, uint8_t* data, size_t len);
  void handleGetCurrentTime(AsyncWebServerRequest* request);
  void handleSetMode(AsyncWebServerRequest* request, uint8_t* data, size_t len);
  void handleGetMode(AsyncWebServerRequest* request);
  
public:
  // Constructor
  WiFiService(LedController* ledController, const char* ssid, const char* password);
  
  // Initialize WiFi service
  void begin();
  
  // Status methods
  bool isConnected();
  IPAddress getIP();
  
  // Update WiFi service (if needed for periodic tasks)
  void update();
};

#endif // WIFI_SERVICE_H
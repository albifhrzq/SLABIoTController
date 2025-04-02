#include "WiFiService.h"

WiFiService::WiFiService(LedController* ledController, const char* ssid, const char* password) {
  this->ledController = ledController;
  this->ssid = ssid;
  this->password = password;
  this->deviceConnected = false;
  this->server = new AsyncWebServer(80);
  this->lastConnectAttempt = 0;
  this->reconnectAttempts = 0;
  this->apActive = false;
}

void WiFiService::begin() {
  // Initialize preferences
  preferences.begin("wifi_config", false);
  
  // Start in Access Point mode directly
  Serial.println("Starting Access Point mode...");
  
  // Start AP
  startAP();
  
  // Setup endpoint API
  setupApiEndpoints();
  
  // Start server
  server->begin();
  Serial.println("HTTP server started in AP mode");
  deviceConnected = true;
}

void WiFiService::startAP() {
  // Shutdown WiFi before reconfiguring
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(500);
  
  // Configure WiFi in AP mode
  WiFi.mode(WIFI_AP);
  
  // Konfigurasi IP statis untuk AP (opsional, tetapi memastikan konsistensi)
  // Gunakan IP 192.168.4.1 (default untuk ESP32 AP)
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  WiFi.softAPConfig(local_IP, gateway, subnet);
  
  // Create access point with the SSID and password provided in the constructor
  bool result = WiFi.softAP(ssid, password);
  
  if (result) {
    apActive = true;
    Serial.print("Access Point started. IP address: ");
    Serial.println(WiFi.softAPIP());
    
    // Save status to preferences
    preferences.putBool("ap_active", true);
  } else {
    apActive = false;
    Serial.println("Failed to start Access Point!");
    
    // Try to restart WiFi in 5 seconds
    lastConnectAttempt = millis() - 25000; // Will trigger reconnect in 5 seconds
  }
}

void WiFiService::checkWiFiStatus() {
  // Check if AP is active but no clients connected for a long time
  if (apActive) {
    // Nothing to do here, just check if AP is still running
    if (WiFi.getMode() != WIFI_AP && WiFi.getMode() != WIFI_AP_STA) {
      Serial.println("AP mode unexpectedly disabled, restarting AP...");
      startAP();
    }
  }
}

void WiFiService::restartWiFi() {
  Serial.println("Restarting WiFi...");
  
  // Shutdown WiFi
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(1000);
  
  // Start AP again
  startAP();
  
  // Reset reconnect counter if successful
  if (apActive) {
    reconnectAttempts = 0;
  } else {
    reconnectAttempts++;
  }
}

void WiFiService::setupApiEndpoints() {
  // CORS handler
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Credentials", "true");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Private-Network", "true");
  
  // Handle OPTIONS method untuk CORS
  server->onNotFound([this](AsyncWebServerRequest *request) {
    this->handleNotFound(request);
  });
  
  // Root endpoint untuk testing
  server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "<html><body><h1>SLAB Aquarium LED Controller</h1><p>API is running</p></body></html>");
  });
  
  // API ping untuk keep-alive
  server->on("/api/ping", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handlePing(request);
  });
  
  // API untuk profil lampu
  server->on("/api/profile", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleGetProfile(request);
  });
  
  server->on("/api/profile", HTTP_POST, 
    [](AsyncWebServerRequest *request) {},
    NULL,
    [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      this->handleSetProfile(request, data, len);
    }
  );
  
  // API untuk rentang waktu
  server->on("/api/timeranges", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleGetTimeRanges(request);
  });
  
  server->on("/api/timeranges", HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    NULL,
    [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      this->handleSetTimeRanges(request, data, len);
    }
  );
  
  // API untuk kontrol manual
  server->on("/api/manual", HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    NULL,
    [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      this->handleManualControl(request, data, len);
    }
  );
  
  // API untuk waktu saat ini
  server->on("/api/time", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleGetCurrentTime(request);
  });
  
  server->on("/api/time", HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    NULL,
    [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      this->handleSetTime(request, data, len);
    }
  );
  
  // API untuk mode operasi
  server->on("/api/mode", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleGetMode(request);
  });
  
  server->on("/api/mode", HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    NULL,
    [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      this->handleSetMode(request, data, len);
    }
  );
  
  // API untuk restart WiFi
  server->on("/api/wifi/restart", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->restartWiFi();
    request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"WiFi restarting\"}");
  });
}

void WiFiService::handleCors(AsyncWebServerRequest* request) {
  if (request->method() == HTTP_OPTIONS) {
    AsyncWebServerResponse *response = request->beginResponse(204);
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
    response->addHeader("Access-Control-Allow-Credentials", "true");
    response->addHeader("Access-Control-Allow-Private-Network", "true");
    response->addHeader("Access-Control-Max-Age", "86400"); // 24 jam
    request->send(response);
  }
}

void WiFiService::handleNotFound(AsyncWebServerRequest* request) {
  if (request->method() == HTTP_OPTIONS) {
    handleCors(request);
  } else {
    Serial.print("Request not found: ");
    Serial.println(request->url());
    
    // Untuk debugging, tampilkan informasi request
    int headers = request->headers();
    for (int i = 0; i < headers; i++) {
      const AsyncWebHeader* h = request->getHeader(i);
      Serial.printf("  %s: %s\n", h->name().c_str(), h->value().c_str());
    }
    
    request->send(404, "text/plain", "Not found");
  }
}

void WiFiService::handleGetProfile(AsyncWebServerRequest* request) {
  String type = "";
  if (request->hasParam("type")) {
    type = request->getParam("type")->value();
  }
  
  String jsonResponse = "";
  
  if (type == "morning") {
    jsonResponse = ledController->getMorningProfileJson();
  } else if (type == "midday") {
    jsonResponse = ledController->getMiddayProfileJson();
  } else if (type == "evening") {
    jsonResponse = ledController->getEveningProfileJson();
  } else if (type == "night") {
    jsonResponse = ledController->getNightProfileJson();
  } else {
    jsonResponse = ledController->getCurrentProfileJson();
  }
  
  request->send(200, "application/json", jsonResponse);
}

void WiFiService::handleSetProfile(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
  String jsonString = String((char*)data);
  
  // Parse JSON
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, jsonString);
  
  // Check for parsing errors
  if (error) {
    request->send(400, "text/plain", String("JSON parsing failed: ") + error.c_str());
    return;
  }
  
  String type = "";
  String profile = "";
  
  // Ekstrak nilai dengan pengecekan keberadaan key
  if (doc.containsKey("type")) {
    type = doc["type"].as<String>();
  }
  
  if (doc.containsKey("profile")) {
    // Serialize the profile object to string
    JsonObject profileObj = doc["profile"].as<JsonObject>();
    StaticJsonDocument<256> profileDoc;
    JsonObject profileRoot = profileDoc.to<JsonObject>();
    
    for (JsonPair kv : profileObj) {
      profileRoot[kv.key().c_str()] = kv.value();
    }
    
    serializeJson(profileDoc, profile);
  }
  
  bool success = true;
  
  if (type == "morning") {
    ledController->setMorningProfile(profile);
    Serial.println("Morning profile updated");
  } else if (type == "midday") {
    ledController->setMiddayProfile(profile);
    Serial.println("Midday profile updated");
  } else if (type == "evening") {
    ledController->setEveningProfile(profile);
    Serial.println("Evening profile updated");
  } else if (type == "night") {
    ledController->setNightProfile(profile);
    Serial.println("Night profile updated");
  } else if (type == "current") {
    ledController->setLightProfileFromJson(profile);
    Serial.println("Current profile updated");
  } else {
    success = false;
  }
  
  if (success) {
    request->send(200, "application/json", "{\"status\":\"success\"}");
  } else {
    request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid profile type\"}");
  }
}

void WiFiService::handleGetTimeRanges(AsyncWebServerRequest* request) {
  String jsonResponse = ledController->getTimeRangesJson();
  request->send(200, "application/json", jsonResponse);
}

void WiFiService::handleSetTimeRanges(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
  String jsonString = String((char*)data);
  
  // Parse JSON untuk memastikan valid
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) {
    request->send(400, "text/plain", String("JSON parsing failed: ") + error.c_str());
    return;
  }
  
  ledController->setTimeRangesFromJson(jsonString);
  Serial.println("Time ranges updated");
  request->send(200, "application/json", "{\"status\":\"success\"}");
}

void WiFiService::handleManualControl(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
  String jsonString = String((char*)data);
  
  // Parse JSON
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, jsonString);
  
  // Check for parsing errors
  if (error) {
    request->send(400, "text/plain", String("JSON parsing failed: ") + error.c_str());
    return;
  }
  
  String led = "";
  int intensity = 0;
  
  // Ekstrak nilai dengan pengecekan keberadaan key
  if (doc.containsKey("led")) {
    led = doc["led"].as<String>();
  }
  
  if (doc.containsKey("value")) {
    intensity = doc["value"].as<int>();
  }
  
  // Batasi nilai intensitas
  if (intensity < 0) intensity = 0;
  if (intensity > 255) intensity = 255;
  
  // Pastikan dalam mode manual
  if (!ledController->isInManualMode()) {
    ledController->enableManualMode(true);
  }
  
  // Mengatur intensitas LED
  if (led == "royalBlue") {
    ledController->setRoyalBlue(intensity);
  } else if (led == "blue") {
    ledController->setBlue(intensity);
  } else if (led == "uv") {
    ledController->setUV(intensity);
  } else if (led == "violet") {
    ledController->setViolet(intensity);
  } else if (led == "red") {
    ledController->setRed(intensity);
  } else if (led == "green") {
    ledController->setGreen(intensity);
  } else if (led == "white") {
    ledController->setWhite(intensity);
  } else {
    request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid LED type\"}");
    return;
  }
  
  request->send(200, "application/json", "{\"status\":\"success\"}");
}

void WiFiService::handleGetCurrentTime(AsyncWebServerRequest* request) {
  String jsonResponse = ledController->getCurrentTimeJson();
  request->send(200, "application/json", jsonResponse);
}

void WiFiService::handleSetTime(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
  String jsonString = String((char*)data);
  Serial.print("Received time JSON: ");
  Serial.println(jsonString);
  
  // Langsung kirim JSON string dari aplikasi ke LedController
  if (ledController->setCurrentTime(jsonString)) {
    Serial.println("Current time updated successfully");
    request->send(200, "application/json", "{\"status\":\"success\"}");
  } else {
    Serial.println("Failed to update time, invalid format");
    request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid time format\"}");
  }
}

void WiFiService::handleSetMode(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
  String jsonString = String((char*)data);
  
  // Parse JSON
  StaticJsonDocument<100> doc;
  DeserializationError error = deserializeJson(doc, jsonString);
  
  // Check for parsing errors
  if (error) {
    request->send(400, "text/plain", String("JSON parsing failed: ") + error.c_str());
    return;
  }
  
  String mode = "";
  
  // Extract mode value
  if (doc.containsKey("mode")) {
    mode = doc["mode"].as<String>();
  }
  
  if (mode == "manual") {
    ledController->enableManualMode(true);
    Serial.println("Switched to manual mode");
    request->send(200, "application/json", "{\"status\":\"success\"}");
  } else if (mode == "auto") {
    ledController->enableManualMode(false);
    Serial.println("Switched to auto mode");
    request->send(200, "application/json", "{\"status\":\"success\"}");
  } else {
    request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid mode\"}");
  }
}

void WiFiService::handleGetMode(AsyncWebServerRequest* request) {
  String mode = ledController->isInManualMode() ? "manual" : "auto";
  String jsonResponse = "{\"mode\":\"" + mode + "\"}";
  request->send(200, "application/json", jsonResponse);
}

bool WiFiService::isConnected() {
  // Dalam mode AP, kita gunakan flag apActive dan periksa jika AP berjalan
  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    return apActive;
  }
  // Dalam mode STA, periksa status koneksi
  return WiFi.status() == WL_CONNECTED;
}

IPAddress WiFiService::getIP() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP();
  } else {
    return WiFi.softAPIP();
  }
}

void WiFiService::update() {
  static unsigned long lastWiFiCheck = 0;
  static unsigned long lastAPCheck = 0;
  static unsigned long lastAutoRestart = 0;
  static int failCount = 0;
  static unsigned long lastClientDisconnect = 0;
  
  // Quick WiFi health check every 10 seconds (lebih pelan dari 5 detik)
  if (millis() - lastWiFiCheck > 10000) {
    lastWiFiCheck = millis();
    
    // Check if WiFi mode is correct
    if (!apActive && (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA)) {
      apActive = true;
      Serial.println("WiFi AP is now active");
    } else if (apActive && WiFi.getMode() != WIFI_AP && WiFi.getMode() != WIFI_AP_STA) {
      apActive = false;
      Serial.println("WiFi AP is no longer active, will attempt to restart");
      lastConnectAttempt = millis() - 25000; // Will trigger reconnect soon
      failCount++;
    }
  }
  
  // More detailed AP status check every 60 seconds (lebih pelan dari 30 detik)
  if (millis() - lastAPCheck > 60000) {
    lastAPCheck = millis();
    
    // Check WiFi status thoroughly but gently
    if (!apActive && WiFi.getMode() == WIFI_OFF) {
      Serial.println("WiFi is OFF, restarting AP...");
      startAP();
    }
    
    // Check if any clients are connected to the AP
    if (WiFi.softAPgetStationNum() > 0) {
      if (!deviceConnected) {
        Serial.println("Client connected to AP");
        deviceConnected = true;
        failCount = 0; // Reset fail counter when we have a connection
      }
      // Jika ada client terhubung, reset timer disconnect
      lastClientDisconnect = 0;
    } else {
      if (deviceConnected) {
        // Jika tidak ada client dan sebelumnya terhubung,
        // mulai timer disconnect jika belum dimulai
        if (lastClientDisconnect == 0) {
          lastClientDisconnect = millis();
          Serial.println("Client disconnected, starting reconnection grace period");
        }
        // Hanya ubah status setelah 3 menit tidak ada koneksi (mencegah reconnect yang berlebihan)
        else if (millis() - lastClientDisconnect > 180000) {
          Serial.println("No clients connected to AP for 3 minutes");
          deviceConnected = false;
          lastClientDisconnect = 0;
        }
      }
    }
    
    // Print status info
    Serial.print("AP status: ");
    Serial.print(apActive ? "Active" : "Inactive");
    if (apActive) {
      Serial.print(", IP: ");
      Serial.print(WiFi.softAPIP().toString());
      Serial.print(", ");
      Serial.print(WiFi.softAPgetStationNum());
      Serial.print(" client(s) connected");
      if (lastClientDisconnect > 0) {
        Serial.print(", Reconnection grace: ");
        Serial.print((millis() - lastClientDisconnect) / 1000);
        Serial.print("s/180s");
      }
    }
    Serial.println();
    
    // If we've had several failed attempts, try a full WiFi restart
    // Hanya restart WiFi setelah 5 kegagalan berturut-turut
    if (reconnectAttempts >= 3 || failCount >= 5) {
      Serial.println("Multiple connection failures, performing full WiFi restart...");
      failCount = 0;
      reconnectAttempts = 0;
      restartWiFi();
    }
  }
  
  // Auto-restart ESP32 once per day for maximum stability
  // (86400000 ms = 24 hours)
  if (millis() - lastAutoRestart > 86400000) {
    lastAutoRestart = millis();
    Serial.println("Performing daily auto-restart for stability...");
    
    // Give time for serial message to be transmitted
    delay(1000);
    
    // Restart ESP32
    ESP.restart();
  }
  
  // Check if we need to attempt reconnection
  if (!apActive && (millis() - lastConnectAttempt > 30000)) {
    lastConnectAttempt = millis();
    Serial.println("Attempting to restart AP...");
    startAP();
  }
}

void WiFiService::handlePing(AsyncWebServerRequest* request) {
  // Buat JSON response yang berisi status server dan informasi
  StaticJsonDocument<256> doc;
  doc["status"] = "active";
  doc["clients"] = WiFi.softAPgetStationNum();
  doc["ip"] = WiFi.softAPIP().toString();
  doc["uptime"] = millis() / 1000; // dalam detik
  doc["ssid"] = ssid;
  
  // Melaporkan memori
  doc["free_heap"] = ESP.getFreeHeap();

  String jsonResponse;
  serializeJson(doc, jsonResponse);
  
  // Catat ping juga di serial untuk debugging
  Serial.println("Ping received from client");
  
  // Kirim respons
  request->send(200, "application/json", jsonResponse);
}

// Destructor untuk membersihkan sumber daya
WiFiService::~WiFiService() {
  // Close preferences
  preferences.end();
  
  // Clean up server
  if (server) {
    // Tidak ada metode end() untuk AsyncWebServer, 
    // tetapi kita bisa membebaskan memori
    delete server;
  }
}
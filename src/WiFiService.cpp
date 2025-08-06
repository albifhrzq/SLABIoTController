#include "WiFiService.h"
#include "esp_wifi.h"  // Untuk akses fungsi WiFi ESP-IDF level rendah

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
  
  // Mulai AP dengan beberapa percobaan
  bool apStartSuccess = false;
  for (int attempt = 0; attempt < 3; attempt++) {
    Serial.print("AP startup attempt ");
    Serial.print(attempt + 1);
    Serial.println("/3");
    
    // Start AP
    startAP();
    
    // Tunggu AP siap
    delay(1000);
    
    // Verifikasi AP aktif dan berjalan
    if (apActive && WiFi.getMode() == WIFI_AP) {
      apStartSuccess = true;
      break;
    }
    
    // Jika gagal, tunggu sedikit sebelum mencoba lagi
    Serial.println("AP startup attempt failed, retrying...");
    delay(2000);
  }
  
  if (!apStartSuccess) {
    Serial.println("WARNING: Failed to start AP after multiple attempts!");
    Serial.println("Continuing with setup, will retry AP startup later.");
  }
  
  // Setup endpoint API
  setupApiEndpoints();
  
  // Start server regardless of AP status
  server->begin();
  Serial.println("HTTP server started");
  deviceConnected = false; // Start with no connections
}

void WiFiService::startAP() {
  // Shutdown WiFi completely first
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(1000); // Tunggu lebih lama (1 detik) untuk memastikan WiFi benar-benar mati
  
  // Force restart ESP32 WiFi drivers jika flag gagal sudah tinggi
  if (reconnectAttempts >= 2) {
    Serial.println("Performing WiFi driver reset...");
    esp_wifi_stop();
    delay(1000);
    esp_wifi_deinit();
    delay(1000);
    esp_wifi_init(NULL);
    delay(1000);
    esp_wifi_start();
    delay(1000);
    reconnectAttempts = 0; // Reset counter setelah reset driver
  }
  
  // Configure WiFi in AP mode explicitly
  WiFi.mode(WIFI_OFF); // Pastikan mati dulu
  delay(500);
  WiFi.mode(WIFI_AP);
  delay(500); // Beri waktu untuk mengatur mode
  
  // Set low-level parameters
  WiFi.setTxPower(WIFI_POWER_19_5dBm); // Set TX power ke level maximum
  
  // Konfigurasi IP statis untuk AP 
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  bool configSuccess = WiFi.softAPConfig(local_IP, gateway, subnet);
  if (!configSuccess) {
    Serial.println("AP IP configuration failed! Using default.");
  } else {
    Serial.println("AP IP configuration success.");
  }
  
  // Setup AP dengan parameter yang lebih eksplisit
  // channel=1, ssid_hidden=0, max_connection=4, beacon_interval=100ms
  bool result = WiFi.softAP(ssid, password, 1, 0, 4);
  
  // Tunggu AP benar-benar siap
  delay(500);
  
  if (result) {
    apActive = true;
    IPAddress myIP = WiFi.softAPIP();
    Serial.print("Access Point started. IP address: ");
    Serial.println(myIP);
    
    // Verifikasi IP - restart jika bukan yang diharapkan
    if (myIP != local_IP && configSuccess) {
      Serial.println("Warning: IP address not as configured, will retry...");
      WiFi.disconnect(true);
      delay(500);
      // Akan dicoba ulang di update berikutnya
      apActive = false;
      return;
    }
    
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
  static unsigned long lastFullCheck = 0;
  
  // Check if AP is active but no clients connected for a long time
  if (apActive) {
    // Verifikasi mode WiFi benar
    if (WiFi.getMode() != WIFI_AP && WiFi.getMode() != WIFI_AP_STA) {
      Serial.println("AP mode unexpectedly disabled, restarting AP...");
      startAP();
      return;
    }
    
    // Verifikasi IP address benar
    IPAddress expectedIP(192, 168, 4, 1);
    if (WiFi.softAPIP() != expectedIP) {
      Serial.println("AP IP address unexpected, reconfiguring...");
      startAP();
      return;
    }
    
    // Full periodic check (setiap 5 menit)
    if (millis() - lastFullCheck > 300000) {
      lastFullCheck = millis();
      
      // Verifikasi SSID aktif - ini bisa jadi mahal, jadi hanya lakukan secara periodik
      wifi_ap_record_t apInfo;
      esp_wifi_sta_get_ap_info(&apInfo);
      if (strlen((char*)apInfo.ssid) == 0) {
        Serial.println("AP SSID not broadcasting properly, restarting WiFi...");
        startAP();
        return;
      }
      
      Serial.println("Full WiFi health check passed");
    }
  } else {
    // Jika AP seharusnya tidak aktif tapi ternyata mode-nya AP, ada masalah
    if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
      Serial.println("WiFi in unexpected AP mode while flag disabled, fixing...");
      apActive = true;
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
  
  // API untuk kontrol manual semua LED sekaligus
  server->on("/api/manual/all", HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    NULL,
    [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      this->handleManualControlAll(request, data, len);
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
  
  Serial.print("Received manual control request: ");
  Serial.println(jsonString);
  
  // Parse JSON
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, jsonString);
  
  // Check for parsing errors
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    request->send(400, "text/plain", String("JSON parsing failed: ") + error.c_str());
    return;
  }
  
  // Extract values with validation
  String led = "";
  int value = 0;
  
  // Get LED name
  if (doc.containsKey("led")) {
    led = doc["led"].as<String>();
  } else {
    Serial.println("Error: Missing 'led' property");
    request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing 'led' property\"}");
    return;
  }
  
  // Get value
  if (doc.containsKey("value")) {
    value = doc["value"].as<int>();
    
    // Check for valid value range
    if (value < 0 || value > 255) {
      Serial.println("Error: Value out of range (0-255)");
      request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Value out of range (0-255)\"}");
      return;
    }
  } else {
    Serial.println("Error: Missing 'value' property");
    request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing 'value' property\"}");
    return;
  }
  
  // Pastikan dalam mode manual
  if (!ledController->isInManualMode()) {
    ledController->enableManualMode(true);
  }
  
  // Mengatur intensitas LED
  if (led == "royalBlue") {
    ledController->setRoyalBlue(value);
  } else if (led == "blue") {
    ledController->setBlue(value);
  } else if (led == "uv") {
    ledController->setUV(value);
  } else if (led == "violet") {
    ledController->setViolet(value);
  } else if (led == "red") {
    ledController->setRed(value);
  } else if (led == "green") {
    ledController->setGreen(value);
  } else if (led == "white") {
    ledController->setWhite(value);
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
    ledController->setOffMode(false); // Disable off mode
    ledController->enableManualMode(true);
    Serial.println("Switched to manual mode");
    request->send(200, "application/json", "{\"status\":\"success\"}");
  } else if (mode == "auto") {
    ledController->setOffMode(false); // Disable off mode
    ledController->enableManualMode(false);
    Serial.println("Switched to auto mode");
    request->send(200, "application/json", "{\"status\":\"success\"}");
  } else if (mode == "off") {
    // Turn off all LEDs and set off mode
    ledController->setOffMode(true);
    Serial.println("Switched to off mode - all LEDs turned off");
    request->send(200, "application/json", "{\"status\":\"success\"}");
  } else {
    Serial.print("Invalid mode received: ");
    Serial.println(mode);
    request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid mode. Valid modes: manual, auto, off\"}");
  }
}

void WiFiService::handleGetMode(AsyncWebServerRequest* request) {
  String mode;
  if (ledController->isInOffMode()) {
    mode = "off";
  } else if (ledController->isInManualMode()) {
    mode = "manual";
  } else {
    mode = "auto";
  }
  String jsonResponse = "{\"mode\":\"" + mode + "\"}";
  request->send(200, "application/json", jsonResponse);
}

bool WiFiService::isConnected() {
  // Dalam mode AP, kita cek kondisi AP lebih komprehensif
  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    // Jika flag internal sudah menunjukkan AP tidak aktif, kembalikan false
    if (!apActive) return false;
    
    // Check if AP is running on expected IP
    IPAddress expectedIP(192, 168, 4, 1);
    IPAddress currentIP = WiFi.softAPIP();
    
    // Check for valid IP (not 0.0.0.0)
    bool validIP = (currentIP[0] != 0 || currentIP[1] != 0 || 
                   currentIP[2] != 0 || currentIP[3] != 0);
                   
    return validIP;
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
  static unsigned long startupTime = millis(); // Catat waktu startup
  
  // Quick WiFi health check - lebih sering pada 5 menit pertama
  unsigned long checkInterval = (millis() - startupTime < 300000) ? 5000 : 10000;
  if (millis() - lastWiFiCheck > checkInterval) {
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
    
    // Lebih agresif memulai AP pada 2 menit pertama jika belum active
    if (!apActive && (millis() - startupTime < 120000)) {
      Serial.println("WiFi not active during early startup, restarting AP...");
      startAP();
    }
  }
  
  // More detailed AP status check every 60 seconds (lebih pelan dari 30 detik)
  // Tetapi lebih sering pada 3 menit pertama
  unsigned long statusCheckInterval = (millis() - startupTime < 180000) ? 30000 : 60000;
  if (millis() - lastAPCheck > statusCheckInterval) {
    lastAPCheck = millis();
    
    // Check WiFi status thoroughly
    checkWiFiStatus();
    
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
  // Lebih agresif pada startup
  unsigned long reconnectInterval = (millis() - startupTime < 300000) ? 10000 : 30000;
  if (!apActive && (millis() - lastConnectAttempt > reconnectInterval)) {
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

// Implementasi fungsi untuk mengontrol semua LED sekaligus
void WiFiService::handleManualControlAll(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
  String jsonString = String((char*)data);
  
  Serial.print("Received manual control ALL request: ");
  Serial.println(jsonString);
  
  // Validasi JSON sebelum memproses
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, jsonString);
  
  // Check for parsing errors
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    String errorMsg = "{\"status\":\"error\",\"message\":\"JSON parsing failed: ";
    errorMsg += error.c_str();
    errorMsg += "\"}";
    request->send(400, "application/json", errorMsg);
    return;
  }
  
  // Periksa apakah semua properti diperlukan ada
  bool isValid = true;
  String errorMessage = "Missing properties: ";
  
  if (!doc.containsKey("royalBlue")) {
    errorMessage += "royalBlue, ";
    isValid = false;
  }
  if (!doc.containsKey("blue")) {
    errorMessage += "blue, ";
    isValid = false;
  }
  if (!doc.containsKey("uv")) {
    errorMessage += "uv, ";
    isValid = false;
  }
  if (!doc.containsKey("violet")) {
    errorMessage += "violet, ";
    isValid = false;
  }
  if (!doc.containsKey("red")) {
    errorMessage += "red, ";
    isValid = false;
  }
  if (!doc.containsKey("green")) {
    errorMessage += "green, ";
    isValid = false;
  }
  if (!doc.containsKey("white")) {
    errorMessage += "white, ";
    isValid = false;
  }
  
  // Jika ada properti yang hilang, kirim error
  if (!isValid) {
    Serial.println(errorMessage);
    String jsonError = "{\"status\":\"error\",\"message\":\"" + errorMessage + "\"}";
    request->send(400, "application/json", jsonError);
    return;
  }
  
  // Jika aplikasi mengirimkan data dengan format yang berbeda (misalnya dengan wrapper),
  // coba periksa untuk wrapper umum
  if (doc.containsKey("intensities") || doc.containsKey("values") || doc.containsKey("data") || doc.containsKey("leds")) {
    Serial.println("Detected wrapper object, trying to extract LED values...");
    JsonObject wrapper;
    
    if (doc.containsKey("intensities")) wrapper = doc["intensities"].as<JsonObject>();
    else if (doc.containsKey("values")) wrapper = doc["values"].as<JsonObject>();
    else if (doc.containsKey("data")) wrapper = doc["data"].as<JsonObject>();
    else if (doc.containsKey("leds")) wrapper = doc["leds"].as<JsonObject>();
    
    // Re-create the correct JSON format expected by the controller
    StaticJsonDocument<512> formattedDoc;
    if (wrapper.containsKey("royalBlue")) formattedDoc["royalBlue"] = wrapper["royalBlue"];
    if (wrapper.containsKey("blue")) formattedDoc["blue"] = wrapper["blue"];
    if (wrapper.containsKey("uv")) formattedDoc["uv"] = wrapper["uv"];
    if (wrapper.containsKey("violet")) formattedDoc["violet"] = wrapper["violet"];
    if (wrapper.containsKey("red")) formattedDoc["red"] = wrapper["red"];
    if (wrapper.containsKey("green")) formattedDoc["green"] = wrapper["green"];
    if (wrapper.containsKey("white")) formattedDoc["white"] = wrapper["white"];
    
    // Serialize to string
    String formattedJson;
    serializeJson(formattedDoc, formattedJson);
    
    Serial.print("Reformatted JSON: ");
    Serial.println(formattedJson);
    
    // Use the reformatted JSON
    jsonString = formattedJson;
  }
  
  // Pastikan mode manual aktif
  if (!ledController->isInManualMode()) {
    ledController->enableManualMode(true);
    Serial.println("Switching to manual mode for controlling all LEDs");
  }
  
  // Terapkan pengaturan LED dari JSON
  ledController->setAllLedsFromJson(jsonString);
  
  // Kirim respons sukses
  request->send(200, "application/json", "{\"status\":\"success\"}");
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
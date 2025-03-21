#include "WiFiService.h"

WiFiService::WiFiService(LedController* ledController, const char* ssid, const char* password) {
  this->ledController = ledController;
  this->ssid = ssid;
  this->password = password;
  this->deviceConnected = false;
  this->server = new AsyncWebServer(80);
}

void WiFiService::begin() {
  // Hubungkan ke WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  
  // Tunggu koneksi
  uint8_t timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    Serial.print(".");
    timeout++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    deviceConnected = true;
    Serial.println("");
    Serial.print("Connected to WiFi with IP: ");
    Serial.println(WiFi.localIP());
    
    // Setup endpoint API
    setupApiEndpoints();
    
    // Mulai server
    server->begin();
    Serial.println("HTTP server started");
  } else {
    Serial.println("");
    Serial.println("Failed to connect to WiFi. Starting access point mode...");
    
    // Jika gagal terhubung, buat access point
    WiFi.softAP("Aquarium-LED-Controller", "12345678");
    Serial.print("Access Point IP address: ");
    Serial.println(WiFi.softAPIP());
    
    // Setup endpoint API
    setupApiEndpoints();
    
    // Mulai server
    server->begin();
    Serial.println("HTTP server started in AP mode");
  }
}

void WiFiService::setupApiEndpoints() {
  // CORS handler
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
  
  // Handle OPTIONS method untuk CORS
  server->onNotFound([this](AsyncWebServerRequest *request) {
    this->handleNotFound(request);
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
}

void WiFiService::handleCors(AsyncWebServerRequest* request) {
  if (request->method() == HTTP_OPTIONS) {
    request->send(204);
  }
}

void WiFiService::handleNotFound(AsyncWebServerRequest* request) {
  if (request->method() == HTTP_OPTIONS) {
    request->send(204);
  } else {
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
  // Tidak ada yang perlu dilakukan di sini karena server berjalan secara asinkron
  // Metode ini disediakan untuk kompatibilitas dengan class BluetoothService
}
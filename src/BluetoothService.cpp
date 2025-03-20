#include "BluetoothService.h"
#include <ArduinoJson.h>

BluetoothService::BluetoothService(LedController* ledController) {
  this->ledController = ledController;
  this->deviceConnected = false;
}

void BluetoothService::begin() {
  // Inisialisasi NimBLE
  NimBLEDevice::init("Aquarium LED Controller");
  
  // Menetapkan daya transmisi (opsional, default adalah 3 dBm)
  NimBLEDevice::setPower(ESP_PWR_LVL_P9); // +9dBm
  
  // Membuat server BLE
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(this);
  
  // Membuat service
  pService = pServer->createService(SERVICE_UUID);
  
  // Membuat karakteristik untuk profil lampu
  pProfileChar = pService->createCharacteristic(
                     PROFILE_CHAR_UUID,
                     NIMBLE_PROPERTY::READ | 
                     NIMBLE_PROPERTY::WRITE | 
                     NIMBLE_PROPERTY::NOTIFY
                   );
  pProfileChar->setCallbacks(this);
  
  // Karakteristik untuk rentang waktu
  pTimeRangesChar = pService->createCharacteristic(
                     TIME_RANGES_CHAR_UUID,
                     NIMBLE_PROPERTY::READ | 
                     NIMBLE_PROPERTY::WRITE | 
                     NIMBLE_PROPERTY::NOTIFY
                   );
  pTimeRangesChar->setCallbacks(this);
  
  // Karakteristik untuk kontrol manual
  pManualControlChar = pService->createCharacteristic(
                     MANUAL_CONTROL_CHAR_UUID,
                     NIMBLE_PROPERTY::WRITE
                   );
  pManualControlChar->setCallbacks(this);
  
  // Karakteristik untuk waktu saat ini
  pCurrentTimeChar = pService->createCharacteristic(
                     CURRENT_TIME_CHAR_UUID,
                     NIMBLE_PROPERTY::READ | 
                     NIMBLE_PROPERTY::NOTIFY
                   );
  pCurrentTimeChar->setCallbacks(this);
  
  // Karakteristik untuk mode operasi
  pModeChar = pService->createCharacteristic(
                     MODE_CHAR_UUID,
                     NIMBLE_PROPERTY::READ | 
                     NIMBLE_PROPERTY::WRITE | 
                     NIMBLE_PROPERTY::NOTIFY
                   );
  pModeChar->setCallbacks(this);
  
  // Memulai service
  pService->start();
  
  // Mulai mengiklankan service
  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // fungsi untuk iOS
  pAdvertising->setMinPreferred(0x12);
  pAdvertising->start();
  
  Serial.println("Bluetooth service started. Device ready for connection...");
}

void BluetoothService::updateCharacteristics() {
  if (deviceConnected) {
    // Update karakteristik dengan nilai terbaru
    pProfileChar->setValue(ledController->getCurrentProfileJson().c_str());
    pProfileChar->notify();
    
    pTimeRangesChar->setValue(ledController->getTimeRangesJson().c_str());
    pTimeRangesChar->notify();
    
    pCurrentTimeChar->setValue(ledController->getCurrentTimeJson().c_str());
    pCurrentTimeChar->notify();
    
    // Set mode value: "manual" atau "auto"
    std::string modeStr = ledController->isInManualMode() ? "manual" : "auto";
    pModeChar->setValue(modeStr);
    pModeChar->notify();
    
    delay(100); // Berikan sedikit waktu antara notifikasi
  }
}

void BluetoothService::onConnect(NimBLEServer* pServer) {
  deviceConnected = true;
  Serial.println("Device connected");
}

void BluetoothService::onDisconnect(NimBLEServer* pServer) {
  deviceConnected = false;
  Serial.println("Device disconnected");
  
  // Mulai kembali iklan setelah pemutusan
  NimBLEDevice::getAdvertising()->start();
}

void BluetoothService::onWrite(NimBLECharacteristic* pCharacteristic) {
  std::string value = pCharacteristic->getValue();
  
  if (value.length() > 0) {
    Serial.print("Received value: ");
    Serial.println(value.c_str());
    
    // Handle berdasarkan karakteristik
    if (pCharacteristic->getUUID().equals(NimBLEUUID(PROFILE_CHAR_UUID))) {
      processProfileCommand(value);
    } 
    else if (pCharacteristic->getUUID().equals(NimBLEUUID(MANUAL_CONTROL_CHAR_UUID))) {
      processManualCommand(value);
    } 
    else if (pCharacteristic->getUUID().equals(NimBLEUUID(TIME_RANGES_CHAR_UUID))) {
      processTimeRangesCommand(value);
    } 
    else if (pCharacteristic->getUUID().equals(NimBLEUUID(MODE_CHAR_UUID))) {
      processModeCommand(value);
    }
  }
}

void BluetoothService::onRead(NimBLECharacteristic* pCharacteristic) {
  // Update nilai karakteristik saat dibaca
  if (pCharacteristic->getUUID().equals(NimBLEUUID(PROFILE_CHAR_UUID))) {
    pCharacteristic->setValue(ledController->getCurrentProfileJson().c_str());
  } 
  else if (pCharacteristic->getUUID().equals(NimBLEUUID(TIME_RANGES_CHAR_UUID))) {
    pCharacteristic->setValue(ledController->getTimeRangesJson().c_str());
  } 
  else if (pCharacteristic->getUUID().equals(NimBLEUUID(CURRENT_TIME_CHAR_UUID))) {
    pCharacteristic->setValue(ledController->getCurrentTimeJson().c_str());
  } 
  else if (pCharacteristic->getUUID().equals(NimBLEUUID(MODE_CHAR_UUID))) {
    std::string modeStr = ledController->isInManualMode() ? "manual" : "auto";
    pCharacteristic->setValue(modeStr);
  }
}

void BluetoothService::processProfileCommand(const std::string& value) {
  // Format: {"type":"morning|midday|evening|night", "profile":"{...}"}
  String jsonString = String(value.c_str());
  
  // Parse JSON
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, jsonString);
  
  // Check for parsing errors
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
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
  
  if (type == "morning") {
    ledController->setMorningProfile(profile);
    Serial.println("Morning profile updated");
  } 
  else if (type == "midday") {
    ledController->setMiddayProfile(profile);
    Serial.println("Midday profile updated");
  } 
  else if (type == "evening") {
    ledController->setEveningProfile(profile);
    Serial.println("Evening profile updated");
  } 
  else if (type == "night") {
    ledController->setNightProfile(profile);
    Serial.println("Night profile updated");
  } 
  else if (type == "current") {
    ledController->setLightProfileFromJson(profile);
    Serial.println("Current profile updated");
  }
}

void BluetoothService::processManualCommand(const std::string& value) {
  // Format: {"led":"royalBlue|blue|uv|violet|red|green|white", "value":0-255}
  String jsonString = String(value.c_str());
  
  // Parse JSON
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, jsonString);
  
  // Check for parsing errors
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
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
    // Kirim notifikasi perubahan mode
    std::string modeStr = "manual";
    pModeChar->setValue(modeStr);
    pModeChar->notify();
  }
  
  // Set intensitas LED
  if (led == "royalBlue") {
    ledController->setRoyalBlue(intensity);
  } 
  else if (led == "blue") {
    ledController->setBlue(intensity);
  } 
  else if (led == "uv") {
    ledController->setUV(intensity);
  } 
  else if (led == "violet") {
    ledController->setViolet(intensity);
  } 
  else if (led == "red") {
    ledController->setRed(intensity);
  } 
  else if (led == "green") {
    ledController->setGreen(intensity);
  } 
  else if (led == "white") {
    ledController->setWhite(intensity);
  }
  
  Serial.print(led.c_str());
  Serial.print(" set to ");
  Serial.println(intensity);
}

void BluetoothService::processTimeRangesCommand(const std::string& value) {
  String jsonString = String(value.c_str());
  ledController->setTimeRangesFromJson(jsonString);
  Serial.println("Time ranges updated");
}

void BluetoothService::processModeCommand(const std::string& value) {
  if (value == "manual") {
    ledController->enableManualMode(true);
    Serial.println("Mode changed to manual");
  } 
  else if (value == "auto") {
    ledController->enableManualMode(false);
    Serial.println("Mode changed to auto");
  }
}

bool BluetoothService::isConnected() {
  return deviceConnected;
} 
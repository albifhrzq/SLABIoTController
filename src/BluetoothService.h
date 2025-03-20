#ifndef BLUETOOTH_SERVICE_H
#define BLUETOOTH_SERVICE_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include <ArduinoJson.h>
#include "LedController.h"

// Service dan Characteristic UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define PROFILE_CHAR_UUID   "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define TIME_RANGES_CHAR_UUID "43f128bd-b9d2-4a83-b9e6-355b7b55a82f"
#define MANUAL_CONTROL_CHAR_UUID "218b3c65-22e0-4ed8-9df5-77c4e79a60d3"
#define CURRENT_TIME_CHAR_UUID "1c95d5e3-d8f7-413a-bf3d-9002ecc1c188"
#define MODE_CHAR_UUID     "49c3dc32-1b9c-4dab-9ff5-bd11a6f6086d"

class BluetoothService : public NimBLEServerCallbacks, public NimBLECharacteristicCallbacks {
private:
  LedController* ledController;
  NimBLEServer* pServer;
  NimBLEService* pService;
  NimBLECharacteristic* pProfileChar;
  NimBLECharacteristic* pTimeRangesChar;
  NimBLECharacteristic* pManualControlChar;
  NimBLECharacteristic* pCurrentTimeChar;
  NimBLECharacteristic* pModeChar;
  
  bool deviceConnected;
  std::string lastCommand;
  
  // Metode privat
  void processProfileCommand(const std::string& value);
  void processManualCommand(const std::string& value);
  void processTimeRangesCommand(const std::string& value);
  void processModeCommand(const std::string& value);
  
public:
  // Constructor
  BluetoothService(LedController* ledController);
  
  // Inisialisasi layanan BLE
  void begin();
  
  // Update nilai karakteristik
  void updateCharacteristics();
  
  // Callback saat perangkat terhubung/terputus
  void onConnect(NimBLEServer* pServer) override;
  void onDisconnect(NimBLEServer* pServer) override;
  
  // Callback saat karakteristik diakses
  void onWrite(NimBLECharacteristic* pCharacteristic) override;
  void onRead(NimBLECharacteristic* pCharacteristic) override;
  
  // Cek koneksi
  bool isConnected();
};

#endif // BLUETOOTH_SERVICE_H 
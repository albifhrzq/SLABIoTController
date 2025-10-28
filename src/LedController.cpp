#include "LedController.h"

LedController::LedController(
  uint8_t pinRoyalBlue, uint8_t pinBlue, uint8_t pinUV, uint8_t pinViolet,
  uint8_t pinRed, uint8_t pinGreen, uint8_t pinWhite,
  uint8_t channelRoyalBlue, uint8_t channelBlue, uint8_t channelUV, uint8_t channelViolet,
  uint8_t channelRed, uint8_t channelGreen, uint8_t channelWhite,
  uint32_t freq, uint8_t resolution,
  RTC_DS3231* rtc
) {
  // Store pin configurations
  this->pinRoyalBlue = pinRoyalBlue;
  this->pinBlue = pinBlue;
  this->pinUV = pinUV;
  this->pinViolet = pinViolet;
  this->pinRed = pinRed;
  this->pinGreen = pinGreen;
  this->pinWhite = pinWhite;
  
  // Store channel configurations
  this->channelRoyalBlue = channelRoyalBlue;
  this->channelBlue = channelBlue;
  this->channelUV = channelUV;
  this->channelViolet = channelViolet;
  this->channelRed = channelRed;
  this->channelGreen = channelGreen;
  this->channelWhite = channelWhite;
  
  // Store PWM properties
  this->freq = freq;
  this->resolution = resolution;
  
  // Store RTC reference
  this->rtc = rtc;
  
  // Default to auto mode
  this->manualMode = false;
  this->offMode = false;
  
  // Initialize hourly schedule with all zeros (user must configure)
  for (int i = 0; i < 24; i++) {
    hourlySchedule[i].hour = i;
    hourlySchedule[i].profile = {0, 0, 0, 0, 0, 0, 0};
  }
}

void LedController::begin() {
  // Configure LED pins for PWM
  ledcSetup(channelRoyalBlue, freq, resolution);
  ledcAttachPin(pinRoyalBlue, channelRoyalBlue);
  
  ledcSetup(channelBlue, freq, resolution);
  ledcAttachPin(pinBlue, channelBlue);
  
  ledcSetup(channelUV, freq, resolution);
  ledcAttachPin(pinUV, channelUV);
  
  ledcSetup(channelViolet, freq, resolution);
  ledcAttachPin(pinViolet, channelViolet);
  
  ledcSetup(channelRed, freq, resolution);
  ledcAttachPin(pinRed, channelRed);
  
  ledcSetup(channelGreen, freq, resolution);
  ledcAttachPin(pinGreen, channelGreen);
  
  ledcSetup(channelWhite, freq, resolution);
  ledcAttachPin(pinWhite, channelWhite);
  
  // Initialize preferences
  preferences.begin("led_ctrl", false);
  
  // Load saved profiles and settings from preferences
  loadPreferences();
  
  // Load hourly schedule from preferences
  loadHourlyScheduleFromPreferences();
  
  // Jika dalam auto mode (bukan manual dan bukan off), terapkan jadwal segera
  if (!manualMode && !offMode) {
    Serial.println("Auto mode active on startup - applying scheduled profile");
    LightProfile currentProfile = getCurrentProfile();
    setLightProfile(currentProfile);
  }
  
  Serial.println("LED Controller initialized");
  Serial.print("Initial mode: ");
  if (offMode) {
    Serial.println("OFF");
  } else if (manualMode) {
    Serial.println("MANUAL");
  } else {
    Serial.println("AUTO");
  }
}

// Save mode to preferences
void LedController::saveModeToPreferences() {
  preferences.putBool("manual_mode", manualMode);
  preferences.putBool("off_mode", offMode);
  
  Serial.print("Mode saved to preferences - Manual: ");
  Serial.print(manualMode ? "YES" : "NO");
  Serial.print(", Off: ");
  Serial.println(offMode ? "YES" : "NO");
}

// Ensure all manual channels are initialized in NVS (called before saving individual channel)
void LedController::ensureAllManualChannelsSaved() {
  // NVS Key name limit: 15 characters max!
  // Using shortened keys: m_rb, m_b, m_uv, m_v, m_r, m_g, m_w
  
  // Hanya inisialisasi jika belum ada data sama sekali
  if (!preferences.isKey("m_rb")) {
    Serial.println(">>> First time setting manual LED - initializing all channels in NVS...");
    
    // Baca nilai current dari hardware (PWM) untuk menjaga state yang sedang menyala
    // Ini memastikan bahwa LED yang sudah menyala tidak tiba-tiba berubah
    uint8_t currentRoyalBlue = ledcRead(channelRoyalBlue);
    uint8_t currentBlue = ledcRead(channelBlue);
    uint8_t currentUV = ledcRead(channelUV);
    uint8_t currentViolet = ledcRead(channelViolet);
    uint8_t currentRed = ledcRead(channelRed);
    uint8_t currentGreen = ledcRead(channelGreen);
    uint8_t currentWhite = ledcRead(channelWhite);
    
    Serial.print(">>> Current PWM values: ");
    Serial.print("RB="); Serial.print(currentRoyalBlue);
    Serial.print(" B="); Serial.print(currentBlue);
    Serial.print(" UV="); Serial.print(currentUV);
    Serial.print(" V="); Serial.print(currentViolet);
    Serial.print(" R="); Serial.print(currentRed);
    Serial.print(" G="); Serial.print(currentGreen);
    Serial.print(" W="); Serial.println(currentWhite);
    
    // Simpan semua channel dengan nilai current (menggunakan key pendek)
    preferences.putUChar("m_rb", currentRoyalBlue);  // manual_royalBlue
    preferences.putUChar("m_b", currentBlue);        // manual_blue
    preferences.putUChar("m_uv", currentUV);         // manual_uv
    preferences.putUChar("m_v", currentViolet);      // manual_violet
    preferences.putUChar("m_r", currentRed);         // manual_red
    preferences.putUChar("m_g", currentGreen);       // manual_green
    preferences.putUChar("m_w", currentWhite);       // manual_white
    
    Serial.println(">>> All manual channels initialized and saved to NVS successfully!");
  }
}

// Load all preferences
void LedController::loadPreferences() {
  // Load mode if saved
  if (preferences.isKey("manual_mode")) {
    manualMode = preferences.getBool("manual_mode", false);
    Serial.println("Loaded mode from preferences: " + String(manualMode ? "manual" : "auto"));
  }
  
  // Load off mode if saved (independent dari manual_mode)
  if (preferences.isKey("off_mode")) {
    offMode = preferences.getBool("off_mode", false);
    Serial.println("Loaded off mode from preferences: " + String(offMode ? "OFF" : "ON"));
  }
  
  // Terapkan state berdasarkan mode yang dimuat
  if (offMode) {
    // Jika off mode, matikan semua LED
    ledcWrite(channelRoyalBlue, 0);
    ledcWrite(channelBlue, 0);
    ledcWrite(channelUV, 0);
    ledcWrite(channelViolet, 0);
    ledcWrite(channelRed, 0);
    ledcWrite(channelGreen, 0);
    ledcWrite(channelWhite, 0);
    Serial.println("Applied OFF mode - all LEDs turned off");
  } else if (manualMode) {
    // Jika dalam mode manual, muat pengaturan LED manual yang terakhir
    // Note: Menggunakan key pendek m_rb (manual_royalBlue) sebagai marker
    // Jika ada, semua 7 channel pasti tersimpan karena disimpan bersamaan
    if (preferences.isKey("m_rb")) {
      uint8_t royalBlue = preferences.getUChar("m_rb", 0);   // m_rb = manual_royalBlue
      uint8_t blue = preferences.getUChar("m_b", 0);         // m_b = manual_blue
      uint8_t uv = preferences.getUChar("m_uv", 0);          // m_uv = manual_uv
      uint8_t violet = preferences.getUChar("m_v", 0);       // m_v = manual_violet
      uint8_t red = preferences.getUChar("m_r", 0);          // m_r = manual_red
      uint8_t green = preferences.getUChar("m_g", 0);        // m_g = manual_green
      uint8_t white = preferences.getUChar("m_w", 0);        // m_w = manual_white
      
      // Terapkan pengaturan LED terakhir dari aplikasi
      ledcWrite(channelRoyalBlue, royalBlue);
      ledcWrite(channelBlue, blue);
      ledcWrite(channelUV, uv);
      ledcWrite(channelViolet, violet);
      ledcWrite(channelRed, red);
      ledcWrite(channelGreen, green);
      ledcWrite(channelWhite, white);
      
      Serial.println("Applied manual LED settings from preferences (last settings from app)");
      Serial.print("  RB="); Serial.print(royalBlue);
      Serial.print(" B="); Serial.print(blue);
      Serial.print(" UV="); Serial.print(uv);
      Serial.print(" V="); Serial.print(violet);
      Serial.print(" R="); Serial.print(red);
      Serial.print(" G="); Serial.print(green);
      Serial.print(" W="); Serial.println(white);
    } else {
      Serial.println("Manual mode but no saved LED values found (first time manual mode)");
      // LED akan tetap mati sampai user mengatur via aplikasi
    }
  } else {
    // Jika auto mode, terapkan profil jadwal saat ini
    Serial.println("Auto mode detected - will apply scheduled profile");
    // Note: Profil akan diterapkan di begin() atau di loop() pertama kali
  }
}

// Save all preferences at once
void LedController::saveAllPreferences() {
  saveHourlyScheduleToPreferences();
  saveModeToPreferences();
}

String LedController::profileToJson(LightProfile profile) {
  // Alokasi memori untuk JSON document
  StaticJsonDocument<200> doc;
  
  // Isi dengan data profile
  doc["royalBlue"] = profile.royalBlue;
  doc["blue"] = profile.blue;
  doc["uv"] = profile.uv;
  doc["violet"] = profile.violet;
  doc["red"] = profile.red;
  doc["green"] = profile.green;
  doc["white"] = profile.white;
  
  // Serialize JSON ke string
  String output;
  serializeJson(doc, output);
  return output;
}

LightProfile LedController::parseProfileJson(String jsonProfile) {
  LightProfile profile = {0, 0, 0, 0, 0, 0, 0}; // Default values
  
  // Parse JSON
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, jsonProfile);
  
  // Check for parsing errors
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return profile;
  }
  
  // Ekstrak nilai dengan pengecekan keberadaan key
  profile.royalBlue = doc.containsKey("royalBlue") ? (uint8_t)doc["royalBlue"] : 0;
  profile.blue = doc.containsKey("blue") ? (uint8_t)doc["blue"] : 0;
  profile.uv = doc.containsKey("uv") ? (uint8_t)doc["uv"] : 0;
  profile.violet = doc.containsKey("violet") ? (uint8_t)doc["violet"] : 0;
  profile.red = doc.containsKey("red") ? (uint8_t)doc["red"] : 0;
  profile.green = doc.containsKey("green") ? (uint8_t)doc["green"] : 0;
  profile.white = doc.containsKey("white") ? (uint8_t)doc["white"] : 0;
  
  return profile;
}

void LedController::enableManualMode(bool enable) {
  Serial.print("enableManualMode called: ");
  Serial.print(enable ? "MANUAL" : "AUTO");
  Serial.print(" (current mode: ");
  Serial.print(manualMode ? "MANUAL" : "AUTO");
  Serial.print(", offMode: ");
  Serial.print(offMode ? "ON" : "OFF");
  Serial.println(")");
  
  // Jika mengubah dari auto ke manual, simpan pengaturan LED saat ini
  if (!manualMode && enable) {
    // Jika tidak ada pengaturan manual sebelumnya, gunakan pengaturan profil saat ini
    if (!preferences.isKey("m_rb")) {
      LightProfile currentProfile = getCurrentProfile();
      
      preferences.putUChar("m_rb", currentProfile.royalBlue);
      preferences.putUChar("m_b", currentProfile.blue);
      preferences.putUChar("m_uv", currentProfile.uv);
      preferences.putUChar("m_v", currentProfile.violet);
      preferences.putUChar("m_r", currentProfile.red);
      preferences.putUChar("m_g", currentProfile.green);
      preferences.putUChar("m_w", currentProfile.white);
      
      // Terapkan pengaturan LED saat ini untuk mode manual
      ledcWrite(channelRoyalBlue, currentProfile.royalBlue);
      ledcWrite(channelBlue, currentProfile.blue);
      ledcWrite(channelUV, currentProfile.uv);
      ledcWrite(channelViolet, currentProfile.violet);
      ledcWrite(channelRed, currentProfile.red);
      ledcWrite(channelGreen, currentProfile.green);
      ledcWrite(channelWhite, currentProfile.white);
    } else {
      // Muat pengaturan manual sebelumnya
      uint8_t royalBlue = preferences.getUChar("m_rb", 0);
      uint8_t blue = preferences.getUChar("m_b", 0);
      uint8_t uv = preferences.getUChar("m_uv", 0);
      uint8_t violet = preferences.getUChar("m_v", 0);
      uint8_t red = preferences.getUChar("m_r", 0);
      uint8_t green = preferences.getUChar("m_g", 0);
      uint8_t white = preferences.getUChar("m_w", 0);
      
      // Terapkan pengaturan LED
      ledcWrite(channelRoyalBlue, royalBlue);
      ledcWrite(channelBlue, blue);
      ledcWrite(channelUV, uv);
      ledcWrite(channelViolet, violet);
      ledcWrite(channelRed, red);
      ledcWrite(channelGreen, green);
      ledcWrite(channelWhite, white);
    }
  }
  
  // Jika mengubah dari manual/off ke auto, langsung update LED sesuai jadwal
  if (manualMode && !enable) {
    Serial.println(">>> SWITCHING TO AUTO MODE - APPLYING SCHEDULED PROFILE IMMEDIATELY <<<");
    LightProfile currentProfile = getCurrentProfile();
    setLightProfile(currentProfile);
  }
  
  manualMode = enable;
  saveModeToPreferences();
  
  Serial.print("Mode changed successfully. New mode: ");
  Serial.println(manualMode ? "MANUAL" : "AUTO");
}

bool LedController::isInManualMode() {
  return manualMode;
}

void LedController::setRoyalBlue(uint8_t intensity) {
  ledcWrite(channelRoyalBlue, intensity);
  // Jika dalam mode manual, simpan pengaturan
  if (manualMode) {
    // Pastikan semua channel lain juga tersimpan (inisialisasi jika belum ada)
    ensureAllManualChannelsSaved();
    // Simpan nilai LED ini (key pendek: m_rb = manual_royalBlue)
    preferences.putUChar("m_rb", intensity);
    Serial.print("Royal Blue saved: "); Serial.println(intensity);
  }
}

void LedController::setBlue(uint8_t intensity) {
  ledcWrite(channelBlue, intensity);
  // Jika dalam mode manual, simpan pengaturan
  if (manualMode) {
    ensureAllManualChannelsSaved();
    preferences.putUChar("m_b", intensity);
    Serial.print("Blue saved: "); Serial.println(intensity);
  }
}

void LedController::setUV(uint8_t intensity) {
  ledcWrite(channelUV, intensity);
  // Jika dalam mode manual, simpan pengaturan
  if (manualMode) {
    ensureAllManualChannelsSaved();
    preferences.putUChar("m_uv", intensity);
    Serial.print("UV saved: "); Serial.println(intensity);
  }
}

void LedController::setViolet(uint8_t intensity) {
  ledcWrite(channelViolet, intensity);
  // Jika dalam mode manual, simpan pengaturan
  if (manualMode) {
    ensureAllManualChannelsSaved();
    preferences.putUChar("m_v", intensity);
    Serial.print("Violet saved: "); Serial.println(intensity);
  }
}

void LedController::setRed(uint8_t intensity) {
  ledcWrite(channelRed, intensity);
  // Jika dalam mode manual, simpan pengaturan
  if (manualMode) {
    ensureAllManualChannelsSaved();
    preferences.putUChar("m_r", intensity);
    Serial.print("Red saved: "); Serial.println(intensity);
  }
}

void LedController::setGreen(uint8_t intensity) {
  ledcWrite(channelGreen, intensity);
  // Jika dalam mode manual, simpan pengaturan
  if (manualMode) {
    ensureAllManualChannelsSaved();
    preferences.putUChar("m_g", intensity);
    Serial.print("Green saved: "); Serial.println(intensity);
  }
}

void LedController::setWhite(uint8_t intensity) {
  ledcWrite(channelWhite, intensity);
  // Jika dalam mode manual, simpan pengaturan
  if (manualMode) {
    ensureAllManualChannelsSaved();
    preferences.putUChar("m_w", intensity);
    Serial.print("White saved: "); Serial.println(intensity);
  }
}

void LedController::update() {
  // Debug logging every 10 seconds
  static unsigned long lastDebugLog = 0;
  if (millis() - lastDebugLog > 10000) {
    lastDebugLog = millis();
    Serial.print("LED Controller Status - Mode: ");
    if (offMode) {
      Serial.println("OFF");
    } else if (manualMode) {
      Serial.println("MANUAL");
    } else {
      Serial.println("AUTO");
    }
  }
  
  // In off mode, don't update anything
  if (offMode) {
    return;
  }
  
  // In manual mode, we don't update based on time
  if (manualMode) {
    return;
  }
  
  // Otherwise, update based on time (AUTO MODE)
  LightProfile profile = getCurrentProfile();
  setLightProfile(profile);
}

void LedController::setLightProfile(LightProfile profile) {
  // Set PWM values for each LED
  ledcWrite(channelRoyalBlue, profile.royalBlue);
  ledcWrite(channelBlue, profile.blue);
  ledcWrite(channelUV, profile.uv);
  ledcWrite(channelViolet, profile.violet);
  ledcWrite(channelRed, profile.red);
  ledcWrite(channelGreen, profile.green);
  ledcWrite(channelWhite, profile.white);
  
  // Print current LED intensities
  Serial.println("===== LED VALUES APPLIED =====");
  printCurrentProfile(profile);
  Serial.println("==============================");
}

void LedController::setLightProfileFromJson(String jsonProfile) {
  LightProfile profile = parseProfileJson(jsonProfile);
  setLightProfile(profile);
}

void LedController::printCurrentProfile(LightProfile profile) {
  Serial.println("Current LED intensities:");
  Serial.print("Royal Blue: "); Serial.println(profile.royalBlue);
  Serial.print("Blue: "); Serial.println(profile.blue);
  Serial.print("UV: "); Serial.println(profile.uv);
  Serial.print("Violet: "); Serial.println(profile.violet);
  Serial.print("Red: "); Serial.println(profile.red);
  Serial.print("Green: "); Serial.println(profile.green);
  Serial.print("White: "); Serial.println(profile.white);
}

String LedController::getCurrentProfileJson() {
  return profileToJson(getCurrentProfile());
}

String LedController::getCurrentTimeJson() {
  DateTime now = rtc->now();
  
  // Alokasi memori untuk JSON document
  StaticJsonDocument<100> doc;
  
  // Isi dengan data waktu
  doc["year"] = now.year();
  doc["month"] = now.month();
  doc["day"] = now.day();
  doc["hour"] = now.hour();
  doc["minute"] = now.minute();
  doc["second"] = now.second();
  
  // Serialize JSON ke string
  String output;
  serializeJson(doc, output);
  return output;
}

bool LedController::setCurrentTime(String timeJson) {
  // Parse JSON time format
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, timeJson);
  
  // Check for parsing errors
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return false;
  }
  
  // Extract time values with validation
  if (!doc.containsKey("year") || !doc.containsKey("month") || !doc.containsKey("day") ||
      !doc.containsKey("hour") || !doc.containsKey("minute") || !doc.containsKey("second")) {
    Serial.println("JSON missing required time fields");
    return false;
  }
  
  int year = doc["year"].as<int>();
  int month = doc["month"].as<int>();
  int day = doc["day"].as<int>();
  int hour = doc["hour"].as<int>();
  int minute = doc["minute"].as<int>();
  int second = doc["second"].as<int>();
  
  // Basic validation
  if (year < 2000 || year > 2100 || month < 1 || month > 12 || day < 1 || day > 31 ||
      hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
    Serial.println("Invalid time values");
    return false;
  }
  
  // Set RTC time
  DateTime newTime(year, month, day, hour, minute, second);
  rtc->adjust(newTime);
  
  Serial.print("RTC time set to: ");
  Serial.print(year);
  Serial.print("-");
  Serial.print(month);
  Serial.print("-");
  Serial.print(day);
  Serial.print(" ");
  Serial.print(hour);
  Serial.print(":");
  Serial.print(minute);
  Serial.print(":");
  Serial.println(second);
  
  return true;
}

LightProfile LedController::getCurrentProfile() {
  // Get current time from RTC
  DateTime now = rtc->now();
  
  int hour = now.hour();
  int minute = now.minute();
  
  Serial.print("Getting current profile for time: ");
  Serial.print(hour);
  Serial.print(":");
  Serial.print(minute);
  Serial.print(" (Hour: ");
  Serial.print(hour);
  Serial.println(")");
  
  // Get the profile for the current hour
  LightProfile currentHourProfile = hourlySchedule[hour].profile;
  
  Serial.print("Current hour profile (hour ");
  Serial.print(hour);
  Serial.print("): R=");
  Serial.print(currentHourProfile.royalBlue);
  Serial.print(" B=");
  Serial.print(currentHourProfile.blue);
  Serial.print(" W=");
  Serial.println(currentHourProfile.white);
  
  // Get the profile for the next hour (with wrap-around)
  int nextHour = (hour + 1) % 24;
  LightProfile nextHourProfile = hourlySchedule[nextHour].profile;
  
  // Interpolate between current and next hour based on minutes
  float ratio = minute / 60.0;
  LightProfile result = interpolateProfiles(currentHourProfile, nextHourProfile, ratio);
  
  Serial.print("Interpolated result (ratio=");
  Serial.print(ratio);
  Serial.print("): R=");
  Serial.print(result.royalBlue);
  Serial.print(" B=");
  Serial.print(result.blue);
  Serial.print(" W=");
  Serial.println(result.white);
  
  return result;
}

LightProfile LedController::interpolateProfiles(LightProfile profile1, LightProfile profile2, float ratio) {
  // Ensure ratio is between 0 and 1
  if (ratio < 0) ratio = 0;
  if (ratio > 1) ratio = 1;
  
  LightProfile result;
  
  // Linear interpolation between profiles
  result.royalBlue = profile1.royalBlue + (profile2.royalBlue - profile1.royalBlue) * ratio;
  result.blue = profile1.blue + (profile2.blue - profile1.blue) * ratio;
  result.uv = profile1.uv + (profile2.uv - profile1.uv) * ratio;
  result.violet = profile1.violet + (profile2.violet - profile1.violet) * ratio;
  result.red = profile1.red + (profile2.red - profile1.red) * ratio;
  result.green = profile1.green + (profile2.green - profile1.green) * ratio;
  result.white = profile1.white + (profile2.white - profile1.white) * ratio;
  
  return result;
}

void LedController::setAllLeds(uint8_t royalBlue, uint8_t blue, uint8_t uv, uint8_t violet, 
                              uint8_t red, uint8_t green, uint8_t white) {
  // Set semua LED sekaligus
  setRoyalBlue(royalBlue);
  setBlue(blue);
  setUV(uv);
  setViolet(violet);
  setRed(red);
  setGreen(green);
  setWhite(white);
  
  // Log the values
  Serial.println("Setting all LEDs to:");
  Serial.print("Royal Blue: "); Serial.println(royalBlue);
  Serial.print("Blue: "); Serial.println(blue);
  Serial.print("UV: "); Serial.println(uv);
  Serial.print("Violet: "); Serial.println(violet);
  Serial.print("Red: "); Serial.println(red);
  Serial.print("Green: "); Serial.println(green);
  Serial.print("White: "); Serial.println(white);
}

void LedController::setAllLedsFromJson(String jsonProfile) {
  // Parse JSON
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, jsonProfile);
  
  // Check for parsing errors
  if (error) {
    Serial.print("LedController JSON parsing failed: ");
    Serial.println(error.c_str());
    return;
  }
  
  // Extract values with error checking and default values
  uint8_t royalBlue = doc.containsKey("royalBlue") ? (uint8_t)doc["royalBlue"] : 0;
  uint8_t blue = doc.containsKey("blue") ? (uint8_t)doc["blue"] : 0;
  uint8_t uv = doc.containsKey("uv") ? (uint8_t)doc["uv"] : 0;
  uint8_t violet = doc.containsKey("violet") ? (uint8_t)doc["violet"] : 0;
  uint8_t red = doc.containsKey("red") ? (uint8_t)doc["red"] : 0;
  uint8_t green = doc.containsKey("green") ? (uint8_t)doc["green"] : 0;
  uint8_t white = doc.containsKey("white") ? (uint8_t)doc["white"] : 0;
  
  // Apply the values
  setAllLeds(royalBlue, blue, uv, violet, red, green, white);
}

// Off mode control methods
void LedController::setOffMode(bool off) {
  offMode = off;
  if (off) {
    // Turn off all LEDs
    setAllLeds(0, 0, 0, 0, 0, 0, 0);
    // Also set to manual mode to prevent auto lighting
    manualMode = true;
    Serial.println("OFF mode enabled - all LEDs turned off");
  } else {
    Serial.println("OFF mode disabled");
  }
  // Save state to preferences (gunakan namespace yang sama: "led_ctrl")
  preferences.putBool("off_mode", offMode);
  preferences.putBool("manual_mode", manualMode);
  Serial.println("Off mode and manual mode saved to preferences");
}

bool LedController::isInOffMode() {
  return offMode;
}

// Destructor
LedController::~LedController() {
  // Free resources
  preferences.end();
}

// ========== HOURLY SCHEDULE FUNCTIONS ==========

void LedController::setHourlyProfile(uint8_t hour, LightProfile profile) {
  if (hour > 23) {
    Serial.println("ERROR: Invalid hour value (must be 0-23)");
    return;
  }
  
  Serial.print(">>> setHourlyProfile called for hour ");
  Serial.print(hour);
  Serial.print(" with values: R=");
  Serial.print(profile.royalBlue);
  Serial.print(" B=");
  Serial.print(profile.blue);
  Serial.print(" W=");
  Serial.println(profile.white);
  
  hourlySchedule[hour].hour = hour;
  hourlySchedule[hour].profile = profile;
  
  // Save immediately to NVS (preferences already opened in begin())
  String key = "h" + String(hour);
  String profileJson = profileToJson(profile);
  preferences.putString(key.c_str(), profileJson);
  
  Serial.print(">>> Hour ");
  Serial.print(hour);
  Serial.println(" SAVED TO NVS (non-volatile storage)");
  
  // Verify by reading back
  String verifyJson = preferences.getString(key.c_str(), "");
  
  Serial.print(">>> VERIFICATION: Read back from NVS: ");
  Serial.println(verifyJson);
}

LightProfile LedController::getHourlyProfile(uint8_t hour) {
  if (hour > 23) {
    Serial.println("Invalid hour value (must be 0-23)");
    return {0, 0, 0, 0, 0, 0, 0};
  }
  
  return hourlySchedule[hour].profile;
}

void LedController::setHourlySchedule(String jsonSchedule) {
  // Parse JSON array - INCREASED BUFFER SIZE to 6144 bytes for 24-hour schedule
  DynamicJsonDocument doc(6144);
  DeserializationError error = deserializeJson(doc, jsonSchedule);
  
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return;
  }
  
  // Check if the root is an array
  if (!doc.is<JsonArray>()) {
    // Maybe it's wrapped in an object with "schedule" key
    if (doc.containsKey("schedule")) {
      JsonArray scheduleArray = doc["schedule"].as<JsonArray>();
      
      for (JsonObject hourObj : scheduleArray) {
        if (hourObj.containsKey("hour")) {
          uint8_t hour = hourObj["hour"];
          
          if (hour <= 23) {
            LightProfile profile;
            profile.royalBlue = hourObj.containsKey("royalBlue") ? (uint8_t)hourObj["royalBlue"] : 0;
            profile.blue = hourObj.containsKey("blue") ? (uint8_t)hourObj["blue"] : 0;
            profile.uv = hourObj.containsKey("uv") ? (uint8_t)hourObj["uv"] : 0;
            profile.violet = hourObj.containsKey("violet") ? (uint8_t)hourObj["violet"] : 0;
            profile.red = hourObj.containsKey("red") ? (uint8_t)hourObj["red"] : 0;
            profile.green = hourObj.containsKey("green") ? (uint8_t)hourObj["green"] : 0;
            profile.white = hourObj.containsKey("white") ? (uint8_t)hourObj["white"] : 0;
            
            setHourlyProfile(hour, profile);
          }
        }
      }
    } else {
      Serial.println("Invalid JSON format for hourly schedule");
      return;
    }
  } else {
    // Process as direct array
    JsonArray scheduleArray = doc.as<JsonArray>();
    
    for (JsonObject hourObj : scheduleArray) {
      if (hourObj.containsKey("hour")) {
        uint8_t hour = hourObj["hour"];
        
        if (hour <= 23) {
          LightProfile profile;
          profile.royalBlue = hourObj.containsKey("royalBlue") ? (uint8_t)hourObj["royalBlue"] : 0;
          profile.blue = hourObj.containsKey("blue") ? (uint8_t)hourObj["blue"] : 0;
          profile.uv = hourObj.containsKey("uv") ? (uint8_t)hourObj["uv"] : 0;
          profile.violet = hourObj.containsKey("violet") ? (uint8_t)hourObj["violet"] : 0;
          profile.red = hourObj.containsKey("red") ? (uint8_t)hourObj["red"] : 0;
          profile.green = hourObj.containsKey("green") ? (uint8_t)hourObj["green"] : 0;
          profile.white = hourObj.containsKey("white") ? (uint8_t)hourObj["white"] : 0;
          
          setHourlyProfile(hour, profile);
        }
      }
    }
  }
  
  // Save to preferences
  saveHourlyScheduleToPreferences();
  
  Serial.println("Hourly schedule updated from JSON");
  
  // If in auto mode, immediately apply the new schedule
  if (!manualMode && !offMode) {
    Serial.println("Auto mode active - applying new schedule immediately");
    LightProfile currentProfile = getCurrentProfile();
    setLightProfile(currentProfile);
  }
}

String LedController::getHourlyScheduleJson() {
  // Create JSON document with larger buffer
  DynamicJsonDocument doc(4096);
  JsonArray scheduleArray = doc.createNestedArray("schedule");
  
  for (int i = 0; i < 24; i++) {
    JsonObject hourObj = scheduleArray.createNestedObject();
    hourObj["hour"] = hourlySchedule[i].hour;
    hourObj["royalBlue"] = hourlySchedule[i].profile.royalBlue;
    hourObj["blue"] = hourlySchedule[i].profile.blue;
    hourObj["uv"] = hourlySchedule[i].profile.uv;
    hourObj["violet"] = hourlySchedule[i].profile.violet;
    hourObj["red"] = hourlySchedule[i].profile.red;
    hourObj["green"] = hourlySchedule[i].profile.green;
    hourObj["white"] = hourlySchedule[i].profile.white;
  }
  
  String output;
  serializeJson(doc, output);
  return output;
}

void LedController::saveHourlyScheduleToPreferences() {
  // Preferences already opened in begin() - no need to open/close
  
  // Save each hour's profile
  for (int i = 0; i < 24; i++) {
    String key = "h" + String(i);
    String profileJson = profileToJson(hourlySchedule[i].profile);
    preferences.putString(key.c_str(), profileJson);
  }
  
  Serial.println("Hourly schedule saved to preferences");
}

void LedController::loadHourlyScheduleFromPreferences() {
  // Preferences already opened in begin() - no need to open/close
  
  // Load each hour's profile
  bool foundSchedule = false;
  for (int i = 0; i < 24; i++) {
    String key = "h" + String(i);
    if (preferences.isKey(key.c_str())) {
      String profileJson = preferences.getString(key.c_str(), "");
      if (profileJson.length() > 0) {
        hourlySchedule[i].hour = i;
        hourlySchedule[i].profile = parseProfileJson(profileJson);
        foundSchedule = true;
      }
    }
  }
  
  if (foundSchedule) {
    Serial.println("Loaded hourly schedule from preferences");
  } else {
    Serial.println("No saved hourly schedule found, using defaults");
  }
}

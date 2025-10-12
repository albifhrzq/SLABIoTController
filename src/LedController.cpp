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
  
  // Initialize hourly schedule with default values
  setDefaultHourlySchedule();
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
  
  Serial.println("LED Controller initialized");
}

// Save mode to preferences
void LedController::saveModeToPreferences() {
  preferences.putBool("manual_mode", manualMode);
  
  Serial.println("Mode saved to preferences");
}

// Load all preferences
void LedController::loadPreferences() {
  // Load mode if saved
  if (preferences.isKey("manual_mode")) {
    manualMode = preferences.getBool("manual_mode", false);
    Serial.println("Loaded mode from preferences: " + String(manualMode ? "manual" : "auto"));
    
    // Load off mode if saved
    if (preferences.isKey("off_mode")) {
      offMode = preferences.getBool("off_mode", false);
      Serial.println("Loaded off mode from preferences: " + String(offMode ? "ON" : "OFF"));
    }
    
    // Jika dalam mode manual, muat pengaturan LED manual
    if (manualMode) {
      // Muat pengaturan manual hanya jika key tersedia
      if (preferences.isKey("manual_royal_blue")) {
        uint8_t royalBlue = preferences.getUChar("manual_royal_blue", 0);
        uint8_t blue = preferences.getUChar("manual_blue", 0);
        uint8_t uv = preferences.getUChar("manual_uv", 0);
        uint8_t violet = preferences.getUChar("manual_violet", 0);
        uint8_t red = preferences.getUChar("manual_red", 0);
        uint8_t green = preferences.getUChar("manual_green", 0);
        uint8_t white = preferences.getUChar("manual_white", 0);
        
        // Terapkan pengaturan LED tanpa menyimpan lagi (hindari rekursi)
        ledcWrite(channelRoyalBlue, royalBlue);
        ledcWrite(channelBlue, blue);
        ledcWrite(channelUV, uv);
        ledcWrite(channelViolet, violet);
        ledcWrite(channelRed, red);
        ledcWrite(channelGreen, green);
        ledcWrite(channelWhite, white);
        
        Serial.println("Loaded manual LED settings from preferences");
      }
    }
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
  // Jika mengubah dari auto ke manual, simpan pengaturan LED saat ini
  if (!manualMode && enable) {
    // Jika tidak ada pengaturan manual sebelumnya, gunakan pengaturan profil saat ini
    if (!preferences.isKey("manual_royal_blue")) {
      LightProfile currentProfile = getCurrentProfile();
      
      preferences.putUChar("manual_royal_blue", currentProfile.royalBlue);
      preferences.putUChar("manual_blue", currentProfile.blue);
      preferences.putUChar("manual_uv", currentProfile.uv);
      preferences.putUChar("manual_violet", currentProfile.violet);
      preferences.putUChar("manual_red", currentProfile.red);
      preferences.putUChar("manual_green", currentProfile.green);
      preferences.putUChar("manual_white", currentProfile.white);
      
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
      uint8_t royalBlue = preferences.getUChar("manual_royal_blue", 0);
      uint8_t blue = preferences.getUChar("manual_blue", 0);
      uint8_t uv = preferences.getUChar("manual_uv", 0);
      uint8_t violet = preferences.getUChar("manual_violet", 0);
      uint8_t red = preferences.getUChar("manual_red", 0);
      uint8_t green = preferences.getUChar("manual_green", 0);
      uint8_t white = preferences.getUChar("manual_white", 0);
      
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
  
  manualMode = enable;
  saveModeToPreferences();
}

bool LedController::isInManualMode() {
  return manualMode;
}

void LedController::setRoyalBlue(uint8_t intensity) {
  ledcWrite(channelRoyalBlue, intensity);
  // Jika dalam mode manual, simpan pengaturan
  if (manualMode) {
    preferences.putUChar("manual_royal_blue", intensity);
  }
}

void LedController::setBlue(uint8_t intensity) {
  ledcWrite(channelBlue, intensity);
  // Jika dalam mode manual, simpan pengaturan
  if (manualMode) {
    preferences.putUChar("manual_blue", intensity);
  }
}

void LedController::setUV(uint8_t intensity) {
  ledcWrite(channelUV, intensity);
  // Jika dalam mode manual, simpan pengaturan
  if (manualMode) {
    preferences.putUChar("manual_uv", intensity);
  }
}

void LedController::setViolet(uint8_t intensity) {
  ledcWrite(channelViolet, intensity);
  // Jika dalam mode manual, simpan pengaturan
  if (manualMode) {
    preferences.putUChar("manual_violet", intensity);
  }
}

void LedController::setRed(uint8_t intensity) {
  ledcWrite(channelRed, intensity);
  // Jika dalam mode manual, simpan pengaturan
  if (manualMode) {
    preferences.putUChar("manual_red", intensity);
  }
}

void LedController::setGreen(uint8_t intensity) {
  ledcWrite(channelGreen, intensity);
  // Jika dalam mode manual, simpan pengaturan
  if (manualMode) {
    preferences.putUChar("manual_green", intensity);
  }
}

void LedController::setWhite(uint8_t intensity) {
  ledcWrite(channelWhite, intensity);
  // Jika dalam mode manual, simpan pengaturan
  if (manualMode) {
    preferences.putUChar("manual_white", intensity);
  }
}

void LedController::update() {
  // In off mode, don't update anything
  if (offMode) {
    return;
  }
  
  // In manual mode, we don't update based on time
  if (manualMode) {
    return;
  }
  
  // Otherwise, update based on time
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
  printCurrentProfile(profile);
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
  
  // Get the profile for the current hour
  LightProfile currentHourProfile = hourlySchedule[hour].profile;
  
  // Get the profile for the next hour (with wrap-around)
  int nextHour = (hour + 1) % 24;
  LightProfile nextHourProfile = hourlySchedule[nextHour].profile;
  
  // Interpolate between current and next hour based on minutes
  float ratio = minute / 60.0;
  return interpolateProfiles(currentHourProfile, nextHourProfile, ratio);
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
  // Save state to preferences
  preferences.begin("led_controller", false);
  preferences.putBool("off_mode", offMode);
  preferences.end();
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

void LedController::setDefaultHourlySchedule() {
  // Set default hourly profiles
  // Night hours (0-6): Dim blue light
  for (int i = 0; i <= 6; i++) {
    hourlySchedule[i].hour = i;
    hourlySchedule[i].profile = {20, 50, 30, 20, 10, 10, 0};
  }
  
  // Morning ramp-up (7-9): Gradually increase intensity
  hourlySchedule[7].hour = 7;
  hourlySchedule[7].profile = {80, 120, 40, 40, 100, 80, 150};
  
  hourlySchedule[8].hour = 8;
  hourlySchedule[8].profile = {120, 160, 60, 60, 120, 120, 180};
  
  hourlySchedule[9].hour = 9;
  hourlySchedule[9].profile = {160, 200, 100, 80, 100, 160, 220};
  
  // Peak daylight (10-16): Bright full spectrum
  for (int i = 10; i <= 16; i++) {
    hourlySchedule[i].hour = i;
    hourlySchedule[i].profile = {200, 255, 150, 100, 100, 200, 255};
  }
  
  // Evening ramp-down (17-19): Warmer tones
  hourlySchedule[17].hour = 17;
  hourlySchedule[17].profile = {120, 180, 180, 140, 180, 80, 140};
  
  hourlySchedule[18].hour = 18;
  hourlySchedule[18].profile = {100, 150, 200, 150, 200, 50, 100};
  
  hourlySchedule[19].hour = 19;
  hourlySchedule[19].profile = {60, 100, 120, 100, 120, 30, 50};
  
  // Late evening (20): Transition to night
  hourlySchedule[20].hour = 20;
  hourlySchedule[20].profile = {40, 80, 80, 60, 60, 20, 20};
  
  // Night hours (21-23): Dim blue light
  for (int i = 21; i <= 23; i++) {
    hourlySchedule[i].hour = i;
    hourlySchedule[i].profile = {20, 50, 30, 20, 10, 10, 0};
  }
  
  Serial.println("Default hourly schedule set");
}

void LedController::setHourlyProfile(uint8_t hour, LightProfile profile) {
  if (hour > 23) {
    Serial.println("Invalid hour value (must be 0-23)");
    return;
  }
  
  hourlySchedule[hour].hour = hour;
  hourlySchedule[hour].profile = profile;
  
  Serial.print("Hourly profile for hour ");
  Serial.print(hour);
  Serial.println(" updated");
}

LightProfile LedController::getHourlyProfile(uint8_t hour) {
  if (hour > 23) {
    Serial.println("Invalid hour value (must be 0-23)");
    return {0, 0, 0, 0, 0, 0, 0};
  }
  
  return hourlySchedule[hour].profile;
}

void LedController::setHourlySchedule(String jsonSchedule) {
  // Parse JSON array
  StaticJsonDocument<4096> doc; // Larger buffer for 24-hour schedule
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
  preferences.begin("led_ctrl", false);
  
  // Save each hour's profile
  for (int i = 0; i < 24; i++) {
    String key = "h" + String(i);
    String profileJson = profileToJson(hourlySchedule[i].profile);
    preferences.putString(key.c_str(), profileJson);
  }
  
  preferences.end();
  Serial.println("Hourly schedule saved to preferences");
}

void LedController::loadHourlyScheduleFromPreferences() {
  preferences.begin("led_ctrl", false);
  
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
  
  preferences.end();
}

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
  
  // Set default time ranges
  this->morningStart = 7;
  this->middayStart = 10;
  this->eveningStart = 17;
  this->nightStart = 21;
  
  // Default to auto mode
  this->manualMode = false;
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
  
  Serial.println("LED Controller initialized");
}

// Save all profiles to preferences
void LedController::saveProfilesToPreferences() {
  // Save morning profile
  String morningJson = profileToJson(morningProfile);
  preferences.putString("profile_morning", morningJson);
  
  // Save midday profile
  String middayJson = profileToJson(middayProfile);
  preferences.putString("profile_midday", middayJson);
  
  // Save evening profile
  String eveningJson = profileToJson(eveningProfile);
  preferences.putString("profile_evening", eveningJson);
  
  // Save night profile
  String nightJson = profileToJson(nightProfile);
  preferences.putString("profile_night", nightJson);
  
  Serial.println("Profiles saved to preferences");
}

// Save time ranges to preferences
void LedController::saveTimeRangesToPreferences() {
  preferences.putInt("morning_start", morningStart);
  preferences.putInt("midday_start", middayStart);
  preferences.putInt("evening_start", eveningStart);
  preferences.putInt("night_start", nightStart);
  
  Serial.println("Time ranges saved to preferences");
}

// Save mode to preferences
void LedController::saveModeToPreferences() {
  preferences.putBool("manual_mode", manualMode);
  
  Serial.println("Mode saved to preferences");
}

// Load all preferences
void LedController::loadPreferences() {
  // Check if profiles are saved
  if (preferences.isKey("profile_morning")) {
    String morningJson = preferences.getString("profile_morning", "");
    if (morningJson.length() > 0) {
      morningProfile = parseProfileJson(morningJson);
      Serial.println("Loaded morning profile from preferences");
    }
  }
  
  if (preferences.isKey("profile_midday")) {
    String middayJson = preferences.getString("profile_midday", "");
    if (middayJson.length() > 0) {
      middayProfile = parseProfileJson(middayJson);
      Serial.println("Loaded midday profile from preferences");
    }
  }
  
  if (preferences.isKey("profile_evening")) {
    String eveningJson = preferences.getString("profile_evening", "");
    if (eveningJson.length() > 0) {
      eveningProfile = parseProfileJson(eveningJson);
      Serial.println("Loaded evening profile from preferences");
    }
  }
  
  if (preferences.isKey("profile_night")) {
    String nightJson = preferences.getString("profile_night", "");
    if (nightJson.length() > 0) {
      nightProfile = parseProfileJson(nightJson);
      Serial.println("Loaded night profile from preferences");
    }
  }
  
  // Load time ranges if saved
  if (preferences.isKey("morning_start")) {
    morningStart = preferences.getInt("morning_start", 7);
    middayStart = preferences.getInt("midday_start", 10);
    eveningStart = preferences.getInt("evening_start", 17);
    nightStart = preferences.getInt("night_start", 21);
    Serial.println("Loaded time ranges from preferences");
  }
  
  // Load mode if saved
  if (preferences.isKey("manual_mode")) {
    manualMode = preferences.getBool("manual_mode", false);
    Serial.println("Loaded mode from preferences: " + String(manualMode ? "manual" : "auto"));
    
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
  
  // If no profiles were loaded from preferences, set defaults
  if (!preferences.isKey("profile_morning")) {
    setDefaultProfiles();
    saveProfilesToPreferences();
  }
}

// Save all preferences at once
void LedController::saveAllPreferences() {
  saveProfilesToPreferences();
  saveTimeRangesToPreferences();
  saveModeToPreferences();
}

void LedController::setDefaultProfiles() {
  // Morning profile (soft morning light)
  morningProfile = {100, 150, 50, 50, 150, 100, 200};
  
  // Midday profile (bright midday light)
  middayProfile = {200, 255, 150, 100, 100, 200, 255};
  
  // Evening profile (warmer evening light)
  eveningProfile = {100, 150, 200, 150, 200, 50, 100};
  
  // Night profile (dim blue-heavy night light)
  nightProfile = {20, 50, 30, 20, 10, 10, 0};
}

void LedController::setMorningProfile(LightProfile profile) {
  morningProfile = profile;
  saveProfilesToPreferences();
}

void LedController::setMorningProfile(String jsonProfile) {
  morningProfile = parseProfileJson(jsonProfile);
  saveProfilesToPreferences();
}

void LedController::setMiddayProfile(LightProfile profile) {
  middayProfile = profile;
  saveProfilesToPreferences();
}

void LedController::setMiddayProfile(String jsonProfile) {
  middayProfile = parseProfileJson(jsonProfile);
  saveProfilesToPreferences();
}

void LedController::setEveningProfile(LightProfile profile) {
  eveningProfile = profile;
  saveProfilesToPreferences();
}

void LedController::setEveningProfile(String jsonProfile) {
  eveningProfile = parseProfileJson(jsonProfile);
  saveProfilesToPreferences();
}

void LedController::setNightProfile(LightProfile profile) {
  nightProfile = profile;
  saveProfilesToPreferences();
}

void LedController::setNightProfile(String jsonProfile) {
  nightProfile = parseProfileJson(jsonProfile);
  saveProfilesToPreferences();
}

String LedController::getMorningProfileJson() {
  return profileToJson(morningProfile);
}

String LedController::getMiddayProfileJson() {
  return profileToJson(middayProfile);
}

String LedController::getEveningProfileJson() {
  return profileToJson(eveningProfile);
}

String LedController::getNightProfileJson() {
  return profileToJson(nightProfile);
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

void LedController::setTimeRanges(int morningStart, int middayStart, int eveningStart, int nightStart) {
  this->morningStart = morningStart;
  this->middayStart = middayStart;
  this->eveningStart = eveningStart;
  this->nightStart = nightStart;
  saveTimeRangesToPreferences();
}

String LedController::getTimeRangesJson() {
  // Alokasi memori untuk JSON document
  StaticJsonDocument<100> doc;
  
  // Isi dengan data waktu
  doc["morningStart"] = morningStart;
  doc["middayStart"] = middayStart;
  doc["eveningStart"] = eveningStart;
  doc["nightStart"] = nightStart;
  
  // Serialize JSON ke string
  String output;
  serializeJson(doc, output);
  return output;
}

void LedController::setTimeRangesFromJson(String jsonTimeRanges) {
  // Parse JSON
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, jsonTimeRanges);
  
  // Check for parsing errors
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return;
  }
  
  // Ekstrak nilai dengan pengecekan keberadaan key
  if (doc.containsKey("morningStart") && doc.containsKey("middayStart") && 
      doc.containsKey("eveningStart") && doc.containsKey("nightStart")) {
    
    int newMorningStart = doc["morningStart"];
    int newMiddayStart = doc["middayStart"];
    int newEveningStart = doc["eveningStart"];
    int newNightStart = doc["nightStart"];
    
    // Set new time ranges
    setTimeRanges(newMorningStart, newMiddayStart, newEveningStart, newNightStart);
  }
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
  float hourFloat = hour + minute / 60.0;
  
  // Determine which profiles to interpolate between
  LightProfile profile1, profile2;
  float ratio;
  
  if (hourFloat >= nightStart || hourFloat < morningStart) {
    // Night profile
    return nightProfile;
  } else if (hourFloat >= morningStart && hourFloat < middayStart) {
    // Morning transition
    profile1 = morningProfile;
    profile2 = middayProfile;
    ratio = (hourFloat - morningStart) / (middayStart - morningStart);
  } else if (hourFloat >= middayStart && hourFloat < eveningStart) {
    // Midday profile
    return middayProfile;
  } else if (hourFloat >= eveningStart && hourFloat < nightStart) {
    // Evening transition
    profile1 = eveningProfile;
    profile2 = nightProfile;
    ratio = (hourFloat - eveningStart) / (nightStart - eveningStart);
  }
  
  // Interpolate between profiles
  return interpolateProfiles(profile1, profile2, ratio);
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

// Destructor
LedController::~LedController() {
  // Free resources
  preferences.end();
} 
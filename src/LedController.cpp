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
  
  // Set default profiles
  setDefaultProfiles();
  
  Serial.println("LED Controller initialized");
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
}

void LedController::setMorningProfile(String jsonProfile) {
  morningProfile = parseProfileJson(jsonProfile);
}

void LedController::setMiddayProfile(LightProfile profile) {
  middayProfile = profile;
}

void LedController::setMiddayProfile(String jsonProfile) {
  middayProfile = parseProfileJson(jsonProfile);
}

void LedController::setEveningProfile(LightProfile profile) {
  eveningProfile = profile;
}

void LedController::setEveningProfile(String jsonProfile) {
  eveningProfile = parseProfileJson(jsonProfile);
}

void LedController::setNightProfile(LightProfile profile) {
  nightProfile = profile;
}

void LedController::setNightProfile(String jsonProfile) {
  nightProfile = parseProfileJson(jsonProfile);
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
  StaticJsonDocument<100> doc;
  DeserializationError error = deserializeJson(doc, jsonTimeRanges);
  
  // Check for parsing errors
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return;
  }
  
  // Ekstrak nilai dengan pengecekan keberadaan key
  if (doc.containsKey("morningStart")) morningStart = doc["morningStart"];
  if (doc.containsKey("middayStart")) middayStart = doc["middayStart"];
  if (doc.containsKey("eveningStart")) eveningStart = doc["eveningStart"];
  if (doc.containsKey("nightStart")) nightStart = doc["nightStart"];
}

void LedController::enableManualMode(bool enable) {
  manualMode = enable;
}

bool LedController::isInManualMode() {
  return manualMode;
}

void LedController::setRoyalBlue(uint8_t intensity) {
  ledcWrite(channelRoyalBlue, intensity);
}

void LedController::setBlue(uint8_t intensity) {
  ledcWrite(channelBlue, intensity);
}

void LedController::setUV(uint8_t intensity) {
  ledcWrite(channelUV, intensity);
}

void LedController::setViolet(uint8_t intensity) {
  ledcWrite(channelViolet, intensity);
}

void LedController::setRed(uint8_t intensity) {
  ledcWrite(channelRed, intensity);
}

void LedController::setGreen(uint8_t intensity) {
  ledcWrite(channelGreen, intensity);
}

void LedController::setWhite(uint8_t intensity) {
  ledcWrite(channelWhite, intensity);
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
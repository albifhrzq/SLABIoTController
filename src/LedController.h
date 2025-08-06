#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>
#include <RTClib.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// Light profile structure for different times of day
struct LightProfile {
  uint8_t royalBlue;
  uint8_t blue;
  uint8_t uv;
  uint8_t violet;
  uint8_t red;
  uint8_t green;
  uint8_t white;
};

class LedController {
private:
  // LED control pins
  uint8_t pinRoyalBlue;
  uint8_t pinBlue;
  uint8_t pinUV;
  uint8_t pinViolet;
  uint8_t pinRed;
  uint8_t pinGreen;
  uint8_t pinWhite;
  
  // PWM channels
  uint8_t channelRoyalBlue;
  uint8_t channelBlue;
  uint8_t channelUV;
  uint8_t channelViolet;
  uint8_t channelRed;
  uint8_t channelGreen;
  uint8_t channelWhite;
  
  // PWM properties
  uint32_t freq;
  uint8_t resolution;
  
  // Light profiles
  LightProfile morningProfile;
  LightProfile middayProfile;
  LightProfile eveningProfile;
  LightProfile nightProfile;
  
  // Time ranges
  int morningStart;
  int middayStart;
  int eveningStart;
  int nightStart;
  
  // RTC instance reference
  RTC_DS3231* rtc;
  
  // Current operating mode
  bool manualMode;
  bool offMode; // New flag to track off mode
  
  // Preferences instance for persistent storage
  Preferences preferences;
  
  // Helper methods
  LightProfile interpolateProfiles(LightProfile profile1, LightProfile profile2, float ratio);
  
  // Save and load preferences
  void saveProfilesToPreferences();
  void saveTimeRangesToPreferences();
  void saveModeToPreferences();
  void loadPreferences();
  
public:
  // Constructor
  LedController(
    uint8_t pinRoyalBlue, uint8_t pinBlue, uint8_t pinUV, uint8_t pinViolet,
    uint8_t pinRed, uint8_t pinGreen, uint8_t pinWhite,
    uint8_t channelRoyalBlue, uint8_t channelBlue, uint8_t channelUV, uint8_t channelViolet,
    uint8_t channelRed, uint8_t channelGreen, uint8_t channelWhite,
    uint32_t freq, uint8_t resolution,
    RTC_DS3231* rtc
  );
  
  // Destructor
  ~LedController();
  
  // Initialize LED controller
  void begin();
  
  // Set default light profiles
  void setDefaultProfiles();
  
  // Set custom profiles
  void setMorningProfile(LightProfile profile);
  void setMorningProfile(String jsonProfile);
  void setMiddayProfile(LightProfile profile);
  void setMiddayProfile(String jsonProfile);
  void setEveningProfile(LightProfile profile);
  void setEveningProfile(String jsonProfile);
  void setNightProfile(LightProfile profile);
  void setNightProfile(String jsonProfile);
  
  // JSON methods for profiles
  String getMorningProfileJson();
  String getMiddayProfileJson();
  String getEveningProfileJson();
  String getNightProfileJson();
  LightProfile parseProfileJson(String jsonProfile);
  String profileToJson(LightProfile profile);
  
  // Set time ranges
  void setTimeRanges(int morningStart, int middayStart, int eveningStart, int nightStart);
  String getTimeRangesJson();
  void setTimeRangesFromJson(String jsonTimeRanges);
  
  // Mode control
  void enableManualMode(bool enable);
  bool isInManualMode();
  
  // Set individual LED intensities directly (for manual control)
  void setRoyalBlue(uint8_t intensity);
  void setBlue(uint8_t intensity);
  void setUV(uint8_t intensity);
  void setViolet(uint8_t intensity);
  void setRed(uint8_t intensity);
  void setGreen(uint8_t intensity);
  void setWhite(uint8_t intensity);
  
  // Set all LED intensities at once
  void setAllLeds(uint8_t royalBlue, uint8_t blue, uint8_t uv, uint8_t violet, 
                  uint8_t red, uint8_t green, uint8_t white);
  void setAllLedsFromJson(String jsonProfile);
  
  // Update lighting based on current time (auto mode) or do nothing (manual mode)
  void update();
  
  // Set a specific light profile directly
  void setLightProfile(LightProfile profile);
  void setLightProfileFromJson(String jsonProfile);
  
  // Get current profile based on time
  LightProfile getCurrentProfile();
  String getCurrentProfileJson();
  
  // Print current profile values to Serial
  void printCurrentProfile(LightProfile profile);
  
  // Get current time from RTC as formatted string
  String getCurrentTimeJson();
  
  // Set current time on RTC
  bool setCurrentTime(String timeJson);
  
  // Save all settings to persistent storage
  void saveAllPreferences();
  
  // Off mode control
  void setOffMode(bool off);
  bool isInOffMode();
};

#endif // LED_CONTROLLER_H 
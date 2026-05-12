#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include <Preferences.h>

class Settings {
public:
  void begin();
  
  void save(int alarmHour, int alarmMinute, bool alarmEnabled, bool neopixelOn, const String &scrollText);
  void load(int &alarmHour, int &alarmMinute, bool &alarmEnabled, bool &neopixelOn, String &scrollText);
  
  void saveWiFi(const String &ssid, const String &password);
  void loadWiFi(String &ssid, String &password);
  
  void saveScrollText(const String &text);
  String loadScrollText(const String &defaultText = "Engr. Ene Ijeoma");
  
private:
  Preferences prefs;
  static const char* NAMESPACE;
};

extern Settings settings;

#endif

#include "Settings.h"

const char* Settings::NAMESPACE = "bedbox";
Settings settings;

void Settings::begin() {
  prefs.begin(NAMESPACE, false);
}

void Settings::save(int alarmHour, int alarmMinute, bool alarmEnabled, bool neopixelOn, const String &scrollText) {
  prefs.putUInt("alarmHour", alarmHour);
  prefs.putUInt("alarmMinute", alarmMinute);
  prefs.putBool("alarmEnabled", alarmEnabled);
  prefs.putBool("neopixelOn", neopixelOn);
  prefs.putString("scrollText", scrollText);
}

void Settings::load(int &alarmHour, int &alarmMinute, bool &alarmEnabled, bool &neopixelOn, String &scrollText) {
  alarmHour = prefs.getUInt("alarmHour", 7);
  alarmMinute = prefs.getUInt("alarmMinute", 0);
  alarmEnabled = prefs.getBool("alarmEnabled", false);
  neopixelOn = prefs.getBool("neopixelOn", false);
  scrollText = prefs.getString("scrollText", scrollText);
}

void Settings::saveWiFi(const String &ssid, const String &password) {
  prefs.putString("ssid", ssid);
  prefs.putString("password", password);
}

void Settings::loadWiFi(String &ssid, String &password) {
  ssid = prefs.getString("ssid", "");
  password = prefs.getString("password", "");
}

void Settings::saveScrollText(const String &text) {
  prefs.putString("scrollText", text);
}

String Settings::loadScrollText(const String &defaultText) {
  return prefs.getString("scrollText", defaultText);
}

#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include <Arduino.h>
#include <WiFi.h>

class WiFiSetup {
public:
  void begin(const String &ssid, const String &password);
  bool isConnected() const { return WiFi.status() == WL_CONNECTED; }
  String getIP() const;
  String getAPIP() const;
  
private:
  static const unsigned long CONNECT_TIMEOUT = 10000; // 10 seconds
  static const char* AP_SSID;
  static const char* AP_PASSWORD;
};

extern WiFiSetup wifiSetup;

#endif

#include "WiFiSetup.h"

const char* WiFiSetup::AP_SSID = "adufe_bedbox";
const char* WiFiSetup::AP_PASSWORD = "12345678";

WiFiSetup wifiSetup;

void WiFiSetup::begin(const String &ssid, const String &password) {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  
  if (ssid.length() > 0) {
    Serial.printf("Attempting STA connect to %s\n", ssid.c_str());
    WiFi.begin(ssid.c_str(), password.c_str());
    
    unsigned long start = millis();
    while (millis() - start < CONNECT_TIMEOUT) {
      if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("Connected. IP: %s\n", WiFi.localIP().toString().c_str());
        return;
      }
      delay(250);
      yield();
    }
  }
  
  Serial.println("STA failed, starting AP");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
}

String WiFiSetup::getIP() const {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }
  return "";
}

String WiFiSetup::getAPIP() const {
  return WiFi.softAPIP().toString();
}

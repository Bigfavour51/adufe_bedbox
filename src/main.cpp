#include <Arduino.h>

// Import all libraries
#include "Display.h"
#include "NeoPixel.h"
#include "Button.h"
#include "RTC.h"
#include "Alarm.h"
#include "Settings.h"
#include "WiFiSetup.h"
#include "WebServerSetup.h"

// ===== APPLICATION STATE =====
int alarmHour = 7;
int alarmMinute = 0;
bool alarmEnabled = false;
bool neopixelOn = false;
String scrollText = "Engr. Ene Ijeoma";

// WiFi state
static bool wifiConnected = false;
static bool webServerStarted = false;

// ===== FORWARD DECLARATIONS =====
void onMessageUpdate(const String &msg);
String getScrollText();
String getAlarmStatus();

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("ESP32 BedBox Starting...");

  // Initialize all modules
  display.begin();
  neopixel.begin();
  Button::init(Button::PIN_HOUR, Button::PIN_MINUTE, Button::PIN_ALARM, Button::PIN_NEOPIXEL);
  alarmModule.begin();
  settings.begin();
  rtcModule.begin();

  // Load settings
  settings.load(alarmHour, alarmMinute, alarmEnabled, neopixelOn, scrollText);

  // WiFi setup
  String savedSSID, savedPassword;
  settings.loadWiFi(savedSSID, savedPassword);
  
  if (savedSSID.length() == 0) {
    savedSSID = "ICT";
    savedPassword = "INNOV8HUB";
    settings.saveWiFi(savedSSID, savedPassword);
    Serial.println("Saved default WiFi credentials");
  }
  
  wifiSetup.begin(savedSSID, savedPassword);

  // Display & UI setup
  display.setScrollingText(scrollText.c_str());
  neopixel.setOn(neopixelOn);

  // Setup web server callbacks
  webServer.onMessageUpdate = onMessageUpdate;
  webServer.getScrollText = getScrollText;
  webServer.getAlarmStatus = getAlarmStatus;
}

// ===== LOOP =====
void loop() {
  // WiFi & web server
  bool currentlyConnected = wifiSetup.isConnected();
  
  if (currentlyConnected) {
    if (!wifiConnected) {
      wifiConnected = true;
      Serial.println("WiFi connected!");
      Serial.print("IP address: "); Serial.println(wifiSetup.getIP());
    }
    
    if (!webServerStarted) {
      webServer.begin();
      webServerStarted = true;
      Serial.println("Web server started");
    }
    
    webServer.handleClient();
  } else if (wifiConnected || webServerStarted) {
    Serial.println("WiFi disconnected, restarting AP mode");
    webServerStarted = false;
    wifiConnected = false;
    wifiSetup.begin("", "");
  }

  // Button handling
  if (Button::checkButton(btnHour)) {
    alarmHour = (alarmHour + 1) % 24;
    settings.save(alarmHour, alarmMinute, alarmEnabled, neopixelOn, scrollText);
  }
  
  if (Button::checkButton(btnMinute)) {
    alarmMinute = (alarmMinute + 1) % 60;
    settings.save(alarmHour, alarmMinute, alarmEnabled, neopixelOn, scrollText);
  }
  
  if (Button::checkButton(btnAlarm)) {
    alarmEnabled = !alarmEnabled;
    settings.save(alarmHour, alarmMinute, alarmEnabled, neopixelOn, scrollText);
  }
  
  if (Button::checkButton(btnNeo)) {
    neopixelOn = !neopixelOn;
    neopixel.setOn(neopixelOn);
    settings.save(alarmHour, alarmMinute, alarmEnabled, neopixelOn, scrollText);
  }

  // Get current time from RTC
  TimeData time = rtcModule.getLocalTime();

  // Check alarm trigger
  alarmModule.setHour(alarmHour);
  alarmModule.setMinute(alarmMinute);
  alarmModule.setEnabled(alarmEnabled);
  alarmModule.check(time.hour, time.minute, time.second);
  alarmModule.update();

  // Display update
  if (alarmModule.isActive()) {
    display.displayAlarm();
  } else {
    display.displayClock(time.hour, time.minute, time.second);
  }
  
  display.animate();
}

// ===== CALLBACK FUNCTIONS =====
void onMessageUpdate(const String &msg) {
  if (msg.length() == 0) {
    scrollText = " ";
  } else {
    scrollText = msg;
  }
  
  display.setScrollingText(scrollText.c_str());
  settings.saveScrollText(scrollText);
}

String getScrollText() {
  return scrollText;
}

String getAlarmStatus() {
  String status = String(alarmHour) + ":";
  if (alarmMinute < 10) status += "0";
  status += String(alarmMinute);
  
  if (alarmEnabled) {
    status += " (enabled)";
  } else {
    status += " (disabled)";
  }
  
  return status;
}
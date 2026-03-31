#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>
#include <Wire.h>
#include <RTClib.h>

// ===== HARDWARE CONFIG =====
// MAX7219
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 8t
#define CS_PIN 5

MD_Parola display = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// Buttons (pullup, active LOW)
#define BTN_HOUR 32
#define BTN_MINUTE 33
#define BTN_ALARM 27
#define BTN_NEOPIXEL 26

// NeoPixel
#define NEOPIXEL_PIN 16
#define NEOPIXEL_COUNT 27
#define NEOPIXEL_BRIGHTNESS 20

// Buzzer
#define BUZZER_PIN 25

// WiFi defaults
const char* default_ssid = "ICT";
const char* default_password = "INNOV8HUB";
char saved_ssid[64] = "";
char saved_password[64] = "";

WebServer server(80);

// Lagos timezone UTC+1
const int TIMEZONE_OFFSET_HOURS = 1;

// RTC
RTC_DS3231 rtc;

// ===== STATE =====
int alarmHour = 7;
int alarmMinute = 0;
bool alarmEnabled = false;

bool neopixelOn = false;
String scrollText = "Engr. Ene Ijeoma";

bool alarmActive = false;
unsigned long alarmStartMillis = 0;
unsigned long buzzerToggleMillis = 0;
bool buzzerState = false;

int lastHour = -1;
int lastMinute = -1;
int lastSecond = -1;

struct ButtonState {
  uint8_t pin;
  bool lastRead;
  bool stableState;
  unsigned long lastDebounce;
};

ButtonState btnHour = {BTN_HOUR, HIGH, HIGH, 0};
ButtonState btnMinute = {BTN_MINUTE, HIGH, HIGH, 0};
ButtonState btnAlarm = {BTN_ALARM, HIGH, HIGH, 0};
ButtonState btnNeo = {BTN_NEOPIXEL, HIGH, HIGH, 0};

// NeoPixel
Adafruit_NeoPixel strip(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
Preferences prefs;

// ===== FORWARD DECLARATIONS =====
void setupWiFi();
void setupWebServer();
void setupRTC();
void setScrollingText();
void updateNeopixel();
bool checkButton(ButtonState &btn);
void saveSettings();
void startAlarm();
void stopAlarm();

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("ESP32 BedBox Starting...");

  // Display init with zones
  display.begin(1); // 2 zones
  display.setZone(0, 0, 3); // Clock → first 4 modules
  display.setZone(1, 4, 7); // Message → last 4 modules
  display.setIntensity(5);
  display.displayClear();

  // NeoPixel
  strip.begin();
  strip.setBrightness(NEOPIXEL_BRIGHTNESS);
  strip.show();

  // Buttons
  pinMode(BTN_HOUR, INPUT_PULLUP);
  pinMode(BTN_MINUTE, INPUT_PULLUP);
  pinMode(BTN_ALARM, INPUT_PULLUP);
  pinMode(BTN_NEOPIXEL, INPUT_PULLUP);

  // Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Preferences
  prefs.begin("bedbox", false);
  alarmHour = prefs.getUInt("alarmHour", 7);
  alarmMinute = prefs.getUInt("alarmMinute", 0);
  alarmEnabled = prefs.getBool("alarmEnabled", false);
  neopixelOn = prefs.getBool("neopixelOn", false);
  scrollText = prefs.getString("scrollText", scrollText);

  // WiFi
  String ssidS = prefs.getString("ssid", "");
  String passS = prefs.getString("password", "");
  if (ssidS.length() == 0) {
    prefs.putString("ssid", default_ssid);
    prefs.putString("password", default_password);
    ssidS = default_ssid;
    passS = default_password;
    Serial.println("Saved default WiFi credentials");
  }
  ssidS.toCharArray(saved_ssid, sizeof(saved_ssid));
  passS.toCharArray(saved_password, sizeof(saved_password));

  // Start scrolling message
  setScrollingText();
  updateNeopixel();

  // I2C for RTC
  Wire.begin(21, 22);

  // WiFi
  setupWiFi();

  // RTC
  setupRTC();

  DateTime now = rtc.now();
  Serial.printf("RTC initialized to: %04d-%02d-%02d %02d:%02d:%02d\n", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
}

// ===== LOOP =====
void loop() {
  static bool wifiConnected = false;
  static bool webServerStarted = false;

  // WiFi & web server
  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiConnected) {
      wifiConnected = true;
      Serial.println("WiFi connected!");
      Serial.print("IP address: "); Serial.println(WiFi.localIP());
    }
    if (!webServerStarted) {
      setupWebServer();
      webServerStarted = true;
      Serial.println("Web server started");
    }
    server.handleClient();
  } else if (wifiConnected || webServerStarted) {
    Serial.println("WiFi disconnected, restarting AP mode");
    webServerStarted = false;
    wifiConnected = false;
    setupWiFi();
  }

  // Buttons
  if (checkButton(btnHour)) { alarmHour = (alarmHour + 1) % 24; saveSettings(); }
  if (checkButton(btnMinute)) { alarmMinute = (alarmMinute + 1) % 60; saveSettings(); }
  if (checkButton(btnAlarm)) { alarmEnabled = !alarmEnabled; saveSettings(); }
  if (checkButton(btnNeo)) { neopixelOn = !neopixelOn; saveSettings(); updateNeopixel(); }

  // Clock
  DateTime nowDT = rtc.now();
  DateTime localDT = nowDT + TimeSpan(TIMEZONE_OFFSET_HOURS * 3600);
  int hour = localDT.hour();
  int minute = localDT.minute();
  int second = localDT.second();

  // Alarm trigger
  if (alarmEnabled && !alarmActive && hour == alarmHour && minute == alarmMinute && second == 0) {
    startAlarm();
  }

  // Alarm active
  if (alarmActive) {
    unsigned long nowMillis = millis();
    if (nowMillis - alarmStartMillis >= 60000) stopAlarm();
    else if (nowMillis - buzzerToggleMillis >= 200) {
      buzzerToggleMillis = nowMillis;
      buzzerState = !buzzerState;
      digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
    }
    if (!display.displayAnimate()) {
      display.displayZoneText(0, "ALARM", PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
    }
  } else {
    bool colon = (second % 2 == 0);
    if (hour != lastHour || minute != lastMinute || second != lastSecond) {
      lastHour = hour;
      lastMinute = minute;
      lastSecond = second;

      char timeStr[6];
      sprintf(timeStr, colon ? "%02d:%02d" : "%02d %02d", hour, minute);
      display.displayZoneText(0, timeStr, PA_CENTER, 50, 0, PA_PRINT, PA_NO_EFFECT);
    }
    display.displayAnimate();
  }

  // Animate message zone
  if (display.displayAnimate()) display.displayReset();
}

// ===== FUNCTIONS =====
void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  if (strlen(saved_ssid) > 0) {
    Serial.printf("Attempting STA connect to %s\n", saved_ssid);
    WiFi.begin(saved_ssid, saved_password);
    unsigned long start = millis();
    while (millis() - start < 10000) {
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
  WiFi.softAP("adufe_bedbox", "12345678");
  IPAddress apIP(192,168,4,1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
  Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
}

void setupWebServer() {
  server.on("/", HTTP_GET, [](){
    String html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>BedBox Config</title></head><body>";
    html += "<h1>BedBox Display Config</h1>";
    html += "<p>Scrolling text: " + scrollText + "</p>";
    html += "<p>Alarm: " + String(alarmHour) + ":" + (alarmMinute<10?"0":"") + String(alarmMinute)
            + (alarmEnabled?" (enabled)":" (disabled)") + "</p>";
    html += "<form method=\"POST\" action=\"/set\">";
    html += "Message: <input name=\"message\" type=\"text\" value=\"" + scrollText + "\" size=\"22\"><br><br>";
    html += "<input type=\"submit\" value=\"Save message\">";
    html += "</form></body></html>";
    server.send(200, "text/html", html);
  });
  server.on("/set", HTTP_POST, [](){
    if (server.hasArg("message")) {
      String msg = server.arg("message");
      if (msg.length() == 0) msg = " ";
      scrollText = msg;
      prefs.putString("scrollText", scrollText);
      setScrollingText();
    }
    server.sendHeader("Location","/",true);
    server.send(303,"text/plain","");
  });
  server.onNotFound([](){ server.send(404,"text/plain","Not Found"); });
  server.begin();
}

void setupRTC() {
  if (!rtc.begin()) { Serial.println("RTC DS3231 not found"); while(1); }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  Serial.println("RTC initialized to compile-time");
}

void setScrollingText() {
  display.displayZoneText(1, scrollText.c_str(), PA_LEFT, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
}

void updateNeopixel() {
  if (neopixelOn) {
    for (int i=0;i<NEOPIXEL_COUNT;i++) strip.setPixelColor(i, strip.Color(200,150,0));
  } else {
    for (int i=0;i<NEOPIXEL_COUNT;i++) strip.setPixelColor(i,0);
  }
  strip.show();
}

void saveSettings() {
  prefs.putUInt("alarmHour", alarmHour);
  prefs.putUInt("alarmMinute", alarmMinute);
  prefs.putBool("alarmEnabled", alarmEnabled);
  prefs.putBool("neopixelOn", neopixelOn);
  prefs.putString("scrollText", scrollText);
}

bool checkButton(ButtonState &btn) {
  bool raw = digitalRead(btn.pin);
  unsigned long now = millis();
  if (raw != btn.lastRead) { btn.lastDebounce = now; btn.lastRead = raw; }
  if ((now - btn.lastDebounce) > 50) {
    if (raw != btn.stableState) {
      btn.stableState = raw;
      if (!btn.stableState) return true;
    }
  }
  return false;
}

void startAlarm() {
  alarmActive = true;
  alarmStartMillis = millis();
  buzzerToggleMillis = millis();
  buzzerState = false;
  digitalWrite(BUZZER_PIN, HIGH);
  Serial.println("Alarm triggered!");
  display.displayZoneText(0, "ALARM", PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
}

void stopAlarm() {
  alarmActive = false;
  digitalWrite(BUZZER_PIN, LOW);
  buzzerState = false;
  lastMinute = -1; // force clock refresh
  Serial.println("Alarm stopped");
}
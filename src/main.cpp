#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>
#include <time.h>

// ----- PIN CONFIG -----

// MAX7219 chains
#define MAX_DEVICES 4
#define CLK_PIN 18
#define DIN_PIN 23
#define CS_CLOCK 5
#define CS_MESSAGE 17

// Buttons (pullup, active LOW)
#define BTN_HOUR 32
#define BTN_MINUTE 33
#define BTN_ALARM 27
#define BTN_NEOPIXEL 26

// NeoPixel
#define NEOPIXEL_PIN 16
#define NEOPIXEL_COUNT 27

// Buzzer (active HIGH)
#define BUZZER_PIN 25

// ----- WIFI / TIME -----
const char* default_ssid = "ICT";
const char* default_password = "INNOV8HUB";
char saved_ssid[64] = "";
char saved_password[64] = "";

const long  gmtOffsetSec = 0;
const int   daylightOffsetSec = 0;

WebServer server(80);

// ----- displays -----
MD_Parola clockDisplay(MD_MAX72XX::FC16_HW, CS_CLOCK, MAX_DEVICES);
MD_Parola msgDisplay(MD_MAX72XX::FC16_HW, CS_MESSAGE, MAX_DEVICES);

Adafruit_NeoPixel strip(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
Preferences prefs;

// ----- state -----
int alarmHour = 7;
int alarmMinute = 0;
bool alarmEnabled = false;

bool neopixelOn = false;
String scrollText = "Engr. Ene Ijeoma";

bool alarmActive = false;
unsigned long alarmStartMillis = 0;
unsigned long buzzerToggleMillis = 0;
bool buzzerState = false;

unsigned long lastClockUpdateMillis = 0;

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

// ----- forward declarations -----
void setupWiFi();
void setupWebServer();
void setupNTPTime();
void setScrollingText();

void handleRoot();
void handleSetMessage();
void updateNeopixel(); 
bool checkButton(ButtonState &btn);
void saveSettings();
void refreshClockDisplay(struct tm* currentTime);
void startAlarm();
void stopAlarm();

void setup()
{
  Serial.begin(115200);
  delay(100);

  // display init
  clockDisplay.begin();
  clockDisplay.setIntensity(2);
  clockDisplay.displayClear();

  msgDisplay.begin();
  msgDisplay.setIntensity(2);
  msgDisplay.displayClear();

  // buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // buttons
  pinMode(BTN_HOUR, INPUT_PULLUP);
  pinMode(BTN_MINUTE, INPUT_PULLUP);
  pinMode(BTN_ALARM, INPUT_PULLUP);
  pinMode(BTN_NEOPIXEL, INPUT_PULLUP);

  // NeoPixel
  strip.begin();
  strip.show();

  // settings
  prefs.begin("bedbox", false);
  alarmHour = prefs.getUInt("alarmHour", 7);
  alarmMinute = prefs.getUInt("alarmMinute", 0);
  alarmEnabled = prefs.getBool("alarmEnabled", false);
  neopixelOn = prefs.getBool("neopixelOn", false);
  scrollText = prefs.getString("scrollText", scrollText);

  // Load WiFi credentials, save defaults if not exist
  String ssidS = prefs.getString("ssid", "");
  String passS = prefs.getString("password", "");
  if (ssidS.length() == 0) {
    // First run - save defaults
    prefs.putString("ssid", default_ssid);
    prefs.putString("password", default_password);
    ssidS = default_ssid;
    passS = default_password;
    Serial.println("Saved default WiFi credentials");
  }
  ssidS.toCharArray(saved_ssid, sizeof(saved_ssid));
  passS.toCharArray(saved_password, sizeof(saved_password));

  updateNeopixel();
  setScrollingText();

  setupWiFi();
  setupWebServer();

  // initialize time from NTP with retry
  setupNTPTime();
}


void loop() {
  server.handleClient();

  unsigned long now = millis();

  if (checkButton(btnHour)) {
    alarmHour = (alarmHour + 1) % 24;
    saveSettings();
  }
  if (checkButton(btnMinute)) {
    alarmMinute = (alarmMinute + 1) % 60;
    saveSettings();
  }
  if (checkButton(btnAlarm)) {
    alarmEnabled = !alarmEnabled;
    saveSettings();
  }
  if (checkButton(btnNeo)) {
    neopixelOn = !neopixelOn;
    saveSettings();
    updateNeopixel();
  }

  struct tm currentTime;
  if (getLocalTime(&currentTime, 1000)) {
    if (!alarmActive && (now - lastClockUpdateMillis >= 1000)) {
      refreshClockDisplay(&currentTime);
      lastClockUpdateMillis = now;
    }

    if (alarmEnabled && !alarmActive
        && currentTime.tm_hour == alarmHour
        && currentTime.tm_min == alarmMinute
        && currentTime.tm_sec == 0) {
      startAlarm();
    }

    if (alarmActive) {
      if (now - alarmStartMillis >= 60000) {
        stopAlarm();
      } else {
        if (now - buzzerToggleMillis >= 200) {
          buzzerToggleMillis = now;
          buzzerState = !buzzerState;
          digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
        }
        if (!clockDisplay.displayAnimate()) {
          clockDisplay.displayText("ALARM", PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        }
      }
    }
  }

  if (alarmActive == false) {
    // keep clock display running and not in alarm mode
    clockDisplay.displayAnimate();
  }

  msgDisplay.displayAnimate();
}

void setupWiFi() {
  WiFi.mode(WIFI_STA);

  if (strlen(saved_ssid) > 0) {
    Serial.printf("Connecting to STA %s\n", saved_ssid);
    WiFi.begin(saved_ssid, saved_password);
    unsigned long startAttemptTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 15000) {
      delay(200);
      Serial.print(".");
    }
    Serial.println();
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Connected. IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("WiFi STA failed, starting AP mode");
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("adufe_bedbox", "12345678");
    IPAddress apIP(192,168,4,1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
    Serial.printf("AP IP: %s\n", WiFi.softAPIP().toString().c_str());
  }
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/set", HTTP_POST, handleSetMessage);
  server.onNotFound([]() { server.send(404, "text/plain", "Not Found"); });
  server.begin();
}

void setupNTPTime() {
  // Configure NTP with two servers (ESP32 configTime supports up to 3 args)
  configTime(gmtOffsetSec, daylightOffsetSec,
             "pool.ntp.org",
             "time.nist.gov");

  Serial.println("Attempting NTP synchronization...");

  // Try multiple times with increasing timeout
  for (int attempt = 1; attempt <= 3; attempt++) {
    Serial.printf("NTP attempt %d/3...\n", attempt);

    struct tm timeInfo;
    if (getLocalTime(&timeInfo, 10000)) {  // 10 second timeout
      Serial.println("NTP synchronization successful!");
      Serial.printf("Current time: %02d:%02d:%02d\n",
                   timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
      return;
    }

    Serial.printf("NTP attempt %d failed, retrying...\n", attempt);
    delay(2000);  // Wait 2 seconds before retry
  }

  Serial.println("All NTP attempts failed. Clock will use ESP32 internal time.");
  Serial.println("Time will be inaccurate until NTP connection is restored.");

  // Set a default time if NTP completely fails
  struct tm defaultTime = {0};
  defaultTime.tm_year = 124;  // 2024 - 1900
  defaultTime.tm_mon = 0;     // January
  defaultTime.tm_mday = 1;    // 1st
  defaultTime.tm_hour = 12;   // Noon
  defaultTime.tm_min = 0;
  defaultTime.tm_sec = 0;
  time_t defaultTimeT = mktime(&defaultTime);
  struct timeval tv = {defaultTimeT, 0};
  settimeofday(&tv, NULL);

  Serial.println("Set default time: 2024-01-01 12:00:00");
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><title>BedBox Config</title></head><body>";
  html += "<h1>BedBox Display Config</h1>";
  html += "<p>Scrolling text: " + scrollText + "</p>";
  html += "<p>Alarm: " + String(alarmHour) + ":" + (alarmMinute < 10 ? "0" : "") + String(alarmMinute)
          + (alarmEnabled ? " (enabled)" : " (disabled)") + "</p>";
  html += "<form method=\"POST\" action=\"/set\">";
  html += "Message: <input name=\"message\" type=\"text\" value=\"" + scrollText + "\" size=\"22\"><br><br>";
  html += "<input type=\"submit\" value=\"Save message\">";
  html += "</form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleSetMessage() {
  if (server.hasArg("message")) {
    String msg = server.arg("message");
    if (msg.length() == 0) {
      msg = " ";
    }
    scrollText = msg;
    prefs.putString("scrollText", scrollText);
    setScrollingText();
  }
  server.sendHeader("Location", "/", true);
  server.send(303, "text/plain", "");
}

void setScrollingText() {
  msgDisplay.displayText(scrollText.c_str(), PA_LEFT, 25, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
}

void refreshClockDisplay(struct tm* currentTime) {
  char timeString[9];
  bool colon = ((currentTime->tm_sec % 2) == 0);
  snprintf(timeString, sizeof(timeString), "%02d%c%02d", currentTime->tm_hour, colon ? ':' : ' ', currentTime->tm_min);

  clockDisplay.displayText(timeString, PA_CENTER, 0, 0, PA_PRINT, PA_PRINT);
}

void startAlarm() {
  alarmActive = true;
  alarmStartMillis = millis();
  buzzerToggleMillis = millis();
  buzzerState = false;
  digitalWrite(BUZZER_PIN, HIGH);
  Serial.println("Alarm triggered!");
  clockDisplay.displayText("ALARM", PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
}

void stopAlarm() {
  alarmActive = false;
  digitalWrite(BUZZER_PIN, LOW);
  buzzerState = false;
  Serial.println("Alarm stopped");
}

void updateNeopixel() {
  if (neopixelOn) {
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
      strip.setPixelColor(i, strip.Color(255, 255, 255));
    }
  } else {
    for (int i = 0; i < NEOPIXEL_COUNT; i++) {
      strip.setPixelColor(i, 0);
    }
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

  if (raw != btn.lastRead) {
    btn.lastDebounce = now;
    btn.lastRead = raw;
  }

  if ((now - btn.lastDebounce) > 50) {
    if (raw != btn.stableState) {
      btn.stableState = raw;
      if (btn.stableState == LOW) {
        return true;
      }
    }
  }

  return false;
}

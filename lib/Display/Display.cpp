#include "Display.h"

Display display;

Display::Display() : parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES) {}

void Display::begin() {
  parola.begin(1); // 2 zones
  parola.setZone(0, 0, 3); // Clock → first 4 modules
  parola.setZone(1, 4, 7); // Message → last 4 modules
  parola.setIntensity(5);
  parola.displayClear();
}

void Display::displayClock(int hour, int minute, int second) {
  static int lastHour = -1;
  static int lastMinute = -1;
  static int lastSecond = -1;
  
  bool colon = (second % 2 == 0);
  
  if (hour != lastHour || minute != lastMinute || second != lastSecond) {
    lastHour = hour;
    lastMinute = minute;
    lastSecond = second;
    
    char timeStr[6];
    sprintf(timeStr, colon ? "%02d:%02d" : "%02d %02d", hour, minute);
    parola.displayZoneText(0, timeStr, PA_CENTER, 50, 0, PA_PRINT, PA_NO_EFFECT);
  }
  parola.displayAnimate();
}

void Display::displayAlarm() {
  if (!parola.displayAnimate()) {
    parola.displayZoneText(0, "ALARM", PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  }
}

void Display::setScrollingText(const char* text) {
  parola.displayZoneText(1, text, PA_LEFT, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
}

void Display::animate() {
  if (parola.displayAnimate()) parola.displayReset();
}

#include "Button.h"

ButtonState btnHour = {Button::PIN_HOUR, HIGH, HIGH, 0};
ButtonState btnMinute = {Button::PIN_MINUTE, HIGH, HIGH, 0};
ButtonState btnAlarm = {Button::PIN_ALARM, HIGH, HIGH, 0};
ButtonState btnNeo = {Button::PIN_NEOPIXEL, HIGH, HIGH, 0};

void Button::init(uint8_t hour, uint8_t minute, uint8_t alarm, uint8_t neo) {
  pinMode(hour, INPUT_PULLUP);
  pinMode(minute, INPUT_PULLUP);
  pinMode(alarm, INPUT_PULLUP);
  pinMode(neo, INPUT_PULLUP);
}

bool Button::checkButton(ButtonState &btn) {
  bool raw = digitalRead(btn.pin);
  unsigned long now = millis();
  
  if (raw != btn.lastRead) {
    btn.lastDebounce = now;
    btn.lastRead = raw;
  }
  
  if ((now - btn.lastDebounce) > 50) {
    if (raw != btn.stableState) {
      btn.stableState = raw;
      if (!btn.stableState) return true;
    }
  }
  return false;
}

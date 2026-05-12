#ifndef BUTTON_H
#define BUTTON_H

#include <Arduino.h>

struct ButtonState {
  uint8_t pin;
  bool lastRead;
  bool stableState;
  unsigned long lastDebounce;
};

class Button {
public:
  static void init(uint8_t btnHour, uint8_t btnMinute, uint8_t btnAlarm, uint8_t btnNeo);
  static bool checkButton(ButtonState &btn);
  
  static const uint8_t PIN_HOUR = 32;
  static const uint8_t PIN_MINUTE = 33;
  static const uint8_t PIN_ALARM = 27;
  static const uint8_t PIN_NEOPIXEL = 26;
};

extern ButtonState btnHour, btnMinute, btnAlarm, btnNeo;

#endif

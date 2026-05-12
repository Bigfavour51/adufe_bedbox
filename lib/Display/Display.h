#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

class Display {
public:
  Display();
  void begin();
  void displayClock(int hour, int minute, int second);
  void displayAlarm();
  void setScrollingText(const char* text);
  void animate();
  
private:
  MD_Parola parola;
  static const MD_MAX72XX::moduleType_t HARDWARE_TYPE = MD_MAX72XX::FC16_HW;
  static const uint8_t MAX_DEVICES = 8;
  static const uint8_t CS_PIN = 5;
};

extern Display display;

#endif

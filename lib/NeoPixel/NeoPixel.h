#ifndef NEOPIXEL_H
#define NEOPIXEL_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

class NeoPixelManager {
public:
  NeoPixelManager();
  void begin();
  void setOn(bool on);
  bool isOn() const { return on; }
  
private:
  Adafruit_NeoPixel strip;
  bool on;
  static const uint8_t PIN = 16;
  static const uint8_t COUNT = 27;
  static const uint8_t BRIGHTNESS = 20;
};

extern NeoPixelManager neopixel;

#endif

#include "NeoPixel.h"

NeoPixelManager neopixel;

NeoPixelManager::NeoPixelManager() 
  : strip(COUNT, PIN, NEO_GRB + NEO_KHZ800), on(false) {}

void NeoPixelManager::begin() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
}

void NeoPixelManager::setOn(bool state) {
  on = state;
  
  if (on) {
    for (int i = 0; i < COUNT; i++) {
      strip.setPixelColor(i, strip.Color(200, 150, 0));
    }
  } else {
    for (int i = 0; i < COUNT; i++) {
      strip.setPixelColor(i, 0);
    }
  }
  strip.show();
}

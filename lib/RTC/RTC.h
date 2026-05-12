#ifndef RTC_MODULE_H
#define RTC_MODULE_H

#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>

struct TimeData {
  int hour;
  int minute;
  int second;
};

class RTCModule {
public:
  void begin();
  TimeData getLocalTime();
  
private:
  RTC_DS3231 rtc;
  static const int TIMEZONE_OFFSET_HOURS = 1; // Lagos UTC+1
  static const uint8_t SDA_PIN = 21;
  static const uint8_t SCL_PIN = 22;
};

extern RTCModule rtcModule;

#endif

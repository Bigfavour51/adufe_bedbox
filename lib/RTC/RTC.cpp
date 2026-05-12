#include "RTC.h"

RTCModule rtcModule;

void RTCModule::begin() {
  Wire.begin(SDA_PIN, SCL_PIN);
  
  if (!rtc.begin()) {
    Serial.println("RTC DS3231 not found");
    while(1);
  }
  
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
  DateTime now = rtc.now();
  Serial.printf("RTC initialized to: %04d-%02d-%02d %02d:%02d:%02d\n", 
    now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
}

TimeData RTCModule::getLocalTime() {
  DateTime nowDT = rtc.now();
  DateTime localDT = nowDT + TimeSpan(TIMEZONE_OFFSET_HOURS * 3600);
  
  TimeData time;
  time.hour = localDT.hour();
  time.minute = localDT.minute();
  time.second = localDT.second();
  
  return time;
}

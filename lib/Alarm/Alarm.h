#ifndef ALARM_H
#define ALARM_H

#include <Arduino.h>

class Alarm {
public:
  void begin();
  void check(int hour, int minute, int second);
  void update();
  void stop();
  
  void setHour(int h) { alarmHour = h; }
  void setMinute(int m) { alarmMinute = m; }
  void setEnabled(bool e) { alarmEnabled = e; }
  
  int getHour() const { return alarmHour; }
  int getMinute() const { return alarmMinute; }
  bool isEnabled() const { return alarmEnabled; }
  bool isActive() const { return alarmActive; }
  
private:
  int alarmHour = 7;
  int alarmMinute = 0;
  bool alarmEnabled = false;
  
  bool alarmActive = false;
  unsigned long alarmStartMillis = 0;
  unsigned long buzzerToggleMillis = 0;
  bool buzzerState = false;
  
  static const uint8_t BUZZER_PIN = 25;
  static const unsigned long ALARM_DURATION = 60000; // 1 minute
  static const unsigned long BUZZER_TOGGLE_INTERVAL = 200; // 200ms
};

extern Alarm alarmModule;

#endif

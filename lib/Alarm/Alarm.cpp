#include "Alarm.h"

Alarm alarmModule;

void Alarm::begin() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
}

void Alarm::check(int hour, int minute, int second) {
  if (alarmEnabled && !alarmActive && hour == alarmHour && minute == alarmMinute && second == 0) {
    alarmActive = true;
    alarmStartMillis = millis();
    buzzerToggleMillis = millis();
    buzzerState = false;
    digitalWrite(BUZZER_PIN, HIGH);
    Serial.println("Alarm triggered!");
  }
}

void Alarm::update() {
  if (!alarmActive) return;
  
  unsigned long nowMillis = millis();
  
  if (nowMillis - alarmStartMillis >= ALARM_DURATION) {
    stop();
  } else if (nowMillis - buzzerToggleMillis >= BUZZER_TOGGLE_INTERVAL) {
    buzzerToggleMillis = nowMillis;
    buzzerState = !buzzerState;
    digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
  }
}

void Alarm::stop() {
  alarmActive = false;
  digitalWrite(BUZZER_PIN, LOW);
  buzzerState = false;
  Serial.println("Alarm stopped");
}

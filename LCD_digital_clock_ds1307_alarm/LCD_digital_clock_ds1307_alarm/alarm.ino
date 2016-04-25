#include "EEPROM.h"
boolean enable_alarm;
#define ALARM_PIN   17 //Analog 3 
unsigned long alarm_startTime = 0;

//////////////////////////////////////////////////////////////////////////
//  read alarm settings from eeprom
//////////////////////////////////////////////////////////////////////////
tmElements_t alarm_read_eeprom() {
  tmElements_t tm_alarm;
  enable_alarm = EEPROM.read(0);
  tm_alarm.Hour = EEPROM.read(1);
  tm_alarm.Minute = EEPROM.read(2);
  tm_alarm.Second = EEPROM.read(3);
  if (tm_alarm.Hour >= 24) { //alarm not set yet
    tm_alarm.Hour = 0;
    tm_alarm.Minute = 0;
    tm_alarm.Second = 0;
  }
  return  tm_alarm;
}

//////////////////////////////////////////////////////////////////////////
//  write alarm settings from eeprom
//////////////////////////////////////////////////////////////////////////
void alarm_write_eeprom(tmElements_t tm_alarm) {
  EEPROM.write(1, tm_alarm.Hour);
  EEPROM.write(2, tm_alarm.Minute);
  EEPROM.write(3, tm_alarm.Second);
}

//////////////////////////////////////////////////////////////////////////
// show alarm time on display (right corner)
//////////////////////////////////////////////////////////////////////////
void show_alarmtime() {
  if (enable_alarm) {
    lcd.setCursor(11, FIRST_LINE);
    lcd.print(int2str(tm_alarm.Hour));
    lcd.print(":");
    lcd.print(int2str(tm_alarm.Minute));
    lcd.setCursor(14, SECOND_LINE);
    lcd.print(int2str(tm_alarm.Second));
  }
}

//////////////////////////////////////////////////////////////////////////
// show alarm time on display
//////////////////////////////////////////////////////////////////////////
void show_alarmtime(byte pos_y) {
  lcd.setCursor(0, SECOND_LINE);
  lcd.print(int2str(tm_alarm.Hour));
  lcd.print(":");
  lcd.print(int2str(tm_alarm.Minute));
  lcd.print(":");
  lcd.print(int2str(tm_alarm.Second));
}

//////////////////////////////////////////////////////////////////////////
//  disable/enabled alarm
//////////////////////////////////////////////////////////////////////////
void toggle_alarm_mode() {
  lcd.clear();
  if (enable_alarm) {
    enable_alarm = false;
    DEBUG_PRINTLN("Alarm Disabled");
    lcd.print("Alarm Disabled");
  } else {
    enable_alarm = true;
    DEBUG_PRINTLN("Alarm Enabled");
    lcd.print("Alarm Enabled");
  }
  EEPROM.write(0, enable_alarm);
  delay(2000);
  lcd.clear();
}

//////////////////////////////////////////////////////////////////////////
// trigger alarm if alarm time = current time
//////////////////////////////////////////////////////////////////////////
void alarm(tmElements_t tm, tmElements_t tm_alarm) {
  if (enable_alarm) {
    // comparing tmElements_t is not allowed
    // tmElements_t must convert to time_t before comparing
    time_t t_time, t_alarm;
    t_time = makeTime(tm); //convert tmElements_t element to time_t variable
    t_alarm = makeTime(tm_alarm);

    if (t_time == t_alarm) {
      DEBUG_PRINTLN("Alarm ON");
      switch_alarm(HIGH);
      alarm_startTime = millis();
    }
  }
}

//////////////////////////////////////////////////////////////////////////
// turn ON/OFF alarm
//////////////////////////////////////////////////////////////////////////
void switch_alarm(boolean state) {
  pinMode(ALARM_PIN, OUTPUT);
  digitalWrite(ALARM_PIN, state);
}

//////////////////////////////////////////////////////////////////////////
// alarm state
//////////////////////////////////////////////////////////////////////////
boolean alarm_state() {
  return digitalRead(ALARM_PIN);
  /*
    if (digitalRead(ALARM_PIN) == HIGH) {
    return true;
    } else {
    return false;
    }
  */
}

//////////////////////////////////////////////////////////////////////////
// auto turn off alarm
//////////////////////////////////////////////////////////////////////////
void auto_off_alarm(int seconds) {
  if (alarm_state() == HIGH) {
    if (millis() >= alarm_startTime + seconds * 1000) {
      DEBUG_PRINTLN("Alarm OFF");
      switch_alarm(LOW);
    }
  }
}



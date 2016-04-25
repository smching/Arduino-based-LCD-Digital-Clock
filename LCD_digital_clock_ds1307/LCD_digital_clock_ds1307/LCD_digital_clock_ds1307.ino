/*
  DS1307 RTC LCD digital clock
  Compatible with the Arduino IDE 1.0 and 1.6.8
  By SM Ching (http://ediy.com.my)
*/

#define DEBUG //comment this line to disable Serial.print() & Serial.println()
#ifdef DEBUG
#define DEBUG_SERIAL_BEGIN(x) Serial.begin(x);
#define DEBUG_PRINT(x)  Serial.print(x)
#define DEBUG_PRINTLN(x)  Serial.println(x)
#else
#define DEBUG_SERIAL_BEGIN(x)
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

#include "Time.h" // https://github.com/PaulStoffregen/Time
#include "LiquidCrystal.h" // control LCDs based on the Hitachi HD44780
#include "DS1307RTC.h" // http://www.pjrc.com/teensy/td_libs_DS1307RTC.html
#include "Wire.h"

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);  // select the pins used on the LCD panel

// define some values used by the panel and buttons
#define btnNONE   0
#define btnRIGHT  1
#define btnUP     2
#define btnDOWN   3
#define btnLEFT   4
#define btnSELECT 5

#define FIRST_LINE 0 //text position for first line
#define SECOND_LINE 1 //text position for second line

tmElements_t tm;
unsigned long previousMillis = 0;   //hold the current millis

//////////////////////////////////////////////////////////////////////////
// the setup routine runs once when you press reset
//////////////////////////////////////////////////////////////////////////
void setup() {
  DEBUG_SERIAL_BEGIN(9600);
  DEBUG_PRINTLN("DS1307 RTC Clock");
  lcd.begin(16, 2);  // start the library
}

//////////////////////////////////////////////////////////////////////////
// the loop routine runs over and over again forever
//////////////////////////////////////////////////////////////////////////
byte previous_ss;
void loop() {
  if (RTC.read(tm)) {
    if (previous_ss != tm.Second) { //if current seconds diff. from previous seconds
      previous_ss = tm.Second;
      show_time(FIRST_LINE);
      show_date(SECOND_LINE);
      DEBUG_PRINT(dateStr(tm));
      DEBUG_PRINT(" ");
      DEBUG_PRINTLN(timeStr(tm));
    }
  } else {
    lcd.clear();
    if (RTC.chipPresent()) {
      DEBUG_PRINTLN("The DS1307 is stopped.  Please run the SetTime");
      DEBUG_PRINTLN("example to initialize the time and begin running.");
      DEBUG_PRINTLN();
      lcd.print("DS1307 stopped");
      lcd.setCursor(0, SECOND_LINE);
      lcd.print("Run SetTime");
    } else {
      DEBUG_PRINTLN("DS1307 read error!  Please check the circuitry.");
      DEBUG_PRINTLN();
      lcd.print("DS1307 error");
      lcd.setCursor(0, SECOND_LINE);
      lcd.print("Check circuitry");
    }
    delay(1000);
    lcd.clear();
  }
  String input = serial_input();
  if (input.length() > 0) save_settings(input);
  button_input();
}

//////////////////////////////////////////////////////////////////////////
// show time on display
//////////////////////////////////////////////////////////////////////////
void show_time(byte pos_y) {
  lcd.setCursor(0, pos_y);
  lcd.print(int2str(tm.Hour));
  lcd.print(":");
  lcd.print(int2str(tm.Minute));
  lcd.print(":");
  lcd.print(int2str(tm.Second));
}

//////////////////////////////////////////////////////////////////////////
// show date on display
//////////////////////////////////////////////////////////////////////////
void show_date(byte pos_y) {
  lcd.setCursor(0, pos_y);
  lcd.print(String(tmYearToCalendar(tm.Year)));
  lcd.print("-");
  lcd.print(int2str(tm.Month));
  lcd.print("-");
  lcd.print(int2str(tm.Day));
}

//////////////////////////////////////////////////////////////////////////
// return incomingString if data available in serial port
//////////////////////////////////////////////////////////////////////////
String serial_input() {
  String incomingString = "";
  while (Serial.available()) { // check if there's incoming serial data
    // read a byte from the serial buffer.
    char incomingByte = Serial.read();
    delay(1); //for stability
    incomingString += incomingByte;
  }
  return incomingString;
}

//////////////////////////////////////////////////////////////////////////
// set date: save_settings(Dyymmdd) eg. save_settings(D160131)
// set time: save_settings(Thhnnss) eg. save_settings(T203456)
//////////////////////////////////////////////////////////////////////////
void save_settings(String input) {
  tmElements_t tm_set;
  tm_set = tm;

  //convert to upper char, the value of cmd will be D or T
  char cmd = input[0] & ~(0x20);

  if (cmd == 'D') { //set date
    int year = input.substring(1, 3).toInt(); //from 2nd character to 4th character
    tm_set.Year = year1970or2000(year); // year can be given as '2010' or '10'.  It is converted to years since 1970
    tm_set.Month = input.substring(3, 5).toInt(); //from 5th character to 6th character
    tm_set.Day = input.substring(5, 7).toInt();

    // checking for valid date, save settings if a date is valid
    if (check_date(tm_set.Year, tm_set.Month, tm_set.Day)) {
      RTC.write(tm_set);
      DEBUG_PRINT("Date changed to ");
      DEBUG_PRINTLN(dateStr(tm_set));
      lcd.clear();
      lcd.print("Date changed");
    }
    else {
      DEBUG_PRINT("Error: ");
      DEBUG_PRINTLN(dateStr(tm_set));
      lcd.clear();
      lcd.print("Err: ");
      lcd.setCursor(0, SECOND_LINE);
      lcd.print(dateStr(tm_set));
    }
  }

  if (cmd == 'T') { //set time
    tm_set.Hour = input.substring(1, 3).toInt();
    tm_set.Minute = input.substring(3, 5).toInt();
    tm_set.Second = input.substring(5, 7).toInt();

    // checking for valid time, save settings if a time is valid
    if (check_time(tm_set.Hour, tm_set.Minute, tm_set.Second)) {
      RTC.write(tm_set);
      DEBUG_PRINT("Time changed to ");
      DEBUG_PRINTLN(timeStr(tm_set));
      lcd.clear();
      lcd.print("Time changed");
      lcd.setCursor(0, SECOND_LINE);
      lcd.print(timeStr(tm_set));
    }
    else {
      DEBUG_PRINT("Time update failed: ");
      DEBUG_PRINTLN(timeStr(tm_set));
      lcd.clear();
      lcd.print("Err: ");
      lcd.setCursor(0, SECOND_LINE);
      lcd.print(timeStr(tm_set));
    }
  }

  delay(2000);
  lcd.clear();
}

//////////////////////////////////////////////////////////////////////////
// button input
//////////////////////////////////////////////////////////////////////////
void button_input() {
  byte lcd_key = key_press();   // read the buttons
  switch (lcd_key) {
    case btnRIGHT:
      DEBUG_PRINTLN("Set Date");
      lcd.clear();
      lcd.print("Set Date");
      set_date();
      break;
    case btnLEFT:
      DEBUG_PRINTLN("Set Time");
      lcd.clear();
      lcd.print("Set Time");
      set_time();
      break;
  }
}

//////////////////////////////////////////////////////////////////////////
// set system date
// return Dyymmnn (eg. "D161231")
//////////////////////////////////////////////////////////////////////////
void set_date() {
  show_date(SECOND_LINE);
  int years = -1;
  int months = -1;
  int days = -1;

  //get_button_input(initial value, x_position, min value, max value)
  years = get_button_input(tmYearToCalendar(tm.Year), 0, 0, 2099); //year ranged from 0 to 2099
  if (years != -1) {
    months = get_button_input(tm.Month, 5, 1, 12); //month ranged from 1 to 12
  }

  if (months != -1) {
    byte lastday = get_lastday(years, months); //last day of a month
    days = get_button_input(tm.Day, 8, 1, lastday);
  }

  if (days != -1) {
    String this_date = "D" + int2str(years % 100) + int2str(months) + int2str(days);
    save_settings(this_date);
  }
}

//////////////////////////////////////////////////////////////////////////
// set system time
// return Thhmmss (eg. "T130150")
//////////////////////////////////////////////////////////////////////////
void set_time() {
  show_time(SECOND_LINE);
  int hours = -1;
  int minutes = -1;
  int seconds = -1;

  //get_button_input(initial value, x_position, min value, max value)
  hours = get_button_input(tm.Hour, 0, 0, 23); //hour ranged from 0 to 23

  if (hours != -1) {
    minutes = get_button_input(tm.Minute, 3, 0, 59); //minute ranged from 0 to 59
  }

  if (minutes != -1) {
    seconds = get_button_input(tm.Second, 6, 0, 59); //second ranged from 0 to 59
  }

  if (seconds != -1) {
    String this_time = "T" + int2str(hours) + int2str(minutes) + int2str(seconds);
    save_settings(this_time);
  }
}

//////////////////////////////////////////////////////////////////////////
// read input from button switches
//////////////////////////////////////////////////////////////////////////
int get_button_input(int val, byte pos_x, int min, int max) {
  previousMillis = millis(); // reset timeout so that the timeout will not occur
  while (1) { // loop forver
    if (isTimeout()) { //exit loop if timeout occured
      return -1;
    }

    byte lcd_key = key_press();   // read the buttons
    switch (lcd_key) {
      case btnUP:
        val++;
        if (val > max) val = min;
        break;
      case btnDOWN:
        val--;
        if (val < min) val = max;
        break;
      case btnSELECT:
        lcd.setCursor(pos_x, SECOND_LINE);
        lcd.print(int2str(val));
        return val;
        break;
    }
    blink_text(pos_x, int2str(val));

    if (lcd_key != btnNONE) { //if a button is pressed
      previousMillis = millis(); // reset timeout so that the timeout will not occur
      DEBUG_PRINTLN(int2str(val));
    }
  }
}
unsigned long blink_previousMillis = 0;
boolean showText = true;
//////////////////////////////////////////////////////////////////////////
// blink text
//////////////////////////////////////////////////////////////////////////
void blink_text(byte pos_x, String str) {
  String str_space = "    "; // reserve enough space for blink text
  byte str_length = str.length();
  str_space = str_space.substring(0, str_length);
  lcd.setCursor(pos_x, SECOND_LINE);

  if (millis() >= blink_previousMillis + 400) { //blink text every 400ms
    blink_previousMillis = millis();
    if (showText) {
      lcd.print(str);
      showText = false;
    } else {
      lcd.print(str_space);
      showText = true;
    }
  }
}

//////////////////////////////////////////////////////////////////////////
// convert integer (max. 2 digits) to string with leading zero
//////////////////////////////////////////////////////////////////////////
String int2str(int i) {
  String str = String(i); //convert to string
  if (i < 10) {
    str = "0" + str; //add leading zero
  }
  return str;
}

//////////////////////////////////////////////////////////////////////////
// return true when idle for 10 seconds (defined by EVENT_TIMEOUT)
//////////////////////////////////////////////////////////////////////////
# define EVENT_TIMEOUT 10 //set timeout after ten seconds
boolean isTimeout() {
  unsigned long event_timeout = EVENT_TIMEOUT * 1000; //convert to second
  if (millis() >= previousMillis + event_timeout) {
    // play_timeout_tone();
    return true;
  }
  else return false;
}
















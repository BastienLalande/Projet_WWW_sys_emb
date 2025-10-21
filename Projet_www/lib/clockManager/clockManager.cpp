#include "DS1307.h"
#include <Arduino.h>

DS1307 clock;

void init_clock(){clock.begin();}

void setupTime(
    uint16_t _year, 
    uint8_t _month, 
    uint8_t _day, 
    uint8_t _hour, 
    uint8_t _minute, 
    uint8_t _second) {
  clock.fillByYMD(_year, _month, _day);
  clock.fillByHMS(_hour, _minute, _second);
  clock.setTime();
}

String getAAMMJJ(){
    clock.getTime();
    return 
        String(clock.year, DEC)+
        String(clock.month, DEC)+
        String(clock.dayOfMonth, DEC);
}

void printTime() {
  clock.getTime();
  Serial.print(clock.hour, DEC); Serial.print(":");
  Serial.print(clock.minute, DEC); Serial.print(":");
  Serial.print(clock.second, DEC); Serial.print("  ");
  Serial.print(clock.month, DEC); Serial.print("/");
  Serial.print(clock.dayOfMonth, DEC); Serial.print("/");
  Serial.print(clock.year + 2000, DEC); Serial.println("");
}
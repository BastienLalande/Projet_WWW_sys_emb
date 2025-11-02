#ifndef CLOCKMANAGER_H
#define CLOCKMANAGER_H

#include <Arduino.h>

void init_clock();

void setupTime(
    uint16_t _year, 
    uint8_t _month, 
    uint8_t _day, 
    uint8_t _hour, 
    uint8_t _minute, 
    uint8_t _second);

void getAAMMJJ(char *date);

void printTime();

#endif // CLOCKMANAGER_H

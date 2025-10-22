#pragma once
#ifndef SD_UTILS_H
#define SD_UTILS_H

#include <Arduino.h>
#include <SD.h>

#ifndef CHIPSELECT
  #define CHIPSELECT 4
#endif

#ifndef SD_PATH_MAX
  #define SD_PATH_MAX 64
#endif

#ifndef IO_BUF_SIZE
  #define IO_BUF_SIZE 64
#endif

bool init_SD();

bool writeFile(const char* path, const uint8_t* data, size_t len);

bool writeFile(const char* path, const char* msg);

void readFile(const char* path);

void createDirectory(const char* dirName);

void saveData(const char* data);

#endif // SD_UTILS_H
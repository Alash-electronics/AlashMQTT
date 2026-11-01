// Имитация Arduino.h для тестов MQTT: время, Print, Client.
#pragma once
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <deque>
#include <string>
#include <vector>
class __FlashStringHelper;
#define F(x) (reinterpret_cast<const __FlashStringHelper *>(x))
inline uint32_t g_ms = 100000;
inline uint32_t millis() { return g_ms; }
inline void (*g_onDelay)() = nullptr;
inline void delay(uint32_t ms) { for (uint32_t i = 0; i < ms; i++) { g_ms++; if (g_onDelay) g_onDelay(); } }
class Print {
 public:
  std::string log;
  void put(const std::string &x) { log += x; }
  size_t print(const char *x) { put(x); return 0; }
  size_t print(const __FlashStringHelper *x) { put((const char *)x); return 0; }
  size_t print(int v) { put(std::to_string(v)); return 0; }
  size_t println(const char *x) { put(x); put("\n"); return 0; }
  size_t println(int v) { print(v); put("\n"); return 0; }
  size_t println(const __FlashStringHelper *x) { put((const char *)x); put("\n"); return 0; }
};

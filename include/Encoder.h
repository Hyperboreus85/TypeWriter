#pragma once

#include <Arduino.h>

class Encoder {
public:
  void begin(uint8_t pinA, uint8_t pinB);
  int16_t consumeDetents();

private:
  static void IRAM_ATTR isrRouter();
  void IRAM_ATTR handleIsr();

  static Encoder *instance_;

  uint8_t pinA_ = 0;
  uint8_t pinB_ = 0;
  volatile int16_t ticks_ = 0;
  volatile uint8_t prevState_ = 0;
  int8_t remainder_ = 0;
};

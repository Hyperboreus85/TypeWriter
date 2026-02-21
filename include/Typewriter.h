#pragma once

#include <Arduino.h>

class Typewriter {
public:
  void begin(const char *text, uint16_t charDelayMs, uint16_t linePauseMs);
  void setText(const char *text);
  void setTiming(uint16_t charDelayMs, uint16_t linePauseMs);
  void update();

private:
  static constexpr uint16_t MAX_TEXT_LEN = 512;

  char text_[MAX_TEXT_LEN + 1] = "";
  uint16_t charDelayMs_ = 50;
  uint16_t linePauseMs_ = 2000;
  uint16_t printIndex_ = 0;
  bool waitingLinePause_ = false;
  unsigned long lastEventMs_ = 0;
};

#pragma once

#include <Arduino.h>

class Typewriter {
public:
  void begin(const char *text, uint32_t charDelayUs, uint32_t linePauseUs);
  void setText(const char *text);
  void setTiming(uint32_t charDelayUs, uint32_t linePauseUs);
  void update();

private:
  static constexpr uint16_t MAX_TEXT_LEN = 512;

  char text_[MAX_TEXT_LEN + 1] = "";
  uint32_t charDelayUs_ = 1000;
  uint32_t linePauseUs_ = 20000;
  uint16_t printIndex_ = 0;
  bool waitingLinePause_ = false;
  unsigned long lastEventUs_ = 0;
};

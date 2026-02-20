#pragma once

#include <Arduino.h>

struct ButtonEvents {
  bool shortClick = false;
  bool doubleClick = false;
  bool long2s = false;
  bool long5s = false;
};

class Button {
public:
  void begin(uint8_t pin);
  ButtonEvents poll();

private:
  uint8_t pin_ = 0;
  bool stableState_ = HIGH;
  bool lastRaw_ = HIGH;
  unsigned long lastChangeMs_ = 0;
  unsigned long pressStartMs_ = 0;
  bool long2Fired_ = false;
  bool long5Fired_ = false;
  uint8_t clickCount_ = 0;
  unsigned long clickDeadlineMs_ = 0;

  static constexpr unsigned long DEBOUNCE_MS = 30;
  static constexpr unsigned long DOUBLE_CLICK_GAP_MS = 350;
  static constexpr unsigned long LONG_PRESS_2S_MS = 2000;
  static constexpr unsigned long LONG_PRESS_5S_MS = 5000;
};

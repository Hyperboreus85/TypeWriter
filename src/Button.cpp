#include "Button.h"

void Button::begin(uint8_t pin) {
  pin_ = pin;
  pinMode(pin_, INPUT_PULLUP);
  stableState_ = digitalRead(pin_);
  lastRaw_ = stableState_;
  lastChangeMs_ = millis();
}

ButtonEvents Button::poll() {
  ButtonEvents events;
  const unsigned long now = millis();
  const bool raw = digitalRead(pin_);

  if (raw != lastRaw_) {
    lastRaw_ = raw;
    lastChangeMs_ = now;
  }

  if ((now - lastChangeMs_) >= DEBOUNCE_MS && raw != stableState_) {
    stableState_ = raw;
    if (stableState_ == LOW) {
      pressStartMs_ = now;
      long2Fired_ = false;
      long5Fired_ = false;
    } else if (!long2Fired_ && !long5Fired_) {
      clickCount_++;
      clickDeadlineMs_ = now + DOUBLE_CLICK_GAP_MS;
    }
  }

  if (stableState_ == LOW) {
    const unsigned long pressedFor = now - pressStartMs_;
    if (!long2Fired_ && pressedFor >= LONG_PRESS_2S_MS) {
      long2Fired_ = true;
      events.long2s = true;
    }
    if (!long5Fired_ && pressedFor >= LONG_PRESS_5S_MS) {
      long5Fired_ = true;
      events.long5s = true;
    }
  }

  if (clickCount_ > 0 && now >= clickDeadlineMs_) {
    if (clickCount_ == 1) {
      events.shortClick = true;
    } else {
      events.doubleClick = true;
    }
    clickCount_ = 0;
  }

  return events;
}

#include "Typewriter.h"

#include <cstring>

void Typewriter::begin(const char *text, uint32_t charDelayUs, uint32_t linePauseUs) {
  setText(text);
  setTiming(charDelayUs, linePauseUs);
  printIndex_ = 0;
  waitingLinePause_ = false;
  lastEventUs_ = micros();
}

void Typewriter::setText(const char *text) {
  strncpy(text_, text, MAX_TEXT_LEN);
  text_[MAX_TEXT_LEN] = '\0';
  printIndex_ = 0;
  waitingLinePause_ = false;
  lastEventUs_ = micros();
}

void Typewriter::setTiming(uint32_t charDelayUs, uint32_t linePauseUs) {
  charDelayUs_ = charDelayUs;
  linePauseUs_ = linePauseUs;
}

void Typewriter::update() {
  const unsigned long now = micros();

  if (text_[0] == '\0') {
    return;
  }

  if (!waitingLinePause_) {
    if (static_cast<unsigned long>(now - lastEventUs_) >= charDelayUs_) {
      const char c = text_[printIndex_++];
      Serial.print(c);
      lastEventUs_ = now;

      if (text_[printIndex_] == '\0') {
        Serial.println();
        printIndex_ = 0;
        waitingLinePause_ = true;
      }
    }
  } else if (static_cast<unsigned long>(now - lastEventUs_) >= linePauseUs_) {
    waitingLinePause_ = false;
    lastEventUs_ = now;
  }
}

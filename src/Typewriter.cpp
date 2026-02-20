#include "Typewriter.h"

#include <cstring>

void Typewriter::begin(const char *text, uint16_t charDelayMs, uint16_t linePauseMs) {
  setText(text);
  setTiming(charDelayMs, linePauseMs);
  printIndex_ = 0;
  waitingLinePause_ = false;
  lastEventMs_ = millis();
}

void Typewriter::setText(const char *text) {
  strncpy(text_, text, MAX_TEXT_LEN);
  text_[MAX_TEXT_LEN] = '\0';
  printIndex_ = 0;
  waitingLinePause_ = false;
  lastEventMs_ = millis();
}

void Typewriter::setTiming(uint16_t charDelayMs, uint16_t linePauseMs) {
  charDelayMs_ = charDelayMs;
  linePauseMs_ = linePauseMs;
}

void Typewriter::update() {
  const unsigned long now = millis();

  if (text_[0] == '\0') {
    return;
  }

  if (!waitingLinePause_) {
    if (now - lastEventMs_ >= charDelayMs_) {
      const char c = text_[printIndex_++];
      Serial.print(c);
      lastEventMs_ = now;

      if (text_[printIndex_] == '\0') {
        Serial.println();
        printIndex_ = 0;
        waitingLinePause_ = true;
      }
    }
  } else if (now - lastEventMs_ >= linePauseMs_) {
    waitingLinePause_ = false;
    lastEventMs_ = now;
  }
}

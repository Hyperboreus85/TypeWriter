#include "Encoder.h"

Encoder *Encoder::instance_ = nullptr;

void Encoder::begin(uint8_t pinA, uint8_t pinB) {
  pinA_ = pinA;
  pinB_ = pinB;

  pinMode(pinA_, INPUT_PULLUP);
  pinMode(pinB_, INPUT_PULLUP);

  prevState_ = (digitalRead(pinA_) << 1) | digitalRead(pinB_);
  instance_ = this;
  attachInterrupt(digitalPinToInterrupt(pinA_), isrRouter, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pinB_), isrRouter, CHANGE);
}

void IRAM_ATTR Encoder::isrRouter() {
  if (instance_ != nullptr) {
    instance_->handleIsr();
  }
}

void IRAM_ATTR Encoder::handleIsr() {
  const uint8_t a = digitalRead(pinA_);
  const uint8_t b = digitalRead(pinB_);
  const uint8_t current = (a << 1) | b;
  const uint8_t index = (prevState_ << 2) | current;

  static const int8_t table[16] = {
      0, -1, 1, 0,
      1, 0, 0, -1,
      -1, 0, 0, 1,
      0, 1, -1, 0,
  };

  ticks_ += table[index];
  prevState_ = current;
}

int16_t Encoder::consumeDetents() {
  int16_t ticks;
  noInterrupts();
  ticks = ticks_;
  ticks_ = 0;
  interrupts();

  const int16_t total = ticks + remainder_;
  const int16_t detents = total / 4;
  remainder_ = total % 4;
  return detents;
}

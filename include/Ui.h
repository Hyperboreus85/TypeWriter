#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Wire.h>

class Ui {
public:
  static constexpr uint16_t MAX_TEXT_LEN = 512;

  bool begin();
  void markDirty();
  void refreshMain(uint16_t charDelayMs, uint16_t pauseMs, bool editChar, const char *text);
  void refreshEditor(const char *buffer, uint16_t pos, uint8_t selection, const char *charSet,
                     uint8_t charCount, uint8_t specialBack, uint8_t specialOk);
  void refreshPassword(uint8_t pwdPos, uint8_t currentDigit);
  void refreshSecretMenu(const char *const *items, uint8_t count, uint8_t selected);
  void refreshMessage(const char *line1, const char *line2);
  void sleep();
  void wake();
  bool isSleeping() const;

private:
  static constexpr uint8_t WIDTH = 128;
  static constexpr uint8_t HEIGHT = 64;
  static constexpr uint8_t ADDRESS = 0x3C; // alcuni moduli SH1106 usano 0x3D

  Adafruit_SH1106G display_{WIDTH, HEIGHT, &Wire, -1};
  bool displayOk_ = false;
  bool dirty_ = true;
  bool sleeping_ = false;
};

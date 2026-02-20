#pragma once

#include <Adafruit_SSD1306.h>

class Ui {
public:
  static constexpr uint8_t MAX_TEXT_LEN = 64;

  bool begin();
  void markDirty();
  void refreshMain(uint16_t charDelayMs, uint16_t pauseMs, bool editChar, const char *text);
  void refreshEditor(const char *buffer, uint8_t pos, uint8_t selection, const char *charSet,
                     uint8_t charCount, uint8_t specialBack, uint8_t specialOk);
  void refreshPassword(uint8_t pwdPos, uint8_t currentDigit);
  void refreshSecretMenu(const char *const *items, uint8_t count, uint8_t selected);
  void refreshMessage(const char *line1, const char *line2);

private:
  static constexpr uint8_t WIDTH = 128;
  static constexpr uint8_t HEIGHT = 64;
  static constexpr uint8_t ADDRESS = 0x3C;

  Adafruit_SSD1306 display_{WIDTH, HEIGHT, &Wire, -1};
  bool displayOk_ = false;
  bool dirty_ = true;
};

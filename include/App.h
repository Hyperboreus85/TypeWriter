#pragma once

#include "Button.h"
#include "Encoder.h"
#include "Typewriter.h"
#include "Ui.h"

class App {
public:
  void setup();
  void loop();

private:
  enum class ScreenState {
    MAIN,
    STRING_EDITOR,
    PASSWORD,
    SECRET_MENU,
    MESSAGE,
  };

  static constexpr uint8_t PIN_ENC_A = 32;
  static constexpr uint8_t PIN_ENC_B = 33;
  static constexpr uint8_t PIN_BUTTON = 25;

  static constexpr uint16_t DEFAULT_CHAR_DELAY_MS = 50;
  static constexpr uint16_t DEFAULT_LINE_PAUSE_MS = 2000;
  static constexpr uint16_t MIN_CHAR_DELAY_MS = 0;
  static constexpr uint16_t MAX_CHAR_DELAY_MS = 1000;
  static constexpr uint16_t MIN_LINE_PAUSE_MS = 0;
  static constexpr uint16_t MAX_LINE_PAUSE_MS = 30000;
  static constexpr uint8_t MAX_TEXT_LEN = 64;

  static constexpr uint8_t PASSWORD_LEN = 4;
  static constexpr uint8_t DEFAULT_PASSWORD[PASSWORD_LEN] = {1, 2, 3, 4};

  static const char DEFAULT_TEXT[];
  static const char *const SECRET_MENU_ITEMS[];
  static const char EDITOR_CHAR_SET[];

  static constexpr uint8_t SECRET_MENU_COUNT = 4;
  static constexpr uint8_t EDITOR_CHAR_COUNT = 73;
  static constexpr uint8_t EDITOR_SPECIAL_BACK = EDITOR_CHAR_COUNT;
  static constexpr uint8_t EDITOR_SPECIAL_OK = EDITOR_CHAR_COUNT + 1;
  static constexpr uint8_t EDITOR_SPECIAL_CANCEL = EDITOR_CHAR_COUNT + 2;
  static constexpr uint8_t EDITOR_OPTIONS_COUNT = EDITOR_CHAR_COUNT + 3;

  bool editCharDelay_ = true;
  uint16_t charDelayMs_ = DEFAULT_CHAR_DELAY_MS;
  uint16_t linePauseMs_ = DEFAULT_LINE_PAUSE_MS;
  char text_[MAX_TEXT_LEN + 1] = "";

  ScreenState screen_ = ScreenState::MAIN;

  char editorBuffer_[MAX_TEXT_LEN + 1] = "";
  uint8_t editorPos_ = 0;
  uint8_t editorSelection_ = 0;

  uint8_t pwdInput_[PASSWORD_LEN] = {0, 0, 0, 0};
  uint8_t pwdPos_ = 0;
  uint8_t pwdCurrentDigit_ = 0;

  uint8_t secretMenuIndex_ = 0;

  char messageLine1_[22] = "";
  char messageLine2_[22] = "";
  unsigned long messageUntilMs_ = 0;

  Encoder encoder_;
  Button button_;
  Ui ui_;
  Typewriter typewriter_;

  uint16_t appliedCharDelayMs_ = DEFAULT_CHAR_DELAY_MS;
  uint16_t appliedLinePauseMs_ = DEFAULT_LINE_PAUSE_MS;
  char appliedText_[MAX_TEXT_LEN + 1] = "";

  void disableWireless();
  void handleEncoder(int16_t detents);
  void handleButton(const ButtonEvents &events);
  void handleMessageTimeout();
  void refreshUi();

  void beginMain();
  void beginEditor(bool preload);
  void beginPassword();
  void showMessage(const char *line1, const char *line2, unsigned long durationMs);

  void applyMainStep(int16_t steps);
  void applyPasswordStep(int16_t steps);
  void applySecretMenuStep(int16_t steps);
  void applyEditorStep(int16_t steps);

  void executeMenuItem(uint8_t index);
  void resetDefaults(bool includeText);
  void saveEditorToMain();
};

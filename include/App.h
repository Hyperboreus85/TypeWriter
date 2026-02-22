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

  static constexpr uint32_t DEFAULT_CHAR_DELAY_US = 1000;
  static constexpr uint32_t DEFAULT_LINE_PAUSE_US = 20000;
  static constexpr uint32_t MIN_CHAR_DELAY_US = 1;
  static constexpr uint32_t MAX_CHAR_DELAY_US = 1000000;
  static constexpr uint32_t MIN_LINE_PAUSE_US = 1;
  static constexpr uint32_t MAX_LINE_PAUSE_US = 30000000;
  static constexpr uint16_t MAX_TEXT_LEN = 512;

  static constexpr unsigned long DISPLAY_SLEEP_TIMEOUT_MS = 60000;

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
  uint32_t charDelayUs_ = DEFAULT_CHAR_DELAY_US;
  uint32_t linePauseUs_ = DEFAULT_LINE_PAUSE_US;
  char text_[MAX_TEXT_LEN + 1] = "";

  ScreenState screen_ = ScreenState::MAIN;

  char editorBuffer_[MAX_TEXT_LEN + 1] = "";
  uint16_t editorPos_ = 0;
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

  uint32_t appliedCharDelayUs_ = DEFAULT_CHAR_DELAY_US;
  uint32_t appliedLinePauseUs_ = DEFAULT_LINE_PAUSE_US;
  char appliedText_[MAX_TEXT_LEN + 1] = "";
  unsigned long lastInteractionMs_ = 0;

  void disableWireless();
  void handleEncoder(int16_t detents, uint8_t speedStep);
  void handleButton(const ButtonEvents &events);
  void handleMessageTimeout();
  void refreshUi();
  void handleDisplayPower(bool userInteraction, bool encoderMoved);

  void beginMain();
  void beginEditor(bool preload);
  void beginPassword();
  void showMessage(const char *line1, const char *line2, unsigned long durationMs);

  void applyMainStep(int16_t detents, uint8_t speedStep);
  void applyPasswordStep(int16_t steps);
  void applySecretMenuStep(int16_t steps);
  void applyEditorStep(int16_t steps);

  void executeMenuItem(uint8_t index);
  void resetDefaults(bool includeText);
  void saveEditorToMain();
};

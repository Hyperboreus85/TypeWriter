#include "App.h"

#include <WiFi.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_wifi.h>

#include <cstring>

const char App::DEFAULT_TEXT[] = "Ciao dal TypeWriter!";
const char *const App::SECRET_MENU_ITEMS[] = {
    "Esci", "Reset parametri", "Reset completo", "Editor stringa"};
const char App::EDITOR_CHAR_SET[] =
    " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.,-_:;!?";

void App::setup() {
  Serial.begin(115200);
  disableWireless();

  strncpy(text_, DEFAULT_TEXT, MAX_TEXT_LEN);
  text_[MAX_TEXT_LEN] = '\0';

  button_.begin(PIN_BUTTON);
  encoder_.begin(PIN_ENC_A, PIN_ENC_B);
  ui_.begin();
  ui_.markDirty();

  strncpy(appliedText_, text_, MAX_TEXT_LEN);
  appliedText_[MAX_TEXT_LEN] = '\0';
  appliedCharDelayMs_ = charDelayMs_;
  appliedLinePauseMs_ = linePauseMs_;
  typewriter_.begin(text_, charDelayMs_, linePauseMs_);
}

void App::loop() {
  const ButtonEvents events = button_.poll();
  const int16_t detents = encoder_.consumeDetents();

  if (charDelayMs_ != appliedCharDelayMs_ || linePauseMs_ != appliedLinePauseMs_) {
    typewriter_.setTiming(charDelayMs_, linePauseMs_);
    appliedCharDelayMs_ = charDelayMs_;
    appliedLinePauseMs_ = linePauseMs_;
  }

  if (strncmp(text_, appliedText_, MAX_TEXT_LEN) != 0) {
    typewriter_.setText(text_);
    strncpy(appliedText_, text_, MAX_TEXT_LEN);
    appliedText_[MAX_TEXT_LEN] = '\0';
  }

  typewriter_.update();

  handleMessageTimeout();
  handleEncoder(detents);
  handleButton(events);
  refreshUi();
}

void App::disableWireless() {
  WiFi.mode(WIFI_OFF);
  esp_wifi_stop();
  esp_wifi_deinit();

  btStop();
  esp_bt_controller_disable();
  esp_bt_controller_deinit();
}

void App::handleEncoder(int16_t detents) {
  if (detents == 0) return;

  switch (screen_) {
    case ScreenState::MAIN:
      applyMainStep(detents);
      break;
    case ScreenState::STRING_EDITOR:
      applyEditorStep(detents);
      break;
    case ScreenState::PASSWORD:
      applyPasswordStep(detents);
      break;
    case ScreenState::SECRET_MENU:
      applySecretMenuStep(detents);
      break;
    case ScreenState::MESSAGE:
      break;
  }
}

void App::handleButton(const ButtonEvents &events) {
  switch (screen_) {
    case ScreenState::MAIN:
      if (events.shortClick) {
        editCharDelay_ = !editCharDelay_;
        ui_.markDirty();
      }
      if (events.doubleClick) {
        beginEditor(true);
      }
      if (events.long5s) {
        beginPassword();
      }
      break;

    case ScreenState::STRING_EDITOR:
      if (events.shortClick) {
        if (editorSelection_ < EDITOR_CHAR_COUNT) {
          if (editorPos_ < MAX_TEXT_LEN) {
            editorBuffer_[editorPos_++] = EDITOR_CHAR_SET[editorSelection_];
            editorBuffer_[editorPos_] = '\0';
            ui_.markDirty();
          }
        } else if (editorSelection_ == EDITOR_SPECIAL_BACK) {
          if (editorPos_ > 0) {
            editorPos_--;
            editorBuffer_[editorPos_] = '\0';
            ui_.markDirty();
          }
        } else if (editorSelection_ == EDITOR_SPECIAL_OK) {
          saveEditorToMain();
          beginMain();
        } else {
          beginMain();
        }
      }

      if (events.long2s) {
        saveEditorToMain();
        showMessage("Stringa salvata", "Ritorno al main", 800);
      }
      break;

    case ScreenState::PASSWORD:
      if (events.shortClick) {
        pwdInput_[pwdPos_] = pwdCurrentDigit_;
        pwdPos_++;
        pwdCurrentDigit_ = 0;

        if (pwdPos_ >= PASSWORD_LEN) {
          bool ok = true;
          for (uint8_t i = 0; i < PASSWORD_LEN; i++) {
            if (pwdInput_[i] != DEFAULT_PASSWORD[i]) {
              ok = false;
              break;
            }
          }

          if (ok) {
            screen_ = ScreenState::SECRET_MENU;
            secretMenuIndex_ = 0;
            ui_.markDirty();
          } else {
            showMessage("Password errata", "Ritorno al main", 1500);
          }
        } else {
          ui_.markDirty();
        }
      }
      break;

    case ScreenState::SECRET_MENU:
      if (events.shortClick) {
        executeMenuItem(secretMenuIndex_);
      }
      break;

    case ScreenState::MESSAGE:
      break;
  }
}

void App::handleMessageTimeout() {
  if (screen_ == ScreenState::MESSAGE && millis() >= messageUntilMs_) {
    beginMain();
  }
}

void App::refreshUi() {
  switch (screen_) {
    case ScreenState::MAIN:
      ui_.refreshMain(charDelayMs_, linePauseMs_, editCharDelay_, text_);
      break;
    case ScreenState::STRING_EDITOR:
      ui_.refreshEditor(editorBuffer_, editorPos_, editorSelection_, EDITOR_CHAR_SET,
                        EDITOR_CHAR_COUNT, EDITOR_SPECIAL_BACK, EDITOR_SPECIAL_OK);
      break;
    case ScreenState::PASSWORD:
      ui_.refreshPassword(pwdPos_, pwdCurrentDigit_);
      break;
    case ScreenState::SECRET_MENU:
      ui_.refreshSecretMenu(SECRET_MENU_ITEMS, SECRET_MENU_COUNT, secretMenuIndex_);
      break;
    case ScreenState::MESSAGE:
      ui_.refreshMessage(messageLine1_, messageLine2_);
      break;
  }
}

void App::beginMain() {
  screen_ = ScreenState::MAIN;
  ui_.markDirty();
}

void App::beginEditor(bool preload) {
  screen_ = ScreenState::STRING_EDITOR;

  if (preload) {
    strncpy(editorBuffer_, text_, MAX_TEXT_LEN);
    editorBuffer_[MAX_TEXT_LEN] = '\0';
    editorPos_ = strlen(editorBuffer_);
  }

  editorSelection_ = 0;
  ui_.markDirty();
}

void App::beginPassword() {
  screen_ = ScreenState::PASSWORD;
  memset(pwdInput_, 0, sizeof(pwdInput_));
  pwdPos_ = 0;
  pwdCurrentDigit_ = 0;
  ui_.markDirty();
}

void App::showMessage(const char *line1, const char *line2, unsigned long durationMs) {
  strncpy(messageLine1_, line1, sizeof(messageLine1_) - 1);
  messageLine1_[sizeof(messageLine1_) - 1] = '\0';
  strncpy(messageLine2_, line2, sizeof(messageLine2_) - 1);
  messageLine2_[sizeof(messageLine2_) - 1] = '\0';

  screen_ = ScreenState::MESSAGE;
  messageUntilMs_ = millis() + durationMs;
  ui_.markDirty();
}

void App::applyMainStep(int16_t steps) {
  const int16_t scaled = editCharDelay_ ? (steps * 5) : (steps * 100);

  if (editCharDelay_) {
    int32_t next = static_cast<int32_t>(charDelayMs_) + scaled;
    if (next < MIN_CHAR_DELAY_MS) next = MIN_CHAR_DELAY_MS;
    if (next > MAX_CHAR_DELAY_MS) next = MAX_CHAR_DELAY_MS;
    charDelayMs_ = static_cast<uint16_t>(next);
  } else {
    int32_t next = static_cast<int32_t>(linePauseMs_) + scaled;
    if (next < MIN_LINE_PAUSE_MS) next = MIN_LINE_PAUSE_MS;
    if (next > MAX_LINE_PAUSE_MS) next = MAX_LINE_PAUSE_MS;
    linePauseMs_ = static_cast<uint16_t>(next);
  }

  ui_.markDirty();
}

void App::applyPasswordStep(int16_t steps) {
  int16_t next = static_cast<int16_t>(pwdCurrentDigit_) + steps;
  while (next < 0) next += 10;
  while (next > 9) next -= 10;
  pwdCurrentDigit_ = static_cast<uint8_t>(next);
  ui_.markDirty();
}

void App::applySecretMenuStep(int16_t steps) {
  int16_t next = static_cast<int16_t>(secretMenuIndex_) + steps;
  while (next < 0) next += SECRET_MENU_COUNT;
  while (next >= SECRET_MENU_COUNT) next -= SECRET_MENU_COUNT;
  secretMenuIndex_ = static_cast<uint8_t>(next);
  ui_.markDirty();
}

void App::applyEditorStep(int16_t steps) {
  int16_t next = static_cast<int16_t>(editorSelection_) + steps;
  while (next < 0) next += EDITOR_OPTIONS_COUNT;
  while (next >= EDITOR_OPTIONS_COUNT) next -= EDITOR_OPTIONS_COUNT;
  editorSelection_ = static_cast<uint8_t>(next);
  ui_.markDirty();
}

void App::executeMenuItem(uint8_t index) {
  switch (index) {
    case 0:
      beginMain();
      break;
    case 1:
      resetDefaults(false);
      showMessage("Reset parametri", "Completato", 900);
      break;
    case 2:
      resetDefaults(true);
      showMessage("Reset completo", "Completato", 900);
      break;
    case 3:
      beginEditor(true);
      break;
  }
}

void App::resetDefaults(bool includeText) {
  charDelayMs_ = DEFAULT_CHAR_DELAY_MS;
  linePauseMs_ = DEFAULT_LINE_PAUSE_MS;
  if (includeText) {
    strncpy(text_, DEFAULT_TEXT, MAX_TEXT_LEN);
    text_[MAX_TEXT_LEN] = '\0';
  }
  ui_.markDirty();
}

void App::saveEditorToMain() {
  strncpy(text_, editorBuffer_, MAX_TEXT_LEN);
  text_[MAX_TEXT_LEN] = '\0';
}

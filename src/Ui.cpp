#include "Ui.h"

#include <Arduino.h>
#include <cstdio>
#include <cstring>

bool Ui::begin() {
  Wire.begin(21, 22);
  Wire.setClock(100000);

  displayOk_ = display_.begin(ADDRESS, true);
  if (!displayOk_) {
    Serial.println(F("OLED non trovato"));
    return false;
  }

  display_.clearDisplay();
  display_.setTextSize(1);
  display_.setTextColor(SH110X_WHITE);
  display_.setTextWrap(false);
  display_.display();
  dirty_ = true;
  sleeping_ = false;
  return true;
}

void Ui::markDirty() { dirty_ = true; }

void Ui::sleep() {
  if (!displayOk_ || sleeping_) return;
  display_.clearDisplay();
  display_.display();
  display_.ssd1306_command(SH110X_DISPLAYOFF);
  sleeping_ = true;
}

void Ui::wake() {
  if (!displayOk_ || !sleeping_) return;
  display_.ssd1306_command(SH110X_DISPLAYON);
  sleeping_ = false;
  dirty_ = true;
}

bool Ui::isSleeping() const { return sleeping_; }

void Ui::refreshMain(uint32_t charDelayUs, uint32_t pauseUs, bool editChar, const char *text) {
  if (!displayOk_ || !dirty_ || sleeping_) return;

  const uint32_t charMs = charDelayUs / 1000;
  const uint32_t charFrac = charDelayUs % 1000;
  const uint32_t pauseMs = pauseUs / 1000;
  const uint32_t pauseFrac = pauseUs % 1000;

  char line[22];

  display_.clearDisplay();
  display_.setCursor(0, 0);
  snprintf(line, sizeof(line), "CHAR:%lu.%03lu", static_cast<unsigned long>(charMs),
           static_cast<unsigned long>(charFrac));
  display_.print(line);

  display_.setCursor(0, 10);
  snprintf(line, sizeof(line), "PAUSA:%lu.%03lu", static_cast<unsigned long>(pauseMs),
           static_cast<unsigned long>(pauseFrac));
  display_.print(line);

  display_.setCursor(0, 20);
  display_.print("EDIT: ");
  display_.print(editChar ? "CHAR" : "PAUSA");

  char view[22];
  if (strlen(text) <= 18) {
    strncpy(view, text, sizeof(view) - 1);
    view[sizeof(view) - 1] = '\0';
  } else {
    strncpy(view, text, 17);
    view[17] = '\0';
    strcat(view, "...");
  }

  display_.setCursor(0, 32);
  display_.print("TXT: ");
  display_.print(view);

  display_.setCursor(0, 52);
  display_.print("1x:toggle 2x:editor");
  display_.display();
  dirty_ = false;
}

void Ui::refreshEditor(const char *buffer, uint16_t pos, uint8_t selection, const char *charSet,
                       uint8_t charCount, uint8_t specialBack, uint8_t specialOk) {
  if (!displayOk_ || !dirty_ || sleeping_) return;

  display_.clearDisplay();
  display_.setCursor(0, 0);
  display_.print("Editor stringa");

  const uint16_t start = pos > 14 ? pos - 14 : 0;
  char view[22];
  uint8_t v = 0;
  if (start > 0) view[v++] = '.';

  for (uint16_t i = start; i < MAX_TEXT_LEN && buffer[i] != '\0' && v < 20; i++) {
    view[v++] = buffer[i];
  }
  view[v] = '\0';

  display_.setCursor(0, 14);
  display_.print(view);

  display_.setCursor(0, 24);
  display_.print("Pos:");
  display_.print(pos);
  display_.print('/');
  display_.print(MAX_TEXT_LEN);

  display_.setCursor(0, 38);
  display_.print("Sel: ");
  if (selection < charCount) {
    display_.print('\'');
    display_.print(charSet[selection]);
    display_.print('\'');
  } else if (selection == specialBack) {
    display_.print("BACK");
  } else if (selection == specialOk) {
    display_.print("OK/FINE");
  } else {
    display_.print("CANCEL");
  }

  display_.setCursor(0, 52);
  display_.print("Click=ok 2s=salva");
  display_.display();
  dirty_ = false;
}

void Ui::refreshPassword(uint8_t pwdPos, uint8_t currentDigit) {
  if (!displayOk_ || !dirty_ || sleeping_) return;

  display_.clearDisplay();
  display_.setCursor(0, 0);
  display_.print("MENU SEGRETO");
  display_.setCursor(0, 12);
  display_.print("Password (4 cifre)");

  display_.setCursor(0, 28);
  display_.print("Inserite: ");
  for (uint8_t i = 0; i < 4; i++) {
    display_.print(i < pwdPos ? '*' : '_');
    display_.print(' ');
  }

  display_.setCursor(0, 44);
  display_.print("Cifra corrente: ");
  display_.print(currentDigit);
  display_.display();
  dirty_ = false;
}

void Ui::refreshSecretMenu(const char *const *items, uint8_t count, uint8_t selected) {
  if (!displayOk_ || !dirty_ || sleeping_) return;

  display_.clearDisplay();
  display_.setCursor(0, 0);
  display_.print("Menu segreto");

  for (uint8_t i = 0; i < count; i++) {
    display_.setCursor(0, 14 + i * 12);
    display_.print(i == selected ? "> " : "  ");
    display_.print(items[i]);
  }

  display_.display();
  dirty_ = false;
}

void Ui::refreshMessage(const char *line1, const char *line2) {
  if (!displayOk_ || !dirty_ || sleeping_) return;

  display_.clearDisplay();
  display_.setCursor(0, 18);
  display_.print(line1);
  display_.setCursor(0, 32);
  display_.print(line2);
  display_.display();
  dirty_ = false;
}

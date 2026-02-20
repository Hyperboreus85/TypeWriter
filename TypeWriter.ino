#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/*
  ================================================================
  COME SI USA
  ================================================================
  Hardware:
  - Encoder: S1->D2, S2->D3, KEY->D4, 5V->5V, GND->GND
  - OLED SH1106 128x64 I2C usando libreria Adafruit_SSD1306
    (indirizzo default 0x3C, cambiare OLED_ADDR se serve 0x3D)

  Schermata principale:
  - Ruota encoder: modifica il parametro attivo (CHAR o PAUSA)
  - Click breve: alterna parametro attivo (EDIT: CHAR / EDIT: PAUSA)
  - Doppio click: entra nell'editor stringa
  - Pressione lunga 5s: apre richiesta password per menu segreto

  Editor stringa:
  - Ruota encoder: seleziona carattere/speciale corrente
  - Click breve: conferma selezione
      * carattere: aggiunge e avanza
      * BACK: cancella ultimo carattere
      * OK/FINE: salva stringa ed esce
      * CANCEL: esce senza salvare
  - Pressione lunga 2s: salva rapidamente ed esce

  Password menu segreto (4 cifre):
  - Ruota encoder: cambia cifra 0..9
  - Click breve: conferma cifra e passa alla successiva
  - Se corretta entra nel menu segreto, altrimenti ritorna a principale

  Menu segreto:
  - Ruota encoder: seleziona voce
  - Click breve: conferma voce
  - Voci: Esci, Reset parametri, Reset completo, Editor stringa

  Password di default: 1234 (modificare DEFAULT_PASSWORD)
*/

// -------------------- Configurazione pin --------------------
const uint8_t PIN_ENC_A = 2;   // S1 / CLK
const uint8_t PIN_ENC_B = 3;   // S2 / DT
const uint8_t PIN_BUTTON = 4;  // KEY (attivo LOW)

// -------------------- Configurazione OLED --------------------
const uint8_t SCREEN_WIDTH = 128;
const uint8_t SCREEN_HEIGHT = 64;
const int8_t OLED_RESET = -1;
const uint8_t OLED_ADDR = 0x3C; // Cambiare a 0x3D se necessario
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// -------------------- Parametri default --------------------
const uint16_t DEFAULT_CHAR_DELAY_MS = 50;
const uint16_t DEFAULT_LINE_PAUSE_MS = 2000;
const uint16_t MIN_CHAR_DELAY_MS = 0;
const uint16_t MAX_CHAR_DELAY_MS = 1000;
const uint16_t MIN_LINE_PAUSE_MS = 0;
const uint16_t MAX_LINE_PAUSE_MS = 30000;
const uint8_t DEFAULT_PASSWORD[4] = {1, 2, 3, 4};

const uint8_t MAX_TEXT_LEN = 64;
const char DEFAULT_TEXT[] = "Ciao dal TypeWriter!";

// -------------------- Stato applicazione --------------------
enum ScreenState {
  SCREEN_MAIN,
  SCREEN_STRING_EDITOR,
  SCREEN_PASSWORD,
  SCREEN_SECRET_MENU,
  SCREEN_MESSAGE
};

ScreenState currentScreen = SCREEN_MAIN;

enum EditTarget {
  EDIT_CHAR_DELAY,
  EDIT_LINE_PAUSE
};

EditTarget activeEditTarget = EDIT_CHAR_DELAY;
uint16_t velocitaDigitazione = DEFAULT_CHAR_DELAY_MS;
uint16_t tempoPausaFine = DEFAULT_LINE_PAUSE_MS;
char miaStringa[MAX_TEXT_LEN + 1] = DEFAULT_TEXT;

// -------------------- Typewriter non bloccante --------------------
uint8_t printIndex = 0;
bool waitingLinePause = false;
unsigned long lastTypeEventMs = 0;

// -------------------- Encoder con ISR --------------------
volatile int16_t encoderTicks = 0;
volatile uint8_t encoderPrevState = 0;
int8_t encoderRemainder = 0;

// -------------------- Gestione pulsante --------------------
const unsigned long DEBOUNCE_MS = 30;
const unsigned long DOUBLE_CLICK_GAP_MS = 350;
const unsigned long LONG_PRESS_2S_MS = 2000;
const unsigned long LONG_PRESS_5S_MS = 5000;

bool buttonStableState = HIGH;
bool buttonLastRaw = HIGH;
unsigned long buttonLastChangeMs = 0;
unsigned long buttonPressStartMs = 0;
bool long2Fired = false;
bool long5Fired = false;
uint8_t clickCount = 0;
unsigned long clickDeadlineMs = 0;

// Eventi input (consumati nel loop)
bool evShortClick = false;
bool evDoubleClick = false;
bool evLong2s = false;
bool evLong5s = false;

// -------------------- UI / display --------------------
bool displayDirty = true;
unsigned long lastDisplayRefreshMs = 0;
const unsigned long DISPLAY_IDLE_REFRESH_MS = 250;

char messageLine1[22] = "";
char messageLine2[22] = "";
unsigned long messageUntilMs = 0;

// -------------------- Password --------------------
uint8_t pwdInput[4] = {0, 0, 0, 0};
uint8_t pwdPos = 0;
uint8_t pwdCurrentDigit = 0;

// -------------------- Menu segreto --------------------
const char *secretMenuItems[] = {
  "Esci",
  "Reset parametri",
  "Reset completo",
  "Editor stringa"
};
const uint8_t secretMenuCount = sizeof(secretMenuItems) / sizeof(secretMenuItems[0]);
uint8_t secretMenuIndex = 0;

// -------------------- Editor stringa --------------------
const char editorCharSet[] =
  " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.,-_:;!?";
const uint8_t editorCharCount = sizeof(editorCharSet) - 1;
const uint8_t editorSpecialBack = editorCharCount;
const uint8_t editorSpecialOk = editorCharCount + 1;
const uint8_t editorSpecialCancel = editorCharCount + 2;
const uint8_t editorOptionsCount = editorCharCount + 3;

char editorBuffer[MAX_TEXT_LEN + 1] = "";
uint8_t editorPos = 0;
uint8_t editorSelection = 0;

void markDisplayDirty() {
  displayDirty = true;
}

void resetToDefaults(bool includeText) {
  velocitaDigitazione = DEFAULT_CHAR_DELAY_MS;
  tempoPausaFine = DEFAULT_LINE_PAUSE_MS;
  if (includeText) {
    strncpy(miaStringa, DEFAULT_TEXT, MAX_TEXT_LEN);
    miaStringa[MAX_TEXT_LEN] = '\0';
  }
  markDisplayDirty();
}

void showTimedMessage(const char *line1, const char *line2, unsigned long durationMs) {
  strncpy(messageLine1, line1, sizeof(messageLine1) - 1);
  messageLine1[sizeof(messageLine1) - 1] = '\0';
  strncpy(messageLine2, line2, sizeof(messageLine2) - 1);
  messageLine2[sizeof(messageLine2) - 1] = '\0';
  messageUntilMs = millis() + durationMs;
  currentScreen = SCREEN_MESSAGE;
  markDisplayDirty();
}

void encoderISR() {
  uint8_t a = digitalRead(PIN_ENC_A);
  uint8_t b = digitalRead(PIN_ENC_B);
  uint8_t current = (a << 1) | b;
  uint8_t index = (encoderPrevState << 2) | current;

  // Tabella Gray-code robusta
  static const int8_t table[16] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0
  };

  encoderTicks += table[index];
  encoderPrevState = current;
}

int16_t consumeEncoderDetents() {
  int16_t ticks;
  noInterrupts();
  ticks = encoderTicks;
  encoderTicks = 0;
  interrupts();

  int16_t accumulated = ticks + encoderRemainder;
  int16_t detents = accumulated / 4;
  encoderRemainder = accumulated % 4;
  return detents;
}

void emitButtonEventShort() { evShortClick = true; }
void emitButtonEventDouble() { evDoubleClick = true; }
void emitButtonEventLong2() { evLong2s = true; }
void emitButtonEventLong5() { evLong5s = true; }

void pollButton() {
  unsigned long now = millis();
  bool raw = digitalRead(PIN_BUTTON);

  if (raw != buttonLastRaw) {
    buttonLastRaw = raw;
    buttonLastChangeMs = now;
  }

  if ((now - buttonLastChangeMs) >= DEBOUNCE_MS && raw != buttonStableState) {
    buttonStableState = raw;
    if (buttonStableState == LOW) {
      buttonPressStartMs = now;
      long2Fired = false;
      long5Fired = false;
    } else {
      if (!long2Fired && !long5Fired) {
        clickCount++;
        clickDeadlineMs = now + DOUBLE_CLICK_GAP_MS;
      }
    }
  }

  if (buttonStableState == LOW) {
    unsigned long pressedFor = now - buttonPressStartMs;
    if (!long2Fired && pressedFor >= LONG_PRESS_2S_MS) {
      long2Fired = true;
      emitButtonEventLong2();
    }
    if (!long5Fired && pressedFor >= LONG_PRESS_5S_MS) {
      long5Fired = true;
      emitButtonEventLong5();
    }
  }

  if (clickCount > 0 && now >= clickDeadlineMs) {
    if (clickCount == 1) {
      emitButtonEventShort();
    } else {
      emitButtonEventDouble();
    }
    clickCount = 0;
  }
}

void clearButtonEvents() {
  evShortClick = false;
  evDoubleClick = false;
  evLong2s = false;
  evLong5s = false;
}

void updateTypewriter() {
  unsigned long now = millis();

  if (miaStringa[0] == '\0') {
    return;
  }

  if (!waitingLinePause) {
    if (now - lastTypeEventMs >= velocitaDigitazione) {
      char c = miaStringa[printIndex++];
      Serial.print(c);
      lastTypeEventMs = now;

      if (miaStringa[printIndex] == '\0') {
        Serial.println();
        printIndex = 0;
        waitingLinePause = true;
      }
    }
  } else {
    if (now - lastTypeEventMs >= tempoPausaFine) {
      waitingLinePause = false;
      lastTypeEventMs = now;
    }
  }
}

void enterMainScreen() {
  currentScreen = SCREEN_MAIN;
  markDisplayDirty();
}

void beginStringEditor(bool preloadCurrentText) {
  currentScreen = SCREEN_STRING_EDITOR;
  if (preloadCurrentText) {
    strncpy(editorBuffer, miaStringa, MAX_TEXT_LEN);
    editorBuffer[MAX_TEXT_LEN] = '\0';
    editorPos = strlen(editorBuffer);
    if (editorPos >= MAX_TEXT_LEN) {
      editorPos = MAX_TEXT_LEN - 1;
      editorBuffer[editorPos] = '\0';
    }
  }
  editorSelection = 0;
  markDisplayDirty();
}

void saveEditorBufferToMain() {
  strncpy(miaStringa, editorBuffer, MAX_TEXT_LEN);
  miaStringa[MAX_TEXT_LEN] = '\0';
}

void beginPasswordScreen() {
  currentScreen = SCREEN_PASSWORD;
  memset(pwdInput, 0, sizeof(pwdInput));
  pwdPos = 0;
  pwdCurrentDigit = 0;
  markDisplayDirty();
}

void handlePasswordResult() {
  bool ok = true;
  for (uint8_t i = 0; i < 4; i++) {
    if (pwdInput[i] != DEFAULT_PASSWORD[i]) {
      ok = false;
      break;
    }
  }

  if (ok) {
    currentScreen = SCREEN_SECRET_MENU;
    secretMenuIndex = 0;
  } else {
    showTimedMessage("Password errata", "Ritorno al main", 1500);
  }
  markDisplayDirty();
}

void applyEncoderToMain(int16_t steps) {
  if (steps == 0) return;

  if (activeEditTarget == EDIT_CHAR_DELAY) {
    int32_t next = (int32_t)velocitaDigitazione + steps;
    if (next < MIN_CHAR_DELAY_MS) next = MIN_CHAR_DELAY_MS;
    if (next > MAX_CHAR_DELAY_MS) next = MAX_CHAR_DELAY_MS;
    velocitaDigitazione = (uint16_t)next;
  } else {
    int32_t next = (int32_t)tempoPausaFine + steps;
    if (next < MIN_LINE_PAUSE_MS) next = MIN_LINE_PAUSE_MS;
    if (next > MAX_LINE_PAUSE_MS) next = MAX_LINE_PAUSE_MS;
    tempoPausaFine = (uint16_t)next;
  }
  markDisplayDirty();
}

void applyEncoderToPassword(int16_t steps) {
  if (steps == 0) return;
  int16_t next = (int16_t)pwdCurrentDigit + steps;
  while (next < 0) next += 10;
  while (next > 9) next -= 10;
  pwdCurrentDigit = (uint8_t)next;
  markDisplayDirty();
}

void applyEncoderToSecretMenu(int16_t steps) {
  if (steps == 0) return;
  int16_t next = (int16_t)secretMenuIndex + steps;
  while (next < 0) next += secretMenuCount;
  while (next >= secretMenuCount) next -= secretMenuCount;
  secretMenuIndex = (uint8_t)next;
  markDisplayDirty();
}

void applyEncoderToEditor(int16_t steps) {
  if (steps == 0) return;
  int16_t next = (int16_t)editorSelection + steps;
  while (next < 0) next += editorOptionsCount;
  while (next >= editorOptionsCount) next -= editorOptionsCount;
  editorSelection = (uint8_t)next;
  markDisplayDirty();
}

void handleMainEvents() {
  if (evShortClick) {
    activeEditTarget = (activeEditTarget == EDIT_CHAR_DELAY) ? EDIT_LINE_PAUSE : EDIT_CHAR_DELAY;
    markDisplayDirty();
  }

  if (evDoubleClick) {
    beginStringEditor(true);
  }

  if (evLong5s) {
    beginPasswordScreen();
  }
}

void handleEditorShortClick() {
  if (editorSelection < editorCharCount) {
    if (editorPos < MAX_TEXT_LEN) {
      editorBuffer[editorPos] = editorCharSet[editorSelection];
      if (editorPos < MAX_TEXT_LEN) {
        editorPos++;
      }
      if (editorPos > MAX_TEXT_LEN) editorPos = MAX_TEXT_LEN;
      editorBuffer[editorPos] = '\0';
    }
  } else if (editorSelection == editorSpecialBack) {
    if (editorPos > 0) {
      editorPos--;
      editorBuffer[editorPos] = '\0';
    }
  } else if (editorSelection == editorSpecialOk) {
    saveEditorBufferToMain();
    enterMainScreen();
  } else if (editorSelection == editorSpecialCancel) {
    enterMainScreen();
  }
  markDisplayDirty();
}

void handleEditorEvents() {
  if (evShortClick) {
    handleEditorShortClick();
  }

  if (evLong2s) {
    saveEditorBufferToMain();
    showTimedMessage("Stringa salvata", "Ritorno al main", 800);
  }
}

void handlePasswordEvents() {
  if (evShortClick) {
    pwdInput[pwdPos] = pwdCurrentDigit;
    pwdPos++;
    pwdCurrentDigit = 0;

    if (pwdPos >= 4) {
      handlePasswordResult();
    }
    markDisplayDirty();
  }
}

void executeSecretMenuItem(uint8_t index) {
  switch (index) {
    case 0:
      enterMainScreen();
      break;
    case 1:
      resetToDefaults(false);
      showTimedMessage("Reset parametri", "Completato", 900);
      break;
    case 2:
      resetToDefaults(true);
      showTimedMessage("Reset completo", "Completato", 900);
      break;
    case 3:
      beginStringEditor(true);
      break;
  }
}

void handleSecretMenuEvents() {
  if (evShortClick) {
    executeSecretMenuItem(secretMenuIndex);
  }
}

void handleMessageTimeout() {
  if (currentScreen == SCREEN_MESSAGE && millis() >= messageUntilMs) {
    enterMainScreen();
  }
}

void drawMainScreen() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("CHAR: ");
  display.print(velocitaDigitazione);
  display.print(" ms");

  display.setCursor(0, 10);
  display.print("PAUSA: ");
  display.print(tempoPausaFine);
  display.print(" ms");

  display.setCursor(0, 20);
  display.print("EDIT: ");
  display.print(activeEditTarget == EDIT_CHAR_DELAY ? "CHAR" : "PAUSA");

  display.setCursor(0, 32);
  display.print("TXT: ");

  char temp[22];
  uint8_t len = strlen(miaStringa);
  if (len <= 18) {
    strncpy(temp, miaStringa, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';
  } else {
    strncpy(temp, miaStringa, 17);
    temp[17] = '\0';
    strcat(temp, "...");
  }
  display.print(temp);

  display.setCursor(0, 52);
  display.print("1x:toggle 2x:editor");
}

void drawPasswordScreen() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("MENU SEGRETO");
  display.setCursor(0, 12);
  display.print("Password (4 cifre)");

  display.setCursor(0, 28);
  display.print("Inserite: ");
  for (uint8_t i = 0; i < 4; i++) {
    if (i < pwdPos) display.print('*');
    else display.print('_');
    display.print(' ');
  }

  display.setCursor(0, 44);
  display.print("Cifra corrente: ");
  display.print(pwdCurrentDigit);
}

void drawSecretMenuScreen() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("Menu segreto");

  for (uint8_t i = 0; i < secretMenuCount; i++) {
    display.setCursor(0, 14 + i * 12);
    display.print(i == secretMenuIndex ? "> " : "  ");
    display.print(secretMenuItems[i]);
  }
}

void drawEditorScreen() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("Editor stringa");

  // Finestra testo attorno al cursore
  int16_t start = editorPos > 14 ? editorPos - 14 : 0;
  char view[22];
  uint8_t v = 0;

  if (start > 0) {
    view[v++] = '.';
  }

  for (uint8_t i = start; i < MAX_TEXT_LEN && editorBuffer[i] != '\0' && v < 20; i++) {
    view[v++] = editorBuffer[i];
  }
  view[v] = '\0';

  display.setCursor(0, 14);
  display.print(view);

  display.setCursor(0, 24);
  display.print("Pos:");
  display.print(editorPos);
  display.print("/");
  display.print(MAX_TEXT_LEN);

  display.setCursor(0, 38);
  display.print("Sel: ");
  if (editorSelection < editorCharCount) {
    display.print('\'');
    display.print(editorCharSet[editorSelection]);
    display.print('\'');
  } else if (editorSelection == editorSpecialBack) {
    display.print("BACK");
  } else if (editorSelection == editorSpecialOk) {
    display.print("OK/FINE");
  } else {
    display.print("CANCEL");
  }

  display.setCursor(0, 52);
  display.print("Click=ok 2s=salva");
}

void drawMessageScreen() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 18);
  display.print(messageLine1);
  display.setCursor(0, 32);
  display.print(messageLine2);
}

void refreshDisplayIfNeeded() {
  unsigned long now = millis();
  if (!displayDirty && (now - lastDisplayRefreshMs < DISPLAY_IDLE_REFRESH_MS)) {
    return;
  }

  display.clearDisplay();

  switch (currentScreen) {
    case SCREEN_MAIN:
      drawMainScreen();
      break;
    case SCREEN_STRING_EDITOR:
      drawEditorScreen();
      break;
    case SCREEN_PASSWORD:
      drawPasswordScreen();
      break;
    case SCREEN_SECRET_MENU:
      drawSecretMenuScreen();
      break;
    case SCREEN_MESSAGE:
      drawMessageScreen();
      break;
  }

  display.display();
  displayDirty = false;
  lastDisplayRefreshMs = now;
}

void setup() {
  Serial.begin(115200);

  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  encoderPrevState = (digitalRead(PIN_ENC_A) << 1) | digitalRead(PIN_ENC_B);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_B), encoderISR, CHANGE);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    // Fallback minimale: se OLED non risponde, si continua comunque con Serial.
    Serial.println(F("OLED non trovato"));
  }

  display.clearDisplay();
  display.setTextWrap(false);
  display.display();
  markDisplayDirty();
}

void loop() {
  pollButton();
  updateTypewriter();
  handleMessageTimeout();

  int16_t enc = consumeEncoderDetents();
  if (enc != 0) {
    switch (currentScreen) {
      case SCREEN_MAIN:
        applyEncoderToMain(enc);
        break;
      case SCREEN_STRING_EDITOR:
        applyEncoderToEditor(enc);
        break;
      case SCREEN_PASSWORD:
        applyEncoderToPassword(enc);
        break;
      case SCREEN_SECRET_MENU:
        applyEncoderToSecretMenu(enc);
        break;
      case SCREEN_MESSAGE:
        break;
    }
  }

  switch (currentScreen) {
    case SCREEN_MAIN:
      handleMainEvents();
      break;
    case SCREEN_STRING_EDITOR:
      handleEditorEvents();
      break;
    case SCREEN_PASSWORD:
      handlePasswordEvents();
      break;
    case SCREEN_SECRET_MENU:
      handleSecretMenuEvents();
      break;
    case SCREEN_MESSAGE:
      break;
  }

  clearButtonEvents();
  refreshDisplayIfNeeded();
}

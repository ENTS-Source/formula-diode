// === VARIABLES ===
#define BTN_PIN D5 // TODO: Will we need to support multiple buttons?

// === STATE MACHINE ===
#define BTN_OPEN HIGH
#define BTN_CLOSED LOW
#define BTN_DEBOUNCE_MS 10

int btnLastState = BTN_OPEN;
int btnLastRead = BTN_OPEN;
int btnLastReadMs = 0;
bool btnDownTrigger = false;
bool btnUpTrigger = false;
bool btnPressed = false;

void btnUpdate() {
  btnDownTrigger = false;
  btnUpTrigger = false;

  int val = digitalRead(BTN_PIN);
  if (val != btnLastRead) {
    btnLastRead = val;
    btnLastReadMs = millis();
  }
  if ((millis() - lastReadMs) > BTN_DEBOUNCE_MS) {
    if (val != btnLastState) {
      btnLastState = val;
      if (lastState == BTN_CLOSED) {
        btnPressed = true;
        btnDownTrigger = true;
      } else {
        btnPressed = false;
        btnUpTrigger = true;
      }
    }
  }
}

void btnUpdateBlocking() {
  btnUpdate();
  delay(BTN_DEBOUNCE_MS + 5);
  btnUpdate();
}

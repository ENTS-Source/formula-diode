#include "Button.h"

Button::Button(uint8_t pin) {
  pinMode(pin, INPUT);
  this->pin = pin;
  this->lastState = BTN_OPEN;
  this->lastRead = BTN_OPEN;
  this->lastReadMs = 0;
  this->isDownTrigger = false;
  this->isUpTrigger = false;
  this->isPressed = false;
}

void Button::update() {
  this->isDownTrigger = false;
  this->isUpTrigger = false;
  
  int val = digitalRead(this->pin);
  if (val != this->lastRead) {
    this->lastRead = val;
    this->lastReadMs = millis();
  }
  if ((millis() - this->lastReadMs) > BTN_DEBOUNCE_MS) {
    if (val != this->lastState) {
      this->lastState = val;
      if (this->lastState == BTN_CLOSED) {
        this->isPressed = true;
        this->isDownTrigger = true;
      } else {
        this->isPressed = false;
        this->isUpTrigger = true;
      }
    }
  }
}

void Button::updateBlocking() {
  this->update();
  delay(BTN_DEBOUNCE_MS + 5);
  this->update();
}

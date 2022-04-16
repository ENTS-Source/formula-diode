#ifndef BUTTON_H
#define BUTTON_H

// *********************************************
// COPIED FROM ARDUINO GAME.
// *********************************************

#include <Arduino.h>

#define BTN_OPEN HIGH
#define BTN_CLOSED LOW
#define BTN_DEBOUNCE_MS 10

class Button {
  private:
    // Comms
    byte pin;

    // State
    unsigned long lastReadMs;
    int lastState;
    int lastRead;
  
  public:
    // Attributes
    bool isDownTrigger;
    bool isUpTrigger;
    bool isPressed;

    // Constructors + functions
    Button(byte pin);
    void update();
    void updateBlocking();
};

#endif

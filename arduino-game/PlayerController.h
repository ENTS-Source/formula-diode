#ifndef PLAYER_CONTROLLER_H
#define PLAYER_CONTROLLER_H

#include <Arduino.h>
#include <FastLED.h>

#define BTN_OPEN HIGH
#define BTN_CLOSED LOW
#define PHYSICS_ACCL 0.2 // Velocity added per button press
#define PHYSICS_FRICTION 0.015
#define PHYSICS_MAX_VELOCITY 3
#define PHYSICS_MIN_VELOCITY 0
// TODO: Gravity
#define PHYSICS_MS 5 // Time between physics checks

class PlayerController {
  private:
    // Comms
    byte readPin;

    // Vehicle properties
    float velocity;
    float position;

    // Physics
    unsigned long lastPhysics;

    // Debouncing
    int lastBtnState;
    int lastBtnRead;
    unsigned long lastBtnReadMs;
    unsigned long debounceDelayMs;

    // Functions
    void readVars();

  public:
    // Vehicle properties
    int location; // int version of position
    int length;
    CRGB color;

    // Constructors + functions
    PlayerController(byte pin);
    void runPhysics();
};

#endif

#ifndef PLAYER_CONTROLLER_H
#define PLAYER_CONTROLLER_H

#include <Arduino.h>
#include <FastLED.h>
#include "Button.h"

#define PHYSICS_ACCL 0.07 // Velocity added per button press
#define PHYSICS_FRICTION 0.015
#define PHYSICS_MAX_VELOCITY 3
#define PHYSICS_MIN_VELOCITY 0
// TODO: Gravity
#define PHYSICS_MS 5 // Time between physics checks

class PlayerController {
  private:
    // Vehicle properties
    float velocity;
    float position;

    // Physics
    unsigned long lastPhysics;

    // Misc
    Button* btn;

    // Functions
    void readVars();

  public:
    // Vehicle properties
    int location; // int version of position
    int length;
    CRGB color;

    // Game state
    unsigned long finishMs;

    // Constructors + functions
    PlayerController(byte pin, CRGB color);
    void runPhysics();
    void reset();
};

#endif

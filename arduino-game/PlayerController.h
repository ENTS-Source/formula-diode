#ifndef PLAYER_CONTROLLER_H
#define PLAYER_CONTROLLER_H

#include <Arduino.h>
#include <FastLED.h>
#include "Button.h"

#define PHYSICS_ACCL 0.2 // Velocity added per button press
#define PHYSICS_FRICTION 0.015
#define PHYSICS_MAX_VELOCITY 3
#define PHYSICS_MIN_VELOCITY 0
// TODO: Gravity

class PlayerController {
  private:
    // Vehicle properties
    float velocity;
    float position;

    // Misc
    Button* btn;

    // Functions
    void readVars();

  public:
    // Vehicle properties
    int location; // int version of position
    int length;
    CRGB color;

    // Constructors + functions
    PlayerController(byte pin, CRGB color);
    void runPhysics();
    void update();
};

#endif

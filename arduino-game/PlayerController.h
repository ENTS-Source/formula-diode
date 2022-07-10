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
    int unhandledPresses;
    unsigned long lastPhysics;

  public:
    // Vehicle properties
    int location; // int version of position
    int length;
    CRGB color;

    // Game state
    unsigned long finishMs;
    unsigned long lastLapFinishMs;
    bool isConnected;

    // Constructors + functions
    PlayerController(CRGB color);
    void recordPresses(int count);
    void runPhysics();
    void reset();
};

#endif

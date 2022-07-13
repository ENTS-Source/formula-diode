#include <FastLED.h>

#define PHYSICS_ACCL 0.07 // Velocity added per button press
#define PHYSICS_FRICTION 0.015
#define PHYSICS_MAX_VELOCITY 3
#define PHYSICS_MIN_VELOCITY 0
// TODO: Gravity
#define PHYSICS_MS 5 // Time between physics checks

struct PlayerState {
  float velocity;
  float position;
  int location; // int position
  int length;
  CRGB color;

  int unhandledPresses;
  long lastPhysics;

  long finishMs;
  long lastLapFinishMs;
  bool isConnected;
};

void playerPhysics(PlayerState player) {
  if (!player.isConnected) {
    return;
  }
  if (player.finishMs > 0) {
    return; // they're done the race
  }

  for (int i = 0; i < player.unhandledPresses; i++) {
    player.velocity += PHYSICS_ACCL;
  }
  player.unhandledPresses = 0;

  if ((millis() - player.lastPhysics) < PHYSICS_MS) {
    return; // done with physics for now
  }
  player.lastPhysics = millis();

  if (player.velocity > PHYSICS_MAX_VELOCITY) {
    player.velocity = PHYSICS_MAX_VELOCITY;
  }

  // Move vehicle
  player.velocity -= player.velocity * PHYSICS_FRICTION;
  if (player.velocity < PHYSICS_MIN_VELOCITY) {
    player.velocity = PHYSICS_MIN_VELOCITY;
  }
  player.position += player.velocity;
  player.location = round(player.position); // int location
}

void playerReset(PlayerState player) {
  player.length = 3;
  player.position = 0;
  player.location = 0;
  player.velocity = 0;
  player.finishMs = 0;
  player.lastLapFinishMs = 0;
  player.unhandledPresses = 0;
}

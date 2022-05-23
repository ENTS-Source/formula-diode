#include "PlayerController.h"

PlayerController::PlayerController(CRGB color) {
  this->color = color;
  this->reset();
  this->isConnected = false;
}

void PlayerController::reset() {
  this->length = 3;
  this->position = 0;
  this->location = 0;
  this->velocity = 0;
  this->finishMs = 0;
  this->unhandledPresses = 0;
}

void PlayerController::runPhysics() {
  if (!this->isConnected) {
    return;
  }
  if (this->finishMs > 0) {
    return; // don't run physics if we're done the race
  }

  for (int i = 0; i < this->unhandledPresses; i++) {
    this->velocity += PHYSICS_ACCL;
  }
  this->unhandledPresses = 0;

  // Check to see if we should be running physics at all
  if ((millis() - this->lastPhysics) < PHYSICS_MS) {
    return;
  }
  this->lastPhysics = millis();

  if (this->velocity > PHYSICS_MAX_VELOCITY) {
    this->velocity = PHYSICS_MAX_VELOCITY;
  }

  // Move vehicle
  this->velocity -= this->velocity * PHYSICS_FRICTION;
  if (this->velocity < PHYSICS_MIN_VELOCITY) {
    this->velocity = PHYSICS_MIN_VELOCITY;
  }
  this->position += this->velocity;
  this->location = round(this->position); // update int location
}

void PlayerController::recordPresses(int count) {
  this->unhandledPresses += count;
}

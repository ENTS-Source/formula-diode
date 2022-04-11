#include "PlayerController.h"

PlayerController::PlayerController(byte pin, CRGB color) {
  this->btn = new Button(pin);
  this->length = 3;
  this->position = 0;
  this->location = 0;
  this->velocity = 0;
  this->color = color;
}

void PlayerController::readVars() {
  this->btn->update();
  if (this->btn->isDownTrigger) {
    this->velocity += PHYSICS_ACCL;
  }
}

void PlayerController::runPhysics() {
  this->readVars();

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

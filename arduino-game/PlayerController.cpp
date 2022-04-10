#include "PlayerController.h"

PlayerController::PlayerController(byte pin) {
  pinMode(pin, INPUT);

  this->readPin = pin;
  this->length = 3;
  this->position = 0;
  this->location = 0;
  this->velocity = 0;
  this->color = CRGB::Blue;
}

void PlayerController::readVars() {
  int val = digitalRead(this->readPin);
  if (val != this->lastBtnRead) {
    this->lastBtnReadMs = millis();
    this->lastBtnRead = val;
  }
  if ((millis() - this->lastBtnReadMs) > this->debounceDelayMs) {
    if (val != this->lastBtnState) {
      this->lastBtnState = val;
      if (this->lastBtnState == BTN_CLOSED) {
        this->velocity += PHYSICS_ACCL;
      }
    }
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

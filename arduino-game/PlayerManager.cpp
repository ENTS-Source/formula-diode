#include "PlayerManager.h"

CRGB colors[MAX_PLAYERS] = {
  CRGB::Blue,
  CRGB::Red,
  CRGB::Green,
  CRGB::Orange,
};

byte i2cAddresses[MAX_PLAYERS] = {
  1,
  2,
  3,
  4,
}

PlayerManager::PlayerManager() {
  this->lastScanMs = 0;
  for (byte i = 0; i < MAX_PLAYERS; i++) {
    this->players[i] = Player(i);
  }
}

Player::Player(byte id) {
  this->id = id;
}

void Player::reset() {
  this->length = INITIAL_PLAYER_LENGTH;
  this->location = 0;
  this->velocity = 0;
  this->finishMs = 0;
  this->lastLapFinishMs = 0;
  this->unhandledPresses = 0;
}

void Player::physics() {
  if (this->finishMs > 0) {
    return; // they're done the race
  }
}
#include "Track.h"

Track::Track(byte startPin, PlayerController* players[]) {
  FastLED.addLeds<WS2812B, LED_STRIP_PIN, GRB>(this->leds, STRIP_LENGTH * STRIP_COUNT);
  for (int i = 0; i < NUM_PLAYERS_POSSIBLE; i++) {
    this->players[i] = players[i];
  }
  this->inGame = false;
  this->startTimeMs = 0;
  this->lightsStartMs = 0;
  this->startBtn = new Button(startPin);
  this->clearStrip();
  this->render();
}

// Primary loop
// ======================================

void Track::update() {
  this->clearStrip();
  this->startBtn->update();

  if (!this->inGame) {
    if (this->lightsStartMs > 0) {
      this->leds[TRAFFIC_START] = TRAFFIC_RED;
      this->leds[TRAFFIC_START - 1] = TRAFFIC_RED;

      if ((millis() - this->lightsStartMs) >= 1000) {
        this->leds[TRAFFIC_START - 2] = TRAFFIC_YELLOW;
        this->leds[TRAFFIC_START - 3] = TRAFFIC_YELLOW;
      }

      if ((millis() - this->lightsStartMs) >= 2000) {
        this->leds[TRAFFIC_START - 4] = TRAFFIC_GREEN;
        this->leds[TRAFFIC_START - 5] = TRAFFIC_GREEN;
      }

      if ((millis() - this->lightsStartMs) >= 2400) {
        this->inGame = true;
        this->lightsStartMs = 0;
        this->startTimeMs = millis();
      }
    } else if (this->startBtn->isDownTrigger) {
      this->lightsStartMs = millis();
    }
  } else {
    this->updatePlayers();
    this->drawPlayers();
  }

  this->render();
}

// Game functions
// ======================================

void Track::updatePlayers() {
  for (int i = 0; i < NUM_PLAYERS_POSSIBLE; i++) {
    this->players[i]->runPhysics();
  }
}

// Render functions
// ======================================

void Track::clearStrip() {
  for (int i = 0; i < (STRIP_LENGTH * STRIP_COUNT); i++) {
    this->leds[i] = CRGB::Black;
  }
}

void Track::drawPlayers() {
  // Figure out where each player is an render them into the LEDs
  int positionMap[STRIP_LENGTH * STRIP_COUNT];
  for (int i = 0; i < NUM_PLAYERS_POSSIBLE; i++) {
    PlayerController* player = this->players[i];
    int startPos = player->location % STRIP_LENGTH;
    for (int j = 0; j < player->length; j++) {
      positionMap[startPos + j]++;
      this->leds[startPos + j] = CRGB(
        this->leds[startPos + j].r + player->color.r,
        this->leds[startPos + j].g + player->color.g,
        this->leds[startPos + j].b + player->color.b
      );
    }
  }

  // Mix the colors if needed
  for (int i = 0; i < (STRIP_LENGTH * STRIP_COUNT); i++) {
    int mapHeight = positionMap[i];
    if (mapHeight > 1) {
      this->leds[i] = CRGB(
        this->leds[i].r / mapHeight,
        this->leds[i].g / mapHeight,
        this->leds[i].b / mapHeight
      );
    }
  }
}

void Track::render() {
  FastLED.show();
}

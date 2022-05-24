#include "Track.h"

Track::Track(byte startPin, PlayerController* players[], Config* config) {
  this->config = config;

  FastLED.addLeds<WS2812B, LED_STRIP_PIN, GRB>(this->leds, STRIP_LENGTH * STRIP_COUNT);
  for (int i = 0; i < I2C_PLAYERS; i++) {
    this->players[i] = players[i];
  }

  this->winner = NULL;
  this->endMs = 0;
  this->inGame = false;
  this->startTimeMs = 0;
  this->lightsStartMs = 0;
  this->startBtn = new Button(startPin);
  this->clearStrip();
  this->render();
}

// Primary loop
// ======================================

bool Track::update() {
  bool shouldReset = false;
  this->clearStrip();
  this->startBtn->update();

  if (!this->inGame) {
    if (this->winner != NULL) {
      unsigned long timeDiff = millis() - this->endMs;
      if (timeDiff < WINNER_SHOWN_MS) {
        this->drawWinner();
      } else {
        this->winner = NULL;
        this->endMs = 0;
      }
    } else if (this->lightsStartMs > 0) {
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
        shouldReset = true;
        this->inGame = true;
        this->lightsStartMs = 0;
        this->startTimeMs = millis();

        // Ensure the players aren't flagged as crossing the line
        for (int i = 0; i < I2C_PLAYERS; i++) {
          this->players[i]->reset();
        }
      }
    } else if (this->startBtn->isDownTrigger) {
      this->lightsStartMs = millis();
      shouldReset = true;
    }
  } else {
    this->updatePlayers();
    this->drawPlayers();
  }

  this->render();
  return shouldReset;
}

// Game functions
// ======================================

void Track::updatePlayers() {
  bool allDone = true;
  for (int i = 0; i < I2C_PLAYERS; i++) {
    if (!this->players[i]->isConnected) {
      continue;
    }
    this->players[i]->runPhysics();
    allDone = allDone && (this->players[i]->finishMs > 0);
  }
  if (allDone) {
    this->inGame = false;
    this->endMs = millis();
    for (int i = 0; i < I2C_PLAYERS; i++) {
      if (!this->players[i]->isConnected) {
        continue;
      }
      if (this->winner == NULL || this->winner->finishMs > this->players[i]->finishMs) {
        this->winner = this->players[i];
      }
    }
  }
}

void Track::setPlayerState(int player, bool isConnected) {
  this->players[player]->isConnected = isConnected;
  Serial.print(player);
  Serial.print(" is now connected? ");
  Serial.println(isConnected);
}

// Render functions
// ======================================

void Track::clearStrip() {
  for (int i = 0; i < (STRIP_LENGTH * STRIP_COUNT); i++) {
    this->leds[i] = CRGB::Black;
  }
}

void Track::drawWinner() {
  for (int i = 0; i < (STRIP_LENGTH * STRIP_COUNT); i++) {
    this->leds[i] = this->winner->color;
  }
}

void Track::drawPlayers() {
  // Figure out where each player is an render them into the LEDs
  int positionMap[STRIP_LENGTH * STRIP_COUNT];
  for (int i = 0; i < (STRIP_LENGTH * STRIP_COUNT); i++) {
    positionMap[i] = 0;
  }
  for (int i = 0; i < I2C_PLAYERS; i++) {
    PlayerController* player = this->players[i];
    if (!player->isConnected) {
      continue;
    }
    if (player->finishMs > 0) { // check first so we don't update the lap time
      continue;
    }
    if (player->location / STRIP_LENGTH >= MAX_LAPS) {
      player->finishMs = millis();
    }
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

void Track::setLed(int i, CRGB color) {
  this->leds[i] = color;
}

void Track::render() {
  FastLED.show();
}

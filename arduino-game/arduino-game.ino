#include <FastLED.h>
#include "Track.h"

// NodeMCU / ESP8266

// Note that LED_STRIP_PIN is set in Track.h

CRGB PLAYER1_COLOR = CRGB::Blue;
CRGB PLAYER2_COLOR = CRGB::Red;
CRGB PLAYER3_COLOR = CRGB::Green;
CRGB PLAYER4_COLOR = CRGB::Yellow;

CRGB TRAFFIC_RED = CRGB(255, 0, 0);
CRGB TRAFFIC_YELLOW = CRGB(255, 200, 0);
CRGB TRAFFIC_GREEN = CRGB(0, 255, 0);

Track* track;

void setup() {
  randomSeed(analogRead(D6)); // unconnected
  Serial.begin(9600);

  PlayerController* players[NUM_PLAYERS_POSSIBLE];
  players[0] = new PlayerController(D0, PLAYER1_COLOR);
  players[1] = new PlayerController(D1, PLAYER2_COLOR);
  players[2] = new PlayerController(D2, PLAYER3_COLOR);
  players[3] = new PlayerController(D3, PLAYER4_COLOR);

  track = new Track(D5, players);
}

void loop() {
  track->update();
}

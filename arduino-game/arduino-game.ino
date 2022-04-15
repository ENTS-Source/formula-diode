#include <FastLED.h>
#include "PlayerController.h"
#include "Track.h"
#include "Networking.h"
#include "Config.h"

// NodeMCU / ESP8266

// Note that LED_STRIP_PIN is set in Track.h

CRGB PLAYER1_COLOR = CRGB::Blue;
CRGB PLAYER2_COLOR = CRGB::Red;
CRGB PLAYER3_COLOR = CRGB::Green;
CRGB PLAYER4_COLOR = CRGB::Orange;

CRGB TRAFFIC_RED = CRGB(255, 0, 0);
CRGB TRAFFIC_YELLOW = CRGB(239, 83, 0);
CRGB TRAFFIC_GREEN = CRGB(0, 132, 5);

Config* config;
Track* track;
Networking* networking;

void setup() {
  randomSeed(analogRead(D6)); // unconnected
  Serial.begin(115200);

  PlayerController* players[MAX_PLAYERS];
  players[0] = new PlayerController(D0, PLAYER1_COLOR);
  players[1] = new PlayerController(D1, PLAYER2_COLOR);
  players[2] = new PlayerController(D2, PLAYER3_COLOR);
  players[3] = new PlayerController(D3, PLAYER4_COLOR);

  config = new Config();
  track = new Track(D5, players, config);
  networking = new Networking(track, config); // networking will read/write config for us
}

void loop() {
  networking->update();
  track->update();
}

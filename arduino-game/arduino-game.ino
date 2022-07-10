#include <FastLED.h>
#include "PlayerController.h"
#include "Track.h"
#include "Networking.h"
#include "Config.h"
#include "GameNet.h"

// Need to import this here so we don't try to import it a thousand times.
#include <AsyncHTTPRequest_Generic.h>

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
GameNet* gamenet;
PlayerController* players[I2C_PLAYERS];

void setup() {
  randomSeed(analogRead(D6)); // unconnected
  Serial.begin(115200);

  players[0] = new PlayerController(PLAYER1_COLOR);
  players[1] = new PlayerController(PLAYER2_COLOR);
  players[2] = new PlayerController(PLAYER3_COLOR);
  players[3] = new PlayerController(PLAYER4_COLOR);

  config = new Config();
  track = new Track(D5, players, config);
  networking = new Networking(track, config); // networking will read/write config for us
  gamenet = new GameNet(D2, D1);

  scanForPlayers();
  updatePlayerIds();
}

void loop() {
  networking->update();
  bool doReset = track->update();
  if (doReset) {
    scanForPlayers();
    gamenet->resetAll();
    updatePlayerIds();
  }
  gamenet->update();

  for (int i = 0; i < I2C_PLAYERS; i++) {
    byte state[TOHOST_LENGTH];
    if (gamenet->populateState(i, state)) {
      players[i]->recordPresses(state[0]);
    }
  }
}

void updatePlayerIds() {
  for (int i = 0; i < I2C_PLAYERS; i++) {
    gamenet->updateColor(i, players[i]->color.r, players[i]->color.g, players[i]->color.b);
  }
}

void scanForPlayers() {
  gamenet->scan();
  for (int i = 0; i < I2C_PLAYERS; i++) {
    track->setPlayerState(i, gamenet->isPlayerConnected(i));
  }
}

void markLapCompleted(int playerNum, int lap, unsigned long ms) {
  AsyncHTTPRequest* request = new AsyncHTTPRequest();
  request->onReadyStateChange(ignoreCallback);
  if (!request->open("POST", (config->scoreboardIP + ":20304/lap_done?player=" + String(playerNum) + "&lap=" + String(lap) + "&time=" + String(ms)).c_str())) {
    Serial.println("ERROR: markLapCompleted is a BAD REQUEST");
    return;
  }
  request->send();
}

void markAllLapsCompleted(int playerNum, unsigned long ms) {
  AsyncHTTPRequest* request = new AsyncHTTPRequest();
  request->onReadyStateChange(ignoreCallback);
  if (!request->open("POST", (config->scoreboardIP + ":20304/player_done?player=" + String(playerNum) + "&time=" + String(ms)).c_str())) {
    Serial.println("ERROR: markAllLapsCompleted is a BAD REQUEST");
    return;
  }
  request->send();
}

void markDNF(int playerNum) {
  AsyncHTTPRequest* request = new AsyncHTTPRequest();
  request->onReadyStateChange(ignoreCallback);
  if (!request->open("POST", (config->scoreboardIP + ":20304/player_dnf?player=" + String(playerNum)).c_str())) {
    Serial.println("ERROR: markDNF is a BAD REQUEST");
    return;
  }
  request->send();
}

void markGameEnd(int winner) {
  AsyncHTTPRequest* request = new AsyncHTTPRequest();
  request->onReadyStateChange(ignoreCallback);
  if (!request->open("POST", (config->scoreboardIP + ":20304/game_end?winner=" + String(winner)).c_str())) {
    Serial.println("ERROR: markGameEnd is a BAD REQUEST");
    return;
  }
  request->send();
}

void markGameStart(int numPlayers) {
  AsyncHTTPRequest* request = new AsyncHTTPRequest();
  request->onReadyStateChange(ignoreCallback);
  if (!request->open("POST", (config->scoreboardIP + ":20304/game_start?players=" + String(numPlayers)).c_str())) {
    Serial.println("ERROR: markGameStart is a BAD REQUEST");
    return;
  }
  request->send();
}

void markGameIntro() {
  AsyncHTTPRequest* request = new AsyncHTTPRequest();
  request->onReadyStateChange(ignoreCallback);
  if (!request->open("POST", (config->scoreboardIP + ":20304/game_intro").c_str())) {
    Serial.println("ERROR: markGameStart is a BAD REQUEST");
    return;
  }
  request->send();
}

void ignoreCallback(void* optParm, AsyncHTTPRequest* request, int readyState) {
  // consume
}

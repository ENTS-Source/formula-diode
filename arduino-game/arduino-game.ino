#include <FastLED.h>

// Need to import this here so we don't try to import it a thousand times.
#include <AsyncHTTPRequest_Generic.h>

// NodeMCU / ESP8266

#define NOT_CONNECTED_PIN D6

CRGB TRAFFIC_RED = CRGB(255, 0, 0);
CRGB TRAFFIC_YELLOW = CRGB(239, 83, 0);
CRGB TRAFFIC_GREEN = CRGB(0, 132, 5);

CRGB colors[I2C_PLAYERS] = {
  CRGB::Blue,
  CRGB::Red,
  CRGB::Green,
  CRGB::Orange,
}
PlayerState players[I2C_PLAYERS] = {
  PlayerState,
  PlayerState,
  PlayerState,
  PlayerState,
};

void setup() {
  randomSeed(analogRead(NOT_CONNECTED_PIN));
  Serial.begin(115200);
  confSetup();
  gnetSetup();
  trakSetup();
  netSetup();

  for (int i = 0; i < I2C_PLAYERS; i++) {
    playerReset(players[i]);
    players[i].color = colors[i];
  }

  gnetScan();
  updatePlayerIds();
}

void loop() {
  bool doReset = trakUpdate();
  if (doReset) {
    gnetScan();
    gnetResetAll();
    updatePlayerIds();
  }
  gnetUpdate();
}

void updatePlayerIds() {
  for (int i = 0; i < I2C_PLAYERS; i++) {
    gnetUpdateColor(i, players[i].color.r, players[i].color.g, players[i].color.b);
  }
}

// ---------------------------------------------------------------
// ---- HTTP
// ===============================================================

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

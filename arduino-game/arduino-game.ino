#include <FastLED.h>
#include <EEPROM.h>
#include <Regexp.h>
#include <AsyncHTTPRequest_Generic.h>
#include <WiFiManager.h>
#include <Wire.h>

// NodeMCU / ESP8266

#define BTN_PIN D5 // TODO: Will we need to support multiple buttons?
#define SDA_PIN D2
#define SCL_PIN D1
#define WIRE_SPEED 100000
#define I2C_PLAYERS 4
#define STRIP_COUNT 1 // TODO: Support this being 2 (using logical strips)
#define MAX_LAPS 3 // TODO: Config val
#define LED_STRIP_PIN D4
#define STRIP_LENGTH 50
#define WINNER_SHOWN_MS 2500

#define CONF_SB_IP_ADDR 128 // stay out of the way of wifimanager
#define CONF_SB_IP_LEN 16 // string, fixed length
#define NOT_CONNECTED_PIN D6
#define CONF_EEPROM_SIZE 512
#define NO_SB_IP "none"
#define BTN_OPEN HIGH
#define BTN_CLOSED LOW
#define BTN_DEBOUNCE_MS 10
#define PHYSICS_ACCL 0.07 // Velocity added per button press
#define PHYSICS_FRICTION 0.015
#define PHYSICS_MAX_VELOCITY 3
#define PHYSICS_MIN_VELOCITY 0
// TODO: Gravity
#define PHYSICS_MS 5 // Time between physics checks
#define I2C_PLAYER_START 0 // address
#define TOHOST_LENGTH 2  // btn 1 presses & btn 2 presses, 2 bytes
#define FROMHOST_ASSIGN 0x10
#define FROMHOST_RESET 0x11
#define TRAFFIC_START 7 // address; +1 from bottom, for aesthetics. Must be at least 6
#define NO_WINNER -1
#define NW_STATUS_LED 4
#define FIELD_SCOREBOARD_NAME "scoreboard"
#define FIELD_NUMLEDS_NAME "numLeds"
#define REQUESTS_IN_POOL 12
#define WIRE_PING_MS 5000

struct PlayerState {
  float velocity;
  float position;
  int location; // int position
  int length;
  CRGB color;

  int unhandledPresses;
  long lastPhysics;

  long finishMs;
  long lastLapFinishMs;
  bool isConnected;
};

char scoreboardIp[CONF_SB_IP_LEN];
CRGB leds[STRIP_LENGTH * STRIP_COUNT];
long startTimeMs = 0;
long lightsStartMs = 0;
long endMs = 0;
int winnerNum = NO_WINNER;
bool inGame = false;
bool btnDownTrigger = false;
bool btnUpTrigger = false;
bool btnPressed = false;
long lastWireScan = 0;

CRGB TRAFFIC_RED = CRGB(255, 0, 0);
CRGB TRAFFIC_YELLOW = CRGB(239, 83, 0);
CRGB TRAFFIC_GREEN = CRGB(0, 132, 5);

CRGB colors[I2C_PLAYERS] = {
  CRGB::Blue,
  CRGB::Red,
  CRGB::Green,
  CRGB::Orange,
};
PlayerState players[I2C_PLAYERS] = {
  PlayerState{},
  PlayerState{},
  PlayerState{},
  PlayerState{},
};
AsyncHTTPRequest reqPool[REQUESTS_IN_POOL] = {
  AsyncHTTPRequest{},
  AsyncHTTPRequest{},
  AsyncHTTPRequest{},
  AsyncHTTPRequest{},
  AsyncHTTPRequest{},
  AsyncHTTPRequest{},
  AsyncHTTPRequest{},
  AsyncHTTPRequest{},
  AsyncHTTPRequest{},
  AsyncHTTPRequest{},
  AsyncHTTPRequest{},
};
int reqPoolIdx = 0;

void setup() {
  randomSeed(analogRead(NOT_CONNECTED_PIN));
  Serial.begin(115200);
  confSetup();
  gnetSetup();
  trakSetup();
  netSetup();

  for (int i = 0; i < REQUESTS_IN_POOL; i++) {
    reqPool[i].onReadyStateChange(ignoreCallback);
  }

  for (int i = 0; i < I2C_PLAYERS; i++) {
    playerReset(players[i]);
    players[i].color = colors[i];
  }

  gnetScan();
  updatePlayerIds();
}

void loop() {
  bool doReset = trakUpdate();
  if (doReset || (!inGame && lightsStartMs == 0 && (millis() - lastWireScan) > WIRE_PING_MS)) {
    lastWireScan = millis();
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
// ---- PLAYERS
// ===============================================================

void playerPhysics(PlayerState &player) {
  if (!player.isConnected) {
    return;
  }
  if (player.finishMs > 0) {
    return; // they're done the race
  }

  for (int i = 0; i < player.unhandledPresses; i++) {
    player.velocity += PHYSICS_ACCL;
  }
  player.unhandledPresses = 0;

  if ((millis() - player.lastPhysics) < PHYSICS_MS) {
    return; // done with physics for now
  }
  player.lastPhysics = millis();

  if (player.velocity > PHYSICS_MAX_VELOCITY) {
    player.velocity = PHYSICS_MAX_VELOCITY;
  }

  // Move vehicle
  player.velocity -= player.velocity * PHYSICS_FRICTION;
  if (player.velocity < PHYSICS_MIN_VELOCITY) {
    player.velocity = PHYSICS_MIN_VELOCITY;
  }
  player.position += player.velocity;
  player.location = round(player.position); // int location
}

void playerReset(PlayerState &player) {
  player.length = 3;
  player.position = 0;
  player.location = 0;
  player.velocity = 0;
  player.finishMs = 0;
  player.lastLapFinishMs = 0;
  player.unhandledPresses = 0;
}

// ---------------------------------------------------------------
// ---- TRACK
// ===============================================================

void trakSetup() {
  FastLED.addLeds<WS2812B, LED_STRIP_PIN, GRB>(leds, STRIP_LENGTH * STRIP_COUNT);
  trakClear();
  trakRender();
}

// Primary loop
// ======================================

bool trakUpdate() {
  bool shouldReset = false;
  trakClear();
  btnUpdate();

  if (!inGame) {
    if (winnerNum != NO_WINNER) {
      long timeDiff = millis() - endMs;
      if (timeDiff < WINNER_SHOWN_MS) {
        trakDrawWinner();
      } else {
        winnerNum = NO_WINNER;
        endMs = 0;
      }
    } else if (lightsStartMs > 0) {
      leds[TRAFFIC_START - 0] = TRAFFIC_RED;
      leds[TRAFFIC_START - 1] = TRAFFIC_RED;

      if ((millis() - lightsStartMs) >= 1000) {
        leds[TRAFFIC_START - 2] = TRAFFIC_YELLOW;
        leds[TRAFFIC_START - 3] = TRAFFIC_YELLOW;
      }

      if ((millis() - lightsStartMs) >= 2000) {
        leds[TRAFFIC_START - 4] = TRAFFIC_GREEN;
        leds[TRAFFIC_START - 5] = TRAFFIC_GREEN;
      }

      if ((millis() - lightsStartMs) >= 2400) {
        shouldReset = true;
        inGame = true;
        lightsStartMs = 0;
        startTimeMs = millis();

        int nPlayers = 0;
        for (int i = 0; i < I2C_PLAYERS; i++) {
          playerReset(players[i]);
          if (players[i].isConnected) {
            nPlayers++;
          }
        }

        markGameStart(nPlayers);
      }
    } else if (btnDownTrigger) {
      lightsStartMs = millis();
      shouldReset = true;
      markGameIntro();
    }
  } else {
    trakUpdatePlayers();
    trakDrawPlayers();
  }

  trakRender();
  return shouldReset;
}

// Game functions
// ======================================

void trakUpdatePlayers() {
  bool allDone = true;
  for (int i = 0; i < I2C_PLAYERS; i++) {
    if (!players[i].isConnected) {
      continue;
    }
    if (players[i].finishMs > 0) {
      continue; // already finished all laps (don't adjust lap times)
    }

    int oldLaps = players[i].location / STRIP_LENGTH;
    playerPhysics(players[i]);
    int newLaps = players[i].location / STRIP_LENGTH;

    if (newLaps != oldLaps) {
      long lastLapMs = players[i].lastLapFinishMs;
      if (lastLapMs == 0) {
        lastLapMs = startTimeMs;
      }
      long lapTime = millis();
      markLapCompleted(i, newLaps, lapTime - lastLapMs);
      players[i].lastLapFinishMs = lapTime;
    }

    if ((players[i].location / STRIP_LENGTH) >= MAX_LAPS) {
      players[i].finishMs = millis();
      markAllLapsCompleted(i, players[i].finishMs - startTimeMs);
    }

    allDone = allDone && (players[i].finishMs > 0);
  }

  if (allDone) {
    inGame = false;
    endMs = millis();
    winnerNum = NO_WINNER;
    for (int i = 0; i < I2C_PLAYERS; i++) {
      if (!players[i].isConnected) {
        continue;
      }
      if (winnerNum == NO_WINNER || players[winnerNum].finishMs > players[i].finishMs) {
        winnerNum = i;
      }
    }
    markGameEnd(winnerNum);
  }
}

// Render functions
// ======================================

void trakClear() {
  for (int i = 0; i < (STRIP_LENGTH * STRIP_COUNT); i++) {
    leds[i] = CRGB::Black;
  }
}

void trakDrawWinner() {
  for (int i = 0; i < (STRIP_LENGTH * STRIP_COUNT); i++) {
    leds[i] = players[winnerNum].color;
  }
}

void trakDrawPlayers() {
  // Figure out where each player is an render them into the LEDs
  int positionMap[STRIP_LENGTH * STRIP_COUNT];
  for (int i = 0; i < (STRIP_LENGTH * STRIP_COUNT); i++) {
    positionMap[i] = 0;
  }
  for (int i = 0; i < I2C_PLAYERS; i++) {
    if (!players[i].isConnected) {
      continue;
    }
    if (players[i].finishMs > 0) {
      continue; // don't render: they're done
    }

    int startPos = players[i].location % STRIP_LENGTH;
    for (int j = 0; j < players[i].length; j++) {
      int targetLoc = startPos + j;
      int maxStripPos = STRIP_LENGTH * STRIP_COUNT;
      if (targetLoc >= maxStripPos) {
        targetLoc = (targetLoc - maxStripPos); // overrun
      }
      positionMap[targetLoc]++;
      leds[targetLoc] = CRGB(
        leds[targetLoc].r + players[i].color.r,
        leds[targetLoc].g + players[i].color.g,
        leds[targetLoc].b + players[i].color.b
      );
    }
  }

  // Mix colors
  for (int i = 0; i < (STRIP_LENGTH * STRIP_COUNT); i++) {
    int mapHeight = positionMap[i];
    if (mapHeight > 1) {
      leds[i] = CRGB(
        leds[i].r / mapHeight,
        leds[i].g / mapHeight,
        leds[i].b / mapHeight
      );
    }
  }
}

void trakRender() {
  FastLED.show();
}

// ---------------------------------------------------------------
// ---- GAME NET
// ===============================================================

void gnetSetup() {
  Wire.setClock(WIRE_SPEED);
  Wire.begin(SDA_PIN, SCL_PIN);
}

void gnetUpdate() {
  for (int i = 0; i < I2C_PLAYERS; i++) {
    if (!players[i].isConnected) {
      continue;
    }

    byte addr = I2C_PLAYER_START + i;
    Wire.requestFrom(addr, TOHOST_LENGTH);
    byte pos = 0;
    while (Wire.available()) {
      byte b = Wire.read();
      if (pos >= TOHOST_LENGTH) {
        continue;
      }

      if (pos == 0) {
        players[i].unhandledPresses += b;
      } else if (pos == 1) {
        // Secondary button not used
      }

      pos++;
    }
  }
}

void gnetScan() {
  for (int i = 0; i < I2C_PLAYERS; i++) {
    byte addr = I2C_PLAYER_START + i;
    Wire.beginTransmission(addr);
    byte err = Wire.endTransmission();
    if (err == 0) {
      Serial.print(i);
      Serial.println(" is connected to I2C");
      players[i].isConnected = true;
    } else {
      Serial.print(i);
      Serial.print(" failed to connect on I2C with error ");
      Serial.println(err);
      players[i].isConnected = false;
    }
  }
}

void gnetUpdateColor(int playerNum, byte r, byte g, byte b) {
  if (!players[playerNum].isConnected) {
    return;
  }

  Wire.beginTransmission(I2C_PLAYER_START + playerNum);
  byte msg[4] = {FROMHOST_ASSIGN, r, g, b};
  Wire.write(msg, 4);
  Wire.endTransmission();
}

void gnetResetController(int playerNum) {
  if (!players[playerNum].isConnected) {
    return;
  }

  Wire.beginTransmission(I2C_PLAYER_START + playerNum);
  byte msg[1] = {FROMHOST_RESET};
  Wire.write(msg, 1);
  Wire.endTransmission();
}

void gnetResetAll() {
  for (int i = 0; i < I2C_PLAYERS; i++) {
    gnetResetController(i);
  }
}

// ---------------------------------------------------------------
// ---- BUTTON
// ===============================================================

int btnLastState = BTN_OPEN;
int btnLastRead = BTN_OPEN;
int btnLastReadMs = 0;

void btnUpdate() {
  btnDownTrigger = false;
  btnUpTrigger = false;

  int val = digitalRead(BTN_PIN);
  if (val != btnLastRead) {
    btnLastRead = val;
    btnLastReadMs = millis();
  }
  if ((millis() - btnLastReadMs) > BTN_DEBOUNCE_MS) {
    if (val != btnLastState) {
      btnLastState = val;
      if (btnLastState == BTN_CLOSED) {
        btnPressed = true;
        btnDownTrigger = true;
      } else {
        btnPressed = false;
        btnUpTrigger = true;
      }
    }
  }
}

void btnUpdateBlocking() {
  btnUpdate();
  delay(BTN_DEBOUNCE_MS + 5);
  btnUpdate();
}

// ---------------------------------------------------------------
// ---- CONFIG
// ===============================================================

void confSetup() {
  EEPROM.begin(CONF_EEPROM_SIZE);
}

void confRead() {
  confReadString(CONF_SB_IP_ADDR, CONF_SB_IP_LEN, scoreboardIp);

  // Validation
  MatchState ms;
  ms.Target(scoreboardIp, CONF_SB_IP_LEN);
  if (ms.Match("[0-9]+.[0-9]+.[0-9]+.[0-9]+") != REGEXP_MATCHED) {
    Serial.print("Invalid scoreboard IP: ");
    Serial.println(scoreboardIp);
    strcpy(scoreboardIp, NO_SB_IP);
  }
  char str2[CONF_SB_IP_LEN] = "";
  int j = 0;
  for (int i = 0; i < CONF_SB_IP_LEN; i++) {
    char c = scoreboardIp[i];
    if ((c >= 48 && c <= 57) || c == 46) { // number or full stop
      str2[j] = c;
      j++;
    }
  }
  strcpy(scoreboardIp, str2);
  if (strcmp(scoreboardIp, "") == 0) {
    Serial.println("Scoreboard IP was empty - resetting");
    strcpy(scoreboardIp, NO_SB_IP);
  }

  // Debug
  Serial.print("Scoreboard IP: ");
  Serial.println(scoreboardIp);
}

void confWrite() {
  char str2[CONF_SB_IP_LEN] = "";
  int j = 0;
  for (int i = 0; i < CONF_SB_IP_LEN; i++) {
    char c = scoreboardIp[i];
    if ((c >= 48 && c <= 57) || c == 46) { // number or full stop
      str2[j] = c;
      j++;
    }
  }
  for (int i = 0; i < CONF_SB_IP_LEN; i++) {
    char c = str2[i];
    if (c <= 48 && c >= 57 && c != 46) { // not a number or full stop
      str2[i] = ' ';
    }
  }

  confWriteString(CONF_SB_IP_ADDR, CONF_SB_IP_LEN, str2);
  EEPROM.commit();
  confRead(); // read back for validation
}

void confClear() {
  strcpy(scoreboardIp, "");
  confWrite();
}

void confReadString(int addr, int len, char str[]) {
  for (int i = 0; i < len; i++) {
    str[i] = char(confReadByte(addr + i));
  }
}

void confWriteString(int addr, int len, char val[]) {
  for (int i = 0; i < len; i++) {
    confWriteByte(addr + i, val[i]);
  }
}

int confReadInt(int addr) {
  byte part1 = confReadByte(addr);
  byte part2 = confReadByte(addr + 1);

  return (part1 << 8) + part2;
}

void confWriteInt(int addr, int val) {
  confWriteByte(addr, val >> 8);
  confWriteByte(addr + 1, val & 0xFF);
}

byte confReadByte(int addr) {
  return EEPROM.read(addr);
}

void confWriteByte(int addr, byte val) {
  EEPROM.write(addr, val);
}

// ---------------------------------------------------------------
// ---- NETWORK
// ===============================================================

WiFiManager wm;
WiFiManagerParameter* scoreboardIPField;

void netSetup() {
  WiFi.mode(WIFI_STA);
  leds[NW_STATUS_LED] = CRGB(50, 5, 5);
  trakRender();

  btnUpdateBlocking();
  if (btnPressed) {
    Serial.println("WiFi reset button pressed - clearing settings");
    wm.resetSettings();
    confClear();
  }

  confRead();
  if (strcmp(scoreboardIp, "") == 0) {
    strcpy(scoreboardIp, NO_SB_IP);
  }
  scoreboardIPField = new WiFiManagerParameter(FIELD_SCOREBOARD_NAME, "Scoreboard IP", scoreboardIp, CONF_SB_IP_LEN, "placeholder=\"none\"");

  wm.addParameter(scoreboardIPField);
  wm.setSaveParamsCallback(netSaveWmParams);

  std::vector<const char *> menu = {"wifi", "param", "sep", "restart", "exit"};
  wm.setMenu(menu);

  bool worked = wm.autoConnect("LEDRacerGameBoard");
  if (!worked) {
    Serial.println("Failed to configure wifi");

    // fail loop
    while(true) {
      leds[NW_STATUS_LED] = CRGB::Red;
      trakRender();
      delay(500);
      leds[NW_STATUS_LED] = CRGB::Black;
      trakRender();
      delay(250);
    }
  }

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  netSaveWmParams(); // just in case

  if (scoreboardIp != NO_SB_IP) {
    Serial.println("Validating scoreboard details");
    IPAddress ip;
    if (!ip.fromString(scoreboardIp)) {
      Serial.println("Invalid IP");

      // fail loop
      while(true) {
        leds[NW_STATUS_LED] = CRGB::Red;
        trakRender();
        delay(500);
        leds[NW_STATUS_LED] = CRGB::Black;
        trakRender();
        delay(250);
      }
    }

    Serial.println("Ready to connect to scoreboard");
  }

  leds[NW_STATUS_LED] = CRGB::Black;
  trakRender();
}

void netSaveWmParams() {
  strcpy(scoreboardIp, scoreboardIPField->getValue());
  Serial.print("New scoreboard IP to write: ");
  Serial.println(scoreboardIp);
  confWrite();
}

// ---------------------------------------------------------------
// ---- HTTP
// ===============================================================

void markLapCompleted(int playerNum, int lap, long ms) {
  if (strcmp(scoreboardIp, NO_SB_IP) == 0) {
    return;
  }
  int reqIdx = reqPoolIdx;
  reqPoolIdx++;
  if (reqPoolIdx >= REQUESTS_IN_POOL) {
    reqPoolIdx = 0;
  }
  char url[128];
  sprintf(url, "http://%s:20304/lap_done?player=%d&lap=%d&time=%d", scoreboardIp, playerNum, lap, ms);
  if (!reqPool[reqIdx].open("POST", url)) {
    Serial.println("ERROR: markLapCompleted is a BAD REQUEST");
    return;
  }
  reqPool[reqIdx].send();
}

void markAllLapsCompleted(int playerNum, long ms) {
  if (strcmp(scoreboardIp, NO_SB_IP) == 0) {
    return;
  }
  int reqIdx = reqPoolIdx;
  reqPoolIdx++;
  if (reqPoolIdx >= REQUESTS_IN_POOL) {
    reqPoolIdx = 0;
  }
  char url[128];
  sprintf(url, "http://%s:20304/player_done?player=%d&time=%d", scoreboardIp, playerNum, ms);
  if (!reqPool[reqIdx].open("POST", url)) {
    Serial.println("ERROR: markAllLapsCompleted is a BAD REQUEST");
    return;
  }
  reqPool[reqIdx].send();
}

void markDNF(int playerNum) {
  if (strcmp(scoreboardIp, NO_SB_IP) == 0) {
    return;
  }
  int reqIdx = reqPoolIdx;
  reqPoolIdx++;
  if (reqPoolIdx >= REQUESTS_IN_POOL) {
    reqPoolIdx = 0;
  }
  char url[128];
  sprintf(url, "http://%s:20304/player_dnf?player=%d", scoreboardIp, playerNum);
  if (!reqPool[reqIdx].open("POST", url)) {
    Serial.println("ERROR: markDNF is a BAD REQUEST");
    return;
  }
  reqPool[reqIdx].send();
}

void markGameEnd(int winner) {
  if (strcmp(scoreboardIp, NO_SB_IP) == 0) {
    return;
  }
  int reqIdx = reqPoolIdx;
  reqPoolIdx++;
  if (reqPoolIdx >= REQUESTS_IN_POOL) {
    reqPoolIdx = 0;
  }
  char url[128];
  sprintf(url, "http://%s:20304/game_end?winner=%d", scoreboardIp, winner);
  if (!reqPool[reqIdx].open("POST", url)) {
    Serial.println("ERROR: markGameEnd is a BAD REQUEST");
    return;
  }
  reqPool[reqIdx].send();
}

void markGameStart(int numPlayers) {
  if (strcmp(scoreboardIp, NO_SB_IP) == 0) {
    return;
  }
  int reqIdx = reqPoolIdx;
  reqPoolIdx++;
  if (reqPoolIdx >= REQUESTS_IN_POOL) {
    reqPoolIdx = 0;
  }
  char url[128];
  sprintf(url, "http://%s:20304/game_start?players=%d", scoreboardIp, numPlayers);
  if (!reqPool[reqIdx].open("POST", url)) {
    Serial.println("ERROR: markGameStart is a BAD REQUEST");
    return;
  }
  reqPool[reqIdx].send();
}

void markGameIntro() {
  if (strcmp(scoreboardIp, NO_SB_IP) == 0) {
    return;
  }
  int reqIdx = reqPoolIdx;
  reqPoolIdx++;
  if (reqPoolIdx >= REQUESTS_IN_POOL) {
    reqPoolIdx = 0;
  }
  char url[64];
  sprintf(url, "http://%s:20304/game_intro", scoreboardIp);
  if (!reqPool[reqIdx].open("POST", url)) {
    Serial.println("ERROR: markGameIntro is a BAD REQUEST");
    return;
  }
  reqPool[reqIdx].send();
}

void ignoreCallback(void* optParm, AsyncHTTPRequest* request, int readyState) {
  // consume
}

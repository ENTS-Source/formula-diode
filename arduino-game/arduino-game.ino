#include <FastLED.h>
#include <EEPROM.h>
#include <Regexp.h>
#include <AsyncHTTPRequest_Generic.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <Wire.h>

// NodeMCU / ESP8266

#define CONF_SB_IP_ADDR 128 // stay out of the way of wifimanager
#define CONF_SB_IP_LEN 16 // string, fixed length
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

String scoreboardIp;
CRGB leds[STRIP_LENGTH * STRIP_COUNT];
long startTimeMs = 0;
long lightsStartMs = 0;
long endMs = 0;
int winnerNum = NO_WINNER;
bool inGame = false;

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
// ---- PLAYERS
// ===============================================================

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

void playerPhysics(PlayerState player) {
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

void playerReset(PlayerState player) {
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
    if (winner != NO_WINNER) {
      long timeDiff = millis() - endMs;
      if (timeDiff < WINNER_SHOWN_MS) {
        trakDrawWinner();
      } else {
        winner = NO_WINNER;
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
        leds[TRAFFIC_START - 2] = TRAFFIC_GREEN;
        leds[TRAFFIC_START - 3] = TRAFFIC_GREEN;
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
      Serial.print("@@ Calculating lap time for player: ");
      Serial.println(i);
      long lastLapMs = players[i].lastLapFinishMs;
      Serial.print("@@ Debug -- Game start time: ");
      Serial.println(startTimeMs);
      Serial.print("@@ Debug - Free Heap: ");
      Serial.println(ESP.getFreeHeap());
      if (lastLapMs == 0) {
        Serial.print("@@ Using game start time as last lap finish time: ");
        Serial.println(startTimeMs);
        lastLapMs = startTimeMs;
      } else {
        Serial.print("@@ Using last lap finish time: ");
        Serial.println(lastLapMs);
      }
      long lapTime = millis();
      Serial.print("@@ Current time: ");
      Serial.println(lapTime);
      Serial.print("@@ Lap time: ");
      Serial.println(lapTime - lastLapMs);
      Serial.println("-------------------------------------------------------------------------");
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
      positionMap[startPos + j]++;
      leds[startPos + j] = CRGB(
        leds[startPos + j].r + players[i].color.r,
        leds[startPos + j].g + players[i].color.g,
        leds[startPos + j].b + players[i].color.b,
      );
    }
  }

  // Mix colors
  for (int i = 0; i < (STRIP_LENGTH * STRIP_COUNT); i++) {
    int mapHeight = positionMap[i];
    if (mapHeight > 1) {
      leds = CRGB(
        leds[i].r / mapHeight,
        leds[i].g / mapHeight,
        leds[i].b / mapHeight,
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
        players[i].unhandledPresses = b;
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
      gnetPlayers[i].isConnected = true;
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
bool btnDownTrigger = false;
bool btnUpTrigger = false;
bool btnPressed = false;

void btnUpdate() {
  btnDownTrigger = false;
  btnUpTrigger = false;

  int val = digitalRead(BTN_PIN);
  if (val != btnLastRead) {
    btnLastRead = val;
    btnLastReadMs = millis();
  }
  if ((millis() - lastReadMs) > BTN_DEBOUNCE_MS) {
    if (val != btnLastState) {
      btnLastState = val;
      if (lastState == BTN_CLOSED) {
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
  scoreboardIp = confReadString(CONF_SB_IP_ADDR, CONF_SB_IP_LEN);

  // Validation
  MatchState ms;
  char sbipArr[CONF_SB_IP_LEN];
  scoreboardIp.toCharArray(sbipArr, CONF_SB_IP_LEN);
  ms.Target(sbipArr, CONF_SB_IP_LEN);
  if (ms.Match("[0-9]+.[0-9]+.[0-9]+.[0-9]+") != REGEXP_MATCHED) {
    Serial.println("Invalid scoreboard IP: " + scoreboardIp);
    scoreboardIp = NO_SB_IP;
  }
  scoreboardIp.trim();
  if (scoreboardIp.length() <= 0) {
    Serial.println("Scoreboard IP was empty - resetting");
    scoreboardIp = NO_SB_IP;
  }

  // Debug
  Serial.println("Scoreboard IP: " + scoreboardIp);
}

void confWrite() {
  String sbip = scoreboardIp;

  // Truncate if needed
  if (sbip.length() > CONF_SB_IP_LEN) {
    sbip = "";
    for (int i = 0; i < CONF_SB_IP_LEN; i++) {
      sbip += scoreboardIp[i];
    }
  }

  // Pad if needed
  while(sbip.length() < CONF_SB_IP_LEN) {
    sbip += " "; // add empty space
  }

  confWriteString(CONF_SB_IP_ADDR, sbip);
  EEPROM.commit();
  confRead(); // read back for validation
}

void confClear() {
  scoreboardIp = "";
  confWrite();
}

String confReadString(int addr, int len) {
  String val = "";
  for (int i = 0; i < len; i++) {
    val += char(confReadByte(addr + i));
  }
  return val;
}

void confWriteString(int addr, String val) {
  for (int i = 0; i < val.length(); i++) {
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
  String sbip = scoreboardIp;
  if (sbip == "") {
    sbip = NO_SB_IP;
  }
  scoreboardIPField = new WiFiManagerParameter(FIELD_SCOREBOARD_NAME, "Scoreboard IP", sbip.c_str(), CONF_SB_IP_LEN, "placeholder=\"none\"");

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
  scoreboardIp = String(scoreboardIPField->getValue());
  confWrite();
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

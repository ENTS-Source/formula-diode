#include <FastLED.h>
#include <EEPROM.h>
#include <Wire.h>

#include "Button.h"
#include "TrafficLight.h"

// Adafruit Feather RP2040 DVI

/*
TODO:
- Fix NO_SCOREBOARD to actually work (also, make it configurable via web)
- Add world editor (ideally via web: add feature button, left+right to move down the track, up+down to make bigger and smaller)
- Easy/Medium/Hard levels (switchable from front button?)
- Make LED strip length web config (with visual so it can be easily measured)
- Try to remove I2C dependency to deal with PCB flexing issues
- Interrupt screensaver with controller buttons? (undecided)
*/

// ==== Configuration/ ====

// ---- Pin configuration
#define BTN_PIN D25
#define NOT_CONNECTED_PIN D6 // used to seed random
#define LED_STRIP_PIN D5

// ---- LED configuration
#define RENDER_INTERVAL 14

// ---- Game configuration
#define MAX_LAPS 3 // TODO: Config val
#define STRIP_COUNT 1 // TODO: Support this being 2 (using logical strips)
#define MIN_STRIP_LENGTH 40
#define MAX_STRIP_LENGTH 600
#define INIT_PLAYER_LENGTH 3 // TODO: Support making players longer/smaller
#define WINNER_SHOWN_MS 2500
#define SCREENSAVER_WAIT_MS 120000 // 2 minutes
#define GAME_TIMEOUT_MS 30000 // 30 seconds, then finish the game by entering screensaver mode
#define COUNT_MODE_WAIT_MS 5000 // 5 seconds

// ---- Physics configuration
#define PHYSICS_ACCL 0.125 // Velocity added per button press
#define PHYSICS_FRICTION 0.017 // Velocity removed by not pressing button
#define GRAVITY_EFFECT 0.007 // Force of gravity for loops
#define SPEED_BOOST_FACTOR 0.3 // Added velocity for being in a speed boost
#define TAR_TRAP_FRICTION 0.18 // Added friction for being in a tar trap
#define TAR_TRAP_ACCL 0.06 // Velocity per button press while in a tar trap
#define CATCHUP_BOOST_ACCL 0.20 // Extra velocity per button press when the player is behind
#define PHYSICS_MAX_VELOCITY 800000 // Maximum speed of a player
#define PHYSICS_MIN_VELOCITY -2 // Minimum speed of a player. Note that negative values allow the player to fall "down" hills.
#define PHYSICS_MS 5 // Time between physics checks

// ---- Screensaver/autoplay configuration
#define AUTO_RAND_MIN 0
#define AUTO_RAND_MAX 1000
#define AUTO_THRESHOLD 50 // Note: debounce is typically 10ms, so 15/1000 is essentially saying "every 15ms, trigger"
#define AUTO_PRESSES_PER 1
#define AUTO_PLAYERS 2

#define CONF_EEPROM_SIZE 512
#define CONF_LENGTH_ADDR 128 // start a bit high to give wifimanager some room
#define TOHOST_LENGTH 2  // btn 1 presses & btn 2 presses, 2 bytes
#define FROMHOST_ASSIGN 0x10
#define FROMHOST_RESET 0x11
#define TRAFFIC_START 7 // address; +1 from bottom, for aesthetics. Must be at least 6
#define NO_WINNER -1
#define NW_STATUS_LED 4

#define GRAVITY_ENABLED 0b00000001
#define SPEED_BOOST_ENABLED 0b00000100
#define SLOPE_FORWARD 0b00000010
#define SLOPE_BACKWARD 0b00000000 // inverse of forward
#define TAR_TRAP_ENABLED 0b00001000

// ==== /Configuration ====

Button btn = Button(BTN_PIN);
TrafficLight light = TrafficLight();

int featuresRange[][3] = {
  // 2022
//  {272, 38, GRAVITY_ENABLED | SLOPE_BACKWARD},
//  {310, 35, GRAVITY_ENABLED | SLOPE_FORWARD},

  // 2025-1
//  {265, 5, SPEED_BOOST_ENABLED},
//  {45, 5, SPEED_BOOST_ENABLED},

  // 2025-2
  {249, 5, TAR_TRAP_ENABLED},
  {307, 5, SPEED_BOOST_ENABLED},
  {119, 5, SPEED_BOOST_ENABLED},
//  {11, 15, SPEED_BOOST_ENABLED},
};

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

int stripLength = MIN_STRIP_LENGTH;
CRGB leds[MAX_STRIP_LENGTH * STRIP_COUNT];
byte stripMap[MAX_STRIP_LENGTH * STRIP_COUNT];
long startTimeMs = 0;
long endMs = 0;
long lastGameEnd = 0;
int winnerNum = NO_WINNER;
bool inGame = false;
bool isAutomatedGame = false;
long lastWireScan = 0;
long lastRender = 0;
long lastPress = 0;
bool countingMode = false;
long countModeStartTimeout = 0;
long lastCountModeChange = 0;

CRGB SPEED_BOOST_COLOR = CRGB(255, 170, 0);
CRGB TAR_TRAP_COLOR = CRGB(46, 0, 74);

void setup() {
  randomSeed(analogRead(NOT_CONNECTED_PIN));
  Serial.begin(115200);
  gnetSetup();
  confSetup();
  ledSetup(false);
  trakSetup();

  for (int i = 0; i < I2C_PLAYERS; i++) {
    playerReset(players[i]);
    players[i].color = colors[i];
    Serial.print("Player ");
    Serial.print(i);
    Serial.print(" is ");
    printColor(colors[i]);
    Serial.println();
  }

  gnetScan();
  gnetResetAll();
  updatePlayerIds();

  // Turn on builtin LED to indicate power/finished setup
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
  bool doReset = trakUpdate();
  if (doReset || ((!inGame || isAutomatedGame) && !light.isRunning() && (millis() - lastWireScan) > WIRE_PING_MS)) {
    lastWireScan = millis();
    gnetScan();
    gnetResetAll();
    updatePlayerIds();
  }
  gnetUpdate();
  if (countingMode) {
    int newLength = stripLength;
    lastGameEnd = millis(); // avoid screensaver
    newLength += players[0].unhandledPresses;
    newLength -= players[1].unhandledPresses;
    // Serial.print("Strip length: ");
    // Serial.print(stripLength);
    // Serial.print("     New strip length: ");
    // Serial.print(newLength);
    // Serial.print("     player0 presses: ");
    // Serial.print(players[0].unhandledPresses);
    // Serial.print("     player1 presses: ");
    // Serial.println(players[1].unhandledPresses);
    players[0].unhandledPresses = 0;
    players[1].unhandledPresses = 0;
    if (newLength != stripLength) {
      confWriteInt(CONF_LENGTH_ADDR, newLength);
      confWrite();
      ledSetup(true); // this will clear the old length and new length for us
      trakCountStrip();
    }
    if (btn.isPressed && (millis() - lastCountModeChange) > COUNT_MODE_WAIT_MS) {
      Serial.println("Exiting counting mode");
      countingMode = false;
      countModeStartTimeout = millis();
      lastCountModeChange = millis();
    }
  } else if (!inGame && !countingMode && (millis() - lastCountModeChange) > COUNT_MODE_WAIT_MS) {
    if (countModeStartTimeout == 0 && btn.isDownTrigger) {
      Serial.println("Starting count mode wait");
      countModeStartTimeout = millis();
    } else if (!btn.isPressed) {
      // Serial.println("Resetting count mode wait");
      countModeStartTimeout = 0;
    } else if ((millis() - countModeStartTimeout) > COUNT_MODE_WAIT_MS) {
      Serial.println("Entering counting mode");
      // countingMode = true;
      lastCountModeChange = millis();
    }
  }
}

void updatePlayerIds() {
  for (int i = 0; i < I2C_PLAYERS; i++) {
    gnetUpdateColor(i, players[i].color.r, players[i].color.g, players[i].color.b);
  }
}

void ledSetup(bool andClear) {
  if (andClear) {
    trakClear(); // clear old size
    trakRender();
  }

  stripLength = confReadInt(CONF_LENGTH_ADDR);
  if (stripLength <= MIN_STRIP_LENGTH) {
    stripLength = MIN_STRIP_LENGTH;
  }
  if (stripLength > MAX_STRIP_LENGTH) { // also avoids empty EEPROM having 0xFFFF values (65535)
    stripLength = MAX_STRIP_LENGTH;
  }
  Serial.print("Strip length: ");
  Serial.println(stripLength);

  if (andClear) {
    trakClear(); // clear new size (if larger/different)
    trakRender();
  }

  for (int i = 0; i < (stripLength * STRIP_COUNT); i++) {
    stripMap[i] = 0;
  }

  int featureRows = sizeof(featuresRange) / sizeof(featuresRange[0]);
  for (int i = 0; i < featureRows; i++) {
    int startIdx = featuresRange[i][0];
    int lengthIdx = featuresRange[i][1];
    byte flags = featuresRange[i][2];
    for (int j = startIdx; j <= (startIdx + lengthIdx); j++) {
      if (j > (stripLength * STRIP_COUNT)) {
        continue;
      }
      stripMap[j] = flags;
    }
  }

  trakSetupForDrawPlayers();
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

  // Check if player is behind
  bool behind = false;
  uint8_t numFinished = 0;
  for (int i = 0; i < I2C_PLAYERS; i++) {
    PlayerState pl = players[i];
    if (pl.finishMs > 0 || !pl.isConnected) {
      numFinished++;
      continue; // skip players that are already done
    }
    int diff = player.location - pl.location;
    if (diff <= (-1 * stripLength)) { // 1 full lap behind
      behind = true;
    }
  }
  // Serial.print("Behind? ");
  // Serial.print(behind);
  if (!behind && (I2C_PLAYERS - numFinished) == 1) {
    behind = true;
  }
  // Serial.print("   Still? ");
  // Serial.println(behind);

  int relPos = (player.location + player.length) % stripLength;
  bool tarTrap = (stripMap[relPos] & TAR_TRAP_ENABLED) != 0;
  for (int i = 0; i < player.unhandledPresses; i++) {
    if (behind) {
      player.velocity += CATCHUP_BOOST_ACCL;
    }
    lastPress = millis();
    if (tarTrap) {
      player.velocity += TAR_TRAP_ACCL;
    } else {
      player.velocity += PHYSICS_ACCL;
    }
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
  float friction = PHYSICS_FRICTION;
  if (tarTrap) {
    friction = TAR_TRAP_FRICTION;
  }
  player.velocity -= player.velocity * friction;
  if (player.velocity < PHYSICS_MIN_VELOCITY) {
    player.velocity = PHYSICS_MIN_VELOCITY;
  }
  player.position += player.velocity;
  player.location = round(player.position); // int location

  // Gravity, front wheel drive
  relPos = (player.location + player.length) % stripLength;
  if ((stripMap[relPos] & GRAVITY_ENABLED) != 0) {
    float effect = GRAVITY_EFFECT;
    if ((stripMap[relPos] & SLOPE_FORWARD) == 0) {
      // means we're sloping backwards
      effect = effect * -1;
    }
    player.velocity += effect;
    player.position += player.velocity;
  }
  if ((stripMap[relPos] & SPEED_BOOST_ENABLED) != 0) {
    player.velocity += SPEED_BOOST_FACTOR;
    player.position += player.velocity;
//    stripMap[relPos] = (stripMap[relPos] & ~SPEED_BOOST_ENABLED);
  }

  // Last minute check on position
  if (player.position < 0) {
    player.position = 0;
  }
  player.location = round(player.position);
}

// ---------------------------------------------------------------
// ---- TRACK
// ===============================================================

void trakSetup() {
  trakClear();
  trakRender();
  FastLED.addLeds<WS2812B, LED_STRIP_PIN, GRB>(leds, MAX_STRIP_LENGTH);
}

// Primary loop
// ======================================

bool trakUpdate() {
  bool shouldReset = false;
  btn.update();

  if (countingMode) {
    return shouldReset; // nothing to do except update the button
  }

  trakClear();

  if (!inGame) {
    if (winnerNum != NO_WINNER) {
      long timeDiff = millis() - endMs;
      if (timeDiff < WINNER_SHOWN_MS) {
        trakDrawWinner();
      } else {
        winnerNum = NO_WINNER;
        endMs = 0;
        lastGameEnd = millis();

        if (isAutomatedGame) {
          lastGameEnd = 0; // allow another automated game to start right away
          shouldReset = true;
        }
      }
    } else if (light.getMsRemaining() > 0) {
      if (isAutomatedGame) {
        leds[TRAFFIC_START + TRAFFIC_LIGHT_FEATURE_LENGTH + 2] = CRGB::Purple;
        leds[TRAFFIC_START + TRAFFIC_LIGHT_FEATURE_LENGTH + 1] = CRGB::Purple;
      }

      CRGB lightsFeature[TRAFFIC_LIGHT_FEATURE_LENGTH];
      light.render(lightsFeature);
      for (byte i = 0; i < TRAFFIC_LIGHT_FEATURE_LENGTH; i++) {
        leds[TRAFFIC_START + i] = lightsFeature[i];
      }
    } else if (light.isFinished()) {
      shouldReset = true;
      inGame = true;
      // lightsStartMs = 0;
      startTimeMs = millis();

      int nPlayers = 0;
      for (int i = 0; i < I2C_PLAYERS; i++) {
        playerReset(players[i]);
        if (players[i].isConnected) {
          nPlayers++;
        }
      }
    } else if ((millis() - lastGameEnd) > SCREENSAVER_WAIT_MS) {
      isAutomatedGame = true;
      Serial.println("Screensaver");

      light.resetAndStart();
      shouldReset = true;
    }
  } else {
    trakUpdatePlayers();
    trakDrawBoosts();
    trakDrawPlayers();
    if (!isAutomatedGame && (millis() - lastPress) > GAME_TIMEOUT_MS) {
      isAutomatedGame = true;
      lastGameEnd = 0; // enter screensaver immediately
    }
  }

  if (btn.isUpTrigger) {
    // Interrupt current game and start a new one
    isAutomatedGame = false;
    light.resetAndStart();
    shouldReset = true;
    lastPress = millis();
    inGame = false;
  }

  if ((millis() - lastRender) >= RENDER_INTERVAL) {
    lastRender = millis();
    trakRender();
  }
  return shouldReset;
}

// Game functions
// ======================================

void trakUpdatePlayers() {
  bool allDone = true;
  for (int i = 0; i < I2C_PLAYERS; i++) {
    if (isAutomatedGame) {
      players[i].isConnected = (i < AUTO_PLAYERS);
    }

    if (!players[i].isConnected) {
      continue;
    }
    if (players[i].finishMs > 0) {
      continue; // already finished all laps (don't adjust lap times)
    }

    if (isAutomatedGame && random(AUTO_RAND_MIN, AUTO_RAND_MAX) <= AUTO_THRESHOLD) {
      players[i].unhandledPresses += AUTO_PRESSES_PER;
    }

    int oldLaps = players[i].location / stripLength;
    playerPhysics(players[i]);
    int newLaps = players[i].location / stripLength;

    if (newLaps != oldLaps) {
      long lastLapMs = players[i].lastLapFinishMs;
      if (lastLapMs == 0) {
        lastLapMs = startTimeMs;
      }
      long lapTime = millis();
      players[i].lastLapFinishMs = lapTime;
    }

    if ((players[i].location / stripLength) >= MAX_LAPS) {
      players[i].finishMs = millis();
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
  }
}

// Render functions
// ======================================

void trakClear() {
  for (int i = 0; i < (stripLength * STRIP_COUNT); i++) {
    leds[i] = CRGB::Black;
  }
}

void trakCountStrip() {
  for (int i = 0; i < (stripLength * STRIP_COUNT); i++) {
    leds[i] = SPEED_BOOST_COLOR;
  }
  trakRender();
}

void trakDrawWinner() {
  for (int i = 0; i < (stripLength * STRIP_COUNT); i++) {
    leds[i] = players[winnerNum].color;
  }
}

void printColor(CRGB color) {
  Serial.print(color.r);
  Serial.print(",");
  Serial.print(color.g);
  Serial.print(",");
  Serial.print(color.b);
}

uint8_t* positionMap = nullptr;
bool* didTrapOverlay = nullptr;

void trakSetupForDrawPlayers() {
  if (positionMap != nullptr) {
    delete[] positionMap;
    positionMap = nullptr;
  }
  if (didTrapOverlay != nullptr) {
    delete[] didTrapOverlay;
    didTrapOverlay = nullptr;
  }
  positionMap = new uint8_t[stripLength * STRIP_COUNT];
  didTrapOverlay = new bool[stripLength * STRIP_COUNT];
}

void trakDrawPlayers() {
  // Figure out where each player is an render them into the LEDs
  for (int i = 0; i < (stripLength * STRIP_COUNT); i++) {
    positionMap[i] = 0;
    didTrapOverlay[i] = false;
  }
  for (int i = 0; i < I2C_PLAYERS; i++) {
    if (!players[i].isConnected) {
      continue;
    }
    if (players[i].finishMs > 0) {
      continue; // don't render: they're done
    }

    int startPos = players[i].location % stripLength;
    for (int j = 0; j < players[i].length; j++) {
      int targetLoc = startPos + j;
      int maxStripPos = stripLength * STRIP_COUNT;
      if (targetLoc >= maxStripPos) {
        targetLoc = (targetLoc - maxStripPos); // overrun
      }
      positionMap[targetLoc]++;
//      Serial.print("@@ Draw ");
//      Serial.print(targetLoc);
//      Serial.print(" curr:");
//      printColor(leds[targetLoc]);
//      Serial.print(" plyr:");
//      printColor(players[i].color);
        if ((stripMap[targetLoc] & TAR_TRAP_ENABLED) != 0 && !didTrapOverlay[targetLoc]) {
          leds[targetLoc] = CRGB(0, 0, 0);
          didTrapOverlay[targetLoc] = true;
        }
        leds[targetLoc] = CRGB(
          leds[targetLoc].r + players[i].color.r,
          leds[targetLoc].g + players[i].color.g,
          leds[targetLoc].b + players[i].color.b
        );
//      Serial.print(" res:");
//      printColor(leds[targetLoc]);
//      Serial.println();
    }
  }

  // Mix colors
  for (int i = 0; i < (stripLength * STRIP_COUNT); i++) {
    uint8_t mapHeight = positionMap[i];
    if (mapHeight > 1) {
      leds[i] = CRGB(
        leds[i].r / mapHeight,
        leds[i].g / mapHeight,
        leds[i].b / mapHeight
      );
    }
  }
}

void trakDrawBoosts() {
  for (int i = 0; i < (stripLength * STRIP_COUNT); i++) {
    if ((stripMap[i] & SPEED_BOOST_ENABLED) != 0) {
      leds[i] = SPEED_BOOST_COLOR;
    }
    if ((stripMap[i] & TAR_TRAP_ENABLED) != 0) {
      leds[i] = TAR_TRAP_COLOR;
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
  Wire.begin(); // no address - I2C controller
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
// ---- CONFIG
// ===============================================================

void confSetup() {
  EEPROM.begin(CONF_EEPROM_SIZE);
}

void confRead() {
}

void confWrite() {
  EEPROM.commit();
}

void confClear() {
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



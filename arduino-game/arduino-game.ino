#include <FastLED.h>
#include <EEPROM.h>
#include <Regexp.h>
#include <WiFiManager.h>
#include <Wire.h>
#include <ArduinoOTA.h>

// NodeMCU / ESP8266

/*
TODO:
- Fix NO_SCOREBOARD to actually work (also, make it configurable via web)
- Add world editor (ideally via web: add feature button, left+right to move down the track, up+down to make bigger and smaller)
- Easy/Medium/Hard levels (switchable from front button?)
- Make LED strip length web config (with visual so it can be easily measured)
- Try to remove I2C dependency to deal with PCB flexing issues
- Interrupt screensaver with controller buttons? (undecided)
- Tar trap assist for kids / general kids boost (slow button presses consistently make the game un-fun)
*/

#define BTN_PIN D5 // TODO: Will we need to support multiple buttons?
#define SDA_PIN D2
#define SCL_PIN D1
#define WIRE_SPEED 100000
#define I2C_PLAYERS 4
#define STRIP_COUNT 1 // TODO: Support this being 2 (using logical strips)
#define MAX_LAPS 3 // TODO: Config val
#define LED_STRIP_PIN D4
//#define STRIP_LENGTH 325
//#define STRIP_LENGTH 385
#define WINNER_SHOWN_MS 2500
#define INIT_PLAYER_LENGTH 3
#define PHYSICS_ACCL 0.125 // Velocity added per button press
#define PHYSICS_FRICTION 0.017
#define GRAVITY_EFFECT 0.007
#define SPEED_BOOST_FACTOR 0.3
#define TAR_TRAP_FRICTION 0.18
#define TAR_TRAP_ACCL 0.06
#define PHYSICS_MAX_VELOCITY 275000
#define PHYSICS_MIN_VELOCITY -2
#define PHYSICS_MS 5 // Time between physics checks
#define SCREENSAVER_WAIT_MS 120000 // 2 minutes
#define GAME_TIMEOUT_MS 30000 // 30 seconds
#define COUNT_MODE_WAIT_MS 5000 // 5 seconds
#define AUTO_RAND_MIN 0
#define AUTO_RAND_MAX 1000
#define AUTO_THRESHOLD 50 // Note: debounce is typically 10ms, so 15/1000 is essentially saying "every 15ms, trigger"
#define AUTO_PRESSES_PER 1
#define AUTO_PLAYERS 2
#define RENDER_INTERVAL 14

#define NOT_CONNECTED_PIN D6
#define CONF_EEPROM_SIZE 512
#define CONF_LENGTH_ADDR 128 // start a bit high to give wifimanager some room
#define BTN_OPEN HIGH
#define BTN_CLOSED LOW
#define BTN_DEBOUNCE_MS 10
#define I2C_PLAYER_START 0 // address
#define TOHOST_LENGTH 2  // btn 1 presses & btn 2 presses, 2 bytes
#define FROMHOST_ASSIGN 0x10
#define FROMHOST_RESET 0x11
#define TRAFFIC_START 7 // address; +1 from bottom, for aesthetics. Must be at least 6
#define NO_WINNER -1
#define NW_STATUS_LED 4
#define WIRE_PING_MS 5000
#define GRAVITY_ENABLED B00000001
#define SPEED_BOOST_ENABLED B00000100
#define SLOPE_FORWARD B00000010
#define SLOPE_BACKWARD B00000000 // inverse of forward
#define TAR_TRAP_ENABLED B00001000

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

int stripLength = 0;
CRGB* leds = nullptr;
byte* stripMap = nullptr;
long startTimeMs = 0;
long lightsStartMs = 0;
long endMs = 0;
long lastGameEnd = 0;
int winnerNum = NO_WINNER;
bool inGame = false;
bool isAutomatedGame = false;
bool btnDownTrigger = false;
bool btnUpTrigger = false;
bool btnPressed = false;
long lastWireScan = 0;
long lastRender = 0;
long lastPress = 0;
bool countingMode = false;
long countModeStartTimeout = 0;

CRGB TRAFFIC_RED = CRGB(255, 0, 0);
CRGB TRAFFIC_YELLOW = CRGB(239, 83, 0);
CRGB TRAFFIC_GREEN = CRGB(0, 132, 5);

CRGB SPEED_BOOST_COLOR = CRGB(255, 170, 0);
CRGB TAR_TRAP_COLOR = CRGB(46, 0, 74);

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

void setupLeds() {
  stripLength = confReadInt(CONF_LENGTH_ADDR);
  if (stripLength <= 0) {
    stripLength = 1;
  }
  leds = new CRGB[stripLength * STRIP_COUNT];
  stripMap = new byte[stripLength * STRIP_COUNT];
  Serial.print("Strip length: ");
  Serial.println(stripLength);

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
}

void setup() {
  randomSeed(analogRead(NOT_CONNECTED_PIN));
  Serial.begin(115200);
  gnetSetup();
  confSetup();
  setupLeds();
  trakSetup();
  netSetup();

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
}

void loop() {
  ArduinoOTA.handle();

  bool doReset = trakUpdate();
  if (doReset || ((!inGame || isAutomatedGame) && lightsStartMs == 0 && (millis() - lastWireScan) > WIRE_PING_MS)) {
    lastWireScan = millis();
    gnetScan();
    gnetResetAll();
    updatePlayerIds();
  }
  gnetUpdate();
  if (countingMode) {
    int lenBefore = stripLength;
    lastGameEnd = millis(); // avoid screensaver
    stripLength += players[0].unhandledPresses;
    stripLength -= players[1].unhandledPresses;
    players[0].unhandledPresses = 0;
    players[1].unhandledPresses = 1;
    if (lenBefore != stripLength) {
      confWriteInt(CONF_LENGTH_ADDR, stripLength);
      setupLeds();
      trakCountStrip();
    }
    if (btnPressed && (countModeStartTimeout == 0 || (millis() - countModeStartTimeout) >= COUNT_MODE_WAIT_MS * 2)) {
      Serial.println("Exiting counting mode");
      countingMode = false;
    }
  } else if (!inGame && !countingMode) {
    if (countModeStartTimeout == 0 && btnDownTrigger) {
      Serial.println("Starting count mode wait");
      countModeStartTimeout = millis();
    } else if (!btnPressed) {
      // Serial.println("Resetting count mode wait");
      countModeStartTimeout = 0;
    } else if ((millis() - countModeStartTimeout) > COUNT_MODE_WAIT_MS) {
      Serial.println("Entering counting mode");
      countingMode = true;
    }
  }
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

  int relPos = (player.location + player.length) % stripLength;
  bool tarTrap = (stripMap[relPos] & TAR_TRAP_ENABLED) != 0;
  for (int i = 0; i < player.unhandledPresses; i++) {
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

void playerReset(PlayerState &player) {
  player.length = INIT_PLAYER_LENGTH;
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
  FastLED.addLeds<WS2812B, LED_STRIP_PIN, GRB>(leds, stripLength * STRIP_COUNT);
  trakClear();
  trakRender();
}

// Primary loop
// ======================================

bool trakUpdate() {
  bool shouldReset = false;
  btnUpdate();

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
    } else if (lightsStartMs > 0) {
      if (isAutomatedGame) {
        leds[TRAFFIC_START + 6] = CRGB::Purple;
        leds[TRAFFIC_START + 5] = CRGB::Purple;
      }
      
      leds[TRAFFIC_START - 0] = TRAFFIC_RED;
      leds[TRAFFIC_START - 1] = TRAFFIC_RED;

      if ((millis() - lightsStartMs) >= 1000) {
        leds[TRAFFIC_START - 2] = TRAFFIC_YELLOW;
        leds[TRAFFIC_START - 3] = TRAFFIC_YELLOW;
      }

      if ((millis() - lightsStartMs) >= 2000) {
        for (int i = 0; i < 6; i++) {
          leds[TRAFFIC_START - i] = TRAFFIC_GREEN;
        }
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
      }
    } else if (btnDownTrigger) {
      isAutomatedGame = false;
      
      lightsStartMs = millis();
      shouldReset = true;
      lastPress = millis();
    } else if ((millis() - lastGameEnd) > SCREENSAVER_WAIT_MS) {
      isAutomatedGame = true;
      Serial.println("Screensaver");
      
      lightsStartMs = millis();
      shouldReset = true;
    }
  } else {
    if (isAutomatedGame && btnUpTrigger) {
      lastGameEnd = millis(); // ensure another game can't start right away

      // force end the game
      endMs = 0;
      inGame = false;
      winnerNum = NO_WINNER;
    } else {
      trakUpdatePlayers();
      trakDrawBoosts();
      trakDrawPlayers();
    }
    if (!isAutomatedGame && (millis() - lastPress) > GAME_TIMEOUT_MS) {
      isAutomatedGame = true;
      lastGameEnd = 0; // enter screensaver immediately
    }
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

void trakDrawPlayers() {
  // Figure out where each player is an render them into the LEDs
  int positionMap[stripLength * STRIP_COUNT];
  bool didTrapOverlay[stripLength * STRIP_COUNT];
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

// ---------------------------------------------------------------
// ---- NETWORK
// ===============================================================

WiFiManager wm;

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

  std::vector<const char *> menu = {"wifi", "sep", "restart", "exit"};
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

  // ArduinoOTA.setPassword("SafetyFirst!");
  ArduinoOTA.onStart([]() {
    Serial.println("OTA update started");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA update complete");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    unsigned int percent = total == 0 ? 0 : static_cast<unsigned int>(
      (static_cast<uint64_t>(progress) * 100) / 100
    );
    Serial.printf("OTA progress %u%%", percent);
    Serial.println();
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error: %u - ", error);
    switch(error) {
      case OTA_AUTH_ERROR:
        Serial.println("auth error");
        break;
      case OTA_BEGIN_ERROR:
        Serial.println("begin error");
        break;
      case OTA_CONNECT_ERROR:
        Serial.println("connect error");
        break;
      case OTA_RECEIVE_ERROR:
        Serial.println("receive error");
        break;
      case OTA_END_ERROR:
        Serial.println("end error");
        break;
      default:
        Serial.println("unknown");
        break;
    }
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready");

  leds[NW_STATUS_LED] = CRGB::Black;
  trakRender();
}


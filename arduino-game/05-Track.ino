#include <FastLED.h>

#define LED_STRIP_PIN D4
#define STRIP_LENGTH 50
#define STRIP_COUNT 1 // TODO: Support this being 2 (using logical strips)
#define MAX_LAPS 3 // TODO: Config val
#define TRAFFIC_START 7 // +1 from bottom, for aesthetics. Must be at least 6
#define WINNER_SHOWN_MS 2500
#define NO_WINNER -1

CRGB leds[STRIP_LENGTH * STRIP_COUNT];
long startTimeMs = 0;
long lightsStartMs = 0;
long endMs = 0;
int winnerNum = NO_WINNER;
bool inGame = false;

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

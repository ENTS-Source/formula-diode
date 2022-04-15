#ifndef TRACK_H
#define TRACK_H

#include <Arduino.h>
#include <FastLED.h>
#include "Button.h"
#include "PlayerController.h"
#include "Config.h"

#define LED_STRIP_PIN D4
#define STRIP_LENGTH 50
#define STRIP_COUNT 1 // TODO: Support this being 2 (using logical strips)
#define MAX_LAPS 3 // TODO: Config val
#define TRAFFIC_START 7 // +1 from bottom, for aesthetics. Must be at least 6
#define WINNER_SHOWN_MS 2500

extern CRGB TRAFFIC_RED;
extern CRGB TRAFFIC_YELLOW;
extern CRGB TRAFFIC_GREEN;

class Track {
  private:
    // Board state
    Config* config;
    PlayerController* players[MAX_PLAYERS];
    CRGB leds[STRIP_LENGTH * STRIP_COUNT];
    unsigned long startTimeMs;
    bool inGame;
    unsigned long lightsStartMs;
    unsigned long endMs;
    PlayerController* winner;

    // Physics / game engine
    void updatePlayers();

    // Rendering
    void clearStrip();
    void drawPlayers();
    void drawWinner();
  
  public:
    // Board state
    Button* startBtn;

    // Constructors and functions
    Track(byte startPin, PlayerController* players[], Config* config);
    void update();
    void setLed(int i, CRGB color); // use sparingly
    void render();
};

#endif

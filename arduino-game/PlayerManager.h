#ifndef PLAYER_MANAGER_H
#define PLAYER_MANAGER_H

#include <Arduino.h>
#include <FastLED.h>
#include <Wire.h>

#define MAX_PLAYERS 4
#define INITIAL_PLAYER_LENGTH 3

#define WIRE_SPEED 100000
#define WIRE_PING_MS 5000

class PlayerManager {
  private:
    Player players[MAX_PLAYERS];
    long lastScanMs;

  public:

    PlayerManager();
    void update();
};

class Player {
  private:
    byte id;
    byte addr;

    byte length;
    int location;
    int velocity;
    long finishMs;
    long lastLapFinishMs;
    int unhandledPresses;

  public:
    bool isConnected;

    Player(byte id);
    void reset();
    void physics();
};

#endif
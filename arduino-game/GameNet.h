#ifndef GAMENET_H
#define GAMENET_H

#include <Arduino.h>
#include <Wire.h>

#define TOHOST_LENGTH 2  // btn 1 presses & btn 2 presses, 2 bytes
#define FROMHOST_ASSIGN 0x10
#define FROMHOST_RESET 0x11

#define I2C_PLAYERS 4 // Must match Config MAX_PLAYERS

#define I2C_PLAYER_START 0

#define FLAG_RESET_STATE 0x01

class GameNet {
  private:
    // Shared vars
    bool isHost;

    // Game host vars
    byte playerStates[I2C_PLAYERS][TOHOST_LENGTH];
    bool connectedPlayers[I2C_PLAYERS];

    // Controller vars
    // (none)

    // Game host functions
    // (none)

    // Controller functions
    // (none)
    
  public:
    // Game host vars
    // (none)

    // Controller vars
    byte idR;
    byte idG;
    byte idB;
    byte acclBtnPresses;    // Incremented externally
    byte secondBtnPresses;  // Incremented externally
    byte flags;

    // Setup / constructors
    GameNet(byte sdaPin, byte sclPin);
    GameNet(int playerNum); // playerNum is the address

    // Game host functions
    void update();
    void scan();
    bool populateState(int playerNum, byte target[TOHOST_LENGTH]);
    void updateColor(int playerNum, byte r, byte g, byte b);
    void resetController(int playerNum);
    void resetAll();

    // Controller functions
    bool checkResetFlag();
    bool isPlayerConnected(int playerNum);
};

#endif

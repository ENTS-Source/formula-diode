#include "GameNet.h"

// Used for interrupts from Wire
GameNet* gamenetInstance;

void onWireReceive(int bytes) {
  byte msg[bytes];
  for (int i = 0; i < bytes; i++) {
    msg[i] = Wire.read();
  }

  Serial.print("I2C MSG: ");
  Serial.println(msg[0], HEX);

  if (msg[0] == FROMHOST_ASSIGN) {
    gamenetInstance->idR = msg[1];
    gamenetInstance->idG = msg[2];
    gamenetInstance->idB = msg[3];
  } else if (msg[0] == FROMHOST_RESET) {
    gamenetInstance->flags = gamenetInstance->flags | FLAG_RESET_STATE;
  }
}

void onWireRequest() {
  byte msg[] = {gamenetInstance->acclBtnPresses, gamenetInstance->secondBtnPresses};
  gamenetInstance->acclBtnPresses = 0;
  gamenetInstance->secondBtnPresses = 0;
  Wire.write(msg, 2);
}

// CLASS DEFINITION HERE
// =================================

//GameNet::GameNet(byte sdaPin, byte sclPin) {
//  gamenetInstance = this;
//  
//  Wire.setClock(100000);
//  Wire.begin(sdaPin, sclPin);
//  this->isHost = true;
//  for (int i = 0; i < I2C_PLAYERS; i++) {
//    for (int j = 0; j < TOHOST_LENGTH; j++) {
//      this->playerStates[i][j] = 0;
//    }
//    this->connectedPlayers[i] = false;
//  }
//}

GameNet::GameNet(int playerNum) {
  gamenetInstance = this;
  
  Wire.setClock(100000);
  Wire.begin(playerNum);
  Wire.onReceive(onWireReceive);
  Wire.onRequest(onWireRequest);
  this->isHost = false;
  this->idR = 0x00;
  this->idG = 0x00;
  this->idB = 0x00;
  this->acclBtnPresses = 0;
  this->secondBtnPresses = 0;
}

void GameNet::update() {
  if (!this->isHost) {
    return;
  }

  // TODO: Broadcast ID state to controllers

  for (int i = 0; i < I2C_PLAYERS; i++) {
    if (!this->connectedPlayers[i]) {
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
      this->playerStates[i][pos] = b;
      pos++;
    }
  }
}

void GameNet::scan() {
  if (!this->isHost) {
    return;
  }
  
  for (byte i = 0; i < I2C_PLAYERS; i++) {
    byte addr = I2C_PLAYER_START + i;
    Wire.beginTransmission(addr);
    byte err = Wire.endTransmission();
    if (err == 0) {
      Serial.print(i);
      Serial.println(" is connected to I2C");
      this->connectedPlayers[i] = true;
    } else {
      Serial.print(i);
      Serial.print(" failed to connect with error ");
      Serial.print(err);
      Serial.println(" on I2C interface");
    }
  }
}

bool GameNet::populateState(int playerNum, byte target[TOHOST_LENGTH]) {
  if (!this->isHost || !this->connectedPlayers[playerNum]) {
    return false;
  }
  for (int i = 0; i < TOHOST_LENGTH; i++) {
    target[i] = this->playerStates[playerNum][i];
  }
  return true;
}

void GameNet::updateColor(int playerNum, byte r, byte g, byte b) {
  if (!this->isHost || !this->connectedPlayers[playerNum]) {
    return;
  }
  Wire.beginTransmission(I2C_PLAYER_START + playerNum);
  byte msg[4] = {FROMHOST_ASSIGN, r, g, b};
  Wire.write(msg, 4);
  Wire.endTransmission();
}

void GameNet::resetController(int playerNum) {
  if (!this->isHost || !this->connectedPlayers[playerNum]) {
    return;
  }
  Wire.beginTransmission(I2C_PLAYER_START + playerNum);
  byte msg[1] = {FROMHOST_RESET};
  Wire.write(msg, 1);
  Wire.endTransmission();
}

void GameNet::resetAll() {
  for (int i = 0; i < I2C_PLAYERS; i++) {
    this->resetController(i);
  }
}

bool GameNet::checkResetFlag() {
  bool isReset = (this->flags & FLAG_RESET_STATE) > 0;
  this->flags = this->flags & ~FLAG_RESET_STATE;
  return isReset;
}

int GameNet::getNumPlayers() {
  int c = 0;
  for (int i = 0; i < I2C_PLAYERS; i++) {
    if (this->connectedPlayers[i]) {
      c++;
    }
  }
  return c;
}

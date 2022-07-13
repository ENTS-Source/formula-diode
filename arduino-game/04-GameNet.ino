#include <Wire.h>

#define SDA_PIN D2
#define SCL_PIN D1
#define WIRE_SPEED 100000

#define I2C_PLAYERS 4
#define I2C_PLAYER_START 0 // address

#define TOHOST_LENGTH 2  // btn 1 presses & btn 2 presses, 2 bytes
#define FROMHOST_ASSIGN 0x10
#define FROMHOST_RESET 0x11

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

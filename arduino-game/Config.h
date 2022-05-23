#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <EEPROM.h>

#define CONF_EEPROM_SIZE 512

#define CONF_SB_IP_ADDR 0
#define CONF_SB_IP_LEN 16 // string, fixed length

#define MIN_PLAYERS 1
#define MAX_PLAYERS 4 // Must match GameNet I2C_PLAYERS

class Config {
  private:
    String readString(int addr, int len);
    int readInt(int addr);
    byte readByte(int addr);

    void writeString(int addr, String val);
    void writeInt(int addr, int val);
    void writeByte(int addr, byte val);

  public:
    String scoreboardIP;
    int numLeds;

    Config();
    void read();
    void write();
    void clear();
};

#endif

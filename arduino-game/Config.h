#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <EEPROM.h>
#include <Regexp.h>

#define CONF_EEPROM_SIZE 512

#define CONF_SB_IP_ADDR 128 // stay out of the way of wifimanager
#define CONF_SB_IP_LEN 16 // string, fixed length
#define NO_SB_IP "none"

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

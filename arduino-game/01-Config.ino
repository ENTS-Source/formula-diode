#include <EEPROM.h>
#include <Regexp.h>

// === VARIABLES ===
#define CONF_SB_IP_ADDR 128 // stay out of the way of wifimanager
#define CONF_SB_IP_LEN 16 // string, fixed length
#define NO_SB_IP "none"

// === STATE MACHINE ===
#define CONF_EEPROM_SIZE 512

String scoreboardIp;

void confSetup() {
  EEPROM.begin(CONF_EEPROM_SIZE);
}

void confRead() {
  scoreboardIp = confReadString(CONF_SB_IP_ADDR, CONF_SB_IP_LEN);

  // Validation
  MatchState ms;
  char sbipArr[CONF_SB_IP_LEN];
  scoreboardIp.toCharArray(sbipArr, CONF_SB_IP_LEN);
  ms.Target(sbipArr, CONF_SB_IP_LEN);
  if (ms.Match("[0-9]+.[0-9]+.[0-9]+.[0-9]+") != REGEXP_MATCHED) {
    Serial.println("Invalid scoreboard IP: " + scoreboardIp);
    scoreboardIp = NO_SB_IP;
  }
  scoreboardIp.trim();
  if (scoreboardIp.length() <= 0) {
    Serial.println("Scoreboard IP was empty - resetting");
    scoreboardIp = NO_SB_IP;
  }

  // Debug
  Serial.println("Scoreboard IP: " + scoreboardIp);
}

void confWrite() {
  String sbip = scoreboardIp;

  // Truncate if needed
  if (sbip.length() > CONF_SB_IP_LEN) {
    sbip = "";
    for (int i = 0; i < CONF_SB_IP_LEN; i++) {
      sbip += scoreboardIp[i];
    }
  }

  // Pad if needed
  while(sbip.length() < CONF_SB_IP_LEN) {
    sbip += " "; // add empty space
  }

  confWriteString(CONF_SB_IP_ADDR, sbip);
  EEPROM.commit();
  confRead(); // read back for validation
}

void confClear() {
  scoreboardIp = "";
  confWrite();
}

String confReadString(int addr, int len) {
  String val = "";
  for (int i = 0; i < len; i++) {
    val += char(confReadByte(addr + i));
  }
  return val;
}

void confWriteString(int addr, String val) {
  for (int i = 0; i < val.length(); i++) {
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
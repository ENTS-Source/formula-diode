#ifndef NETWORKING_H
#define NETWORKING_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FastLED.h>
#include <WiFiManager.h>
#include "Track.h"
#include "Config.h"
#include "GameNet.h"

#define NW_STATUS_LED 4
#define FIELD_SCOREBOARD_NAME "scoreboard"
#define FIELD_NUMLEDS_NAME "numLeds"

#define WS_PLAYER_UPDATE 0x50

class Networking {
  private:
    Config* config;
    WiFiClient tcpClient;

    // WifiManager stuff
    WiFiManager wm; // internal constructor
    WiFiManagerParameter* scoreboardIPField;
    WiFiManagerParameter* numPlayersField;
    void saveWmParams();

  public:
    Networking(Track* track, Config* config);
    void update();
//    void markLapCompleted(int playerNum, int lap, long ms);
//    void markAllLapsCompleted(int playerNum, long ms);
//    void markDNF(int playerNum);
//    void markGameEnd();
//    void markGameStart(int numPlayers);
};

#endif

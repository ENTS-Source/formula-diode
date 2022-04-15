#ifndef NETWORKING_H
#define NETWORKING_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FastLED.h>
#include <WiFiManager.h>
#include "Track.h"
#include "Config.h"

#define NW_STATUS_LED 4
#define FIELD_SCOREBOARD_NAME "scoreboard"
#define FIELD_NUMLEDS_NAME "numLeds"
#define FIELD_NUMPLAYERS_NAME "numPlayers"

class Networking {
  private:
    Config* config;

    // WifiManager stuff
    WiFiManager wm; // internal constructor
    WiFiManagerParameter* scoreboardIPField;
    WiFiManagerParameter* numPlayersField;
    void saveWmParams();

  public:
    Networking(Track* track, Config* config);
    void update();
};

#endif
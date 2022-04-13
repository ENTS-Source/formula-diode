#ifndef NETWORKING_H
#define NETWORKING_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FastLED.h>
#include <WiFiManager.h>
#include "Track.h"

#define NW_STATUS_LED 4

class Networking {
  private:
  public:
    Networking(Track* track);
    void update();
};

#endif
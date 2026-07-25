#ifndef TRAFFIC_LIGHT_H
#define TRAFFIC_LIGHT_H

#include <Arduino.h>
#include <FastLED.h>

#define TRAFFIC_LIGHT_FEATURE_LENGTH 6
#define TRAFFIC_LIGHT_TIME_MS 3000
#define TRAFFIC_HOLD_GREEN_MS 400

class TrafficLight {
  private:
    long startMs;
    CRGB leds[TRAFFIC_LIGHT_FEATURE_LENGTH];

    void update();

  public:
    TrafficLight();
    void resetAndStart();
    void render(CRGB leds[]); // expects TRAFFIC_LIGHT_FEATURE_LENGTH
    long getMsRemaining();
    bool isFinished();
    bool isRunning();
};

#endif
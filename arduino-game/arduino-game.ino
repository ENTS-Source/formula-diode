#include <FastLED.h>

// NodeMCU / ES8266

#define CAR_LENGTH 3

void setup() {
  prepareStrip();
}

void loop() {
  runTraffic();
}

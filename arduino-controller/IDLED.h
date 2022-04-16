#ifndef ID_LED_H
#define ID_LED_H

#include <Arduino.h>
#include <FastLED.h>

#define LED_PIN 4

class IDLED {
  private:
    CRGB leds[1];

  public:
    IDLED();
    void off();
    void setColor(int red, int green, int blue);
};

#endif

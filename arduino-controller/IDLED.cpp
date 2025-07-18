#include "IDLED.h"

IDLED::IDLED() {
  FastLED.addLeds<WS2812B, LED_PIN, RGB>(this->leds, 1);
}

void IDLED::off() {
  this->setColor(0, 0, 0);
}

void IDLED::setColor(int r, int g, int b) {
  this->leds[0] = CRGB(r, g, b);
  FastLED.show();
}

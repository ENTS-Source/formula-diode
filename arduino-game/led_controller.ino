#define LED_PIN D4
#define NUM_LEDS 50

CRGB leds[NUM_LEDS];

void prepareStrip() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  clearStrip();
}

void clearStrip() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
}

void drawEntity(int topPos, CRGB colors[], int length) {
  int startPos = topPos - length;
  for (int i = 0; i < length; i++) {
    leds[i + startPos] = colors[length - 1 - i];
  }
  FastLED.show();
}

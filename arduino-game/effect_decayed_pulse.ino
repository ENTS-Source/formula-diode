#define PULSE_DECAY 8
#define PULSE_LENGTH 100

void runDecayedPulse(int length, CRGB color) {
  CRGB colors[length];
  for (int p = 0; p < length; p++) {
    int factor = random(255);
    colors[p] = CRGB(
      (color.r * factor) / 256,
      (color.g * factor) / 256,
      (color.b * factor) / 256
    );
  }
  drawEntity(length, colors, length);
  for (int i = 0; i < PULSE_LENGTH; i++) {
    for (int p = 0; p < length; p++) {
      colors[p].fadeToBlackBy(PULSE_DECAY);
    }
    drawEntity(length, colors, length);
    delay(25);
  }
}
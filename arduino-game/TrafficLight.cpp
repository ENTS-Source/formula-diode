#include "TrafficLight.h"

CRGB TRAFFIC_NOT_SET = CRGB::Black;
CRGB TRAFFIC_RED = CRGB(255, 0, 0);
CRGB TRAFFIC_YELLOW = CRGB(239, 83, 0);
CRGB TRAFFIC_GREEN = CRGB(0, 132, 5);

TrafficLight::TrafficLight() {
  this->startMs = 0;
}

void TrafficLight::update() {
  long progress = millis() - this->startMs;
  if (this->startMs == 0 || progress > (TRAFFIC_LIGHT_TIME_MS + TRAFFIC_HOLD_GREEN_MS)) {
    for (byte i = 0; i < TRAFFIC_LIGHT_FEATURE_LENGTH; i++) {
      this->leds[i] = TRAFFIC_NOT_SET;
    }
    return;
  }

  long msPerSegment = TRAFFIC_LIGHT_TIME_MS / 3;

  int sizeOfSegment = TRAFFIC_LIGHT_FEATURE_LENGTH / (TRAFFIC_LIGHT_TIME_MS / 1000);
  for (byte i = 0; i < sizeOfSegment; i++) {
    this->leds[i] = TRAFFIC_RED;
  }

  if (progress >= msPerSegment) {
    for (byte i = 0; i < sizeOfSegment; i++) {
      this->leds[i + sizeOfSegment] = TRAFFIC_YELLOW;
    }
  }
  if (progress >= (msPerSegment * 2)) {
    for (byte i = 0; i < TRAFFIC_LIGHT_FEATURE_LENGTH; i++) {
      this->leds[i] = TRAFFIC_GREEN;
    }
  }
}

void TrafficLight::resetAndStart() {
  this->startMs = millis();
  this->update();
}

long TrafficLight::getMsRemaining() {
  this->update();
  if (this->leds[0] != TRAFFIC_NOT_SET) {
    return TRAFFIC_LIGHT_TIME_MS - (millis() - this->startMs);
  }
  return 0;
}

bool TrafficLight::isFinished() {
  this->update();
  return this->leds[0] == TRAFFIC_GREEN;
}

bool TrafficLight::isRunning() {
  this->update();
  return this->leds[0] != TRAFFIC_NOT_SET;
}

void TrafficLight::render(CRGB leds[]) {
  this->update();
  for (byte i = 0; i < TRAFFIC_LIGHT_FEATURE_LENGTH; i++) {
    leds[i] = this->leds[i];
  }
}

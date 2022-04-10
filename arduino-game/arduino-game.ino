#include <FastLED.h>
#include "PlayerController.h"

// NodeMCU / ESP8266

PlayerController player1(D0);
PlayerController player2(D1);
PlayerController player3(D2);
PlayerController player4(D3);

void setup() {
  randomSeed(analogRead(D6)); // unconnected
  prepareStrip();
  Serial.begin(9600);
}

void loop() {
  // runTraffic();
  // runDecayedPulse(50, CRGB(random(255), random(255), random(255)));
  
  player1.runPhysics();
  // player2.runPhysics();
  // player3.runPhysics();
  // player4.runPhysics();

  clearStrip(false);
  drawPlayer(player1);
  render();
}

#include "Networking.h"

Networking::Networking(Track* track) {
  WiFi.mode(WIFI_STA);

  track->setLed(NW_STATUS_LED, CRGB(50, 5, 5));
  track->render();

  WiFiManager wm; // internal constructor

  track->startBtn->updateBlocking();
  Serial.println(track->startBtn->isPressed);
  if (track->startBtn->isPressed) {
    Serial.println("Wifi reset button pressed - clearing wifi manager");
    wm.resetSettings();
  }

  bool worked = wm.autoConnect("LEDRacerGameBoard");
  if (!worked) {
    Serial.println("Failed to configure wifi");

    // fail loop
    while (true) {
      track->setLed(NW_STATUS_LED, CRGB::Red);
      track->render();
      delay(500);
      track->setLed(NW_STATUS_LED, CRGB::Black);
      track->render();
      delay(250);
    }
  }

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  track->setLed(NW_STATUS_LED, CRGB::Black);
  track->render();
}

void Networking::update() {
  // TODO: Things
}

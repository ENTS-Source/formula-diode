#include "Networking.h"

Networking::Networking(Track* track) {
  WiFi.begin(SSID, PSK);
  
  while (WiFi.status() != WL_CONNECTED) {
    track->setLed(NW_STATUS_LED, CRGB(10, 10, 30));
    track->render();
    delay(100);
    track->setLed(NW_STATUS_LED, CRGB::Black);
    track->render();
    delay(100);
  }  
  
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
}

void Networking::update() {
  // TODO: Things
}
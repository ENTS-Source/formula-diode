#include "Networking.h"

Networking::Networking(Track* track, Config* config) {
  this->config = config;

  WiFi.mode(WIFI_STA);

  track->setLed(NW_STATUS_LED, CRGB(50, 5, 5));
  track->render();

  track->startBtn->updateBlocking();
  if (track->startBtn->isPressed) {
    Serial.println("Wifi reset button pressed - clearing wifi manager");
    this->wm.resetSettings();
    this->config->clear();
  }

  this->scoreboardIPField = new WiFiManagerParameter(FIELD_SCOREBOARD_NAME, "Scoreboard IP", "none", CONF_SB_IP_LEN, "placeholder=\"none\"");

  this->wm.addParameter(this->scoreboardIPField);
  this->wm.setSaveParamsCallback([this]() {
    this->saveWmParams();
  });

  std::vector<const char *> menu = {"wifi", "param", "sep", "restart", "exit"};
  this->wm.setMenu(menu);

  bool worked = this->wm.autoConnect("LEDRacerGameBoard");
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

  this->saveWmParams(); // in case the user didn't set them up

  track->setLed(NW_STATUS_LED, CRGB::Black);
  track->render();
}

void Networking::update() {
  // TODO: Things
}

void Networking::saveWmParams() {
  this->config->scoreboardIP = String(this->scoreboardIPField->getValue());
  this->config->write();
}

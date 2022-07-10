#include "Networking.h"

void voidMqttCallback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
}

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

  // Use the saved value, if any
  this->config->read();
  String sbIp = this->config->scoreboardIP;
  if (sbIp == "") {
    sbIp = "none";
  }
  this->scoreboardIPField = new WiFiManagerParameter(FIELD_SCOREBOARD_NAME, "Scoreboard IP", sbIp.c_str(), CONF_SB_IP_LEN, "placeholder=\"none\"");

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

  if (this->config->scoreboardIP != NO_SB_IP) {
    Serial.println("Validating scoreboard server details");
    IPAddress ip;
    if (!ip.fromString(this->config->scoreboardIP)) {
      Serial.println("Invalid IP");
      while (true) {
        // Enter error state
        track->setLed(NW_STATUS_LED, CRGB::Red);
        track->render();
        delay(1000);
      }
    }
    Serial.println("Ready to connect to scoreboard server");
  }

  track->setLed(NW_STATUS_LED, CRGB::Black);
  track->render();
}

void Networking::update() {
  // TODO: Things?
}

void Networking::saveWmParams() {
  this->config->scoreboardIP = String(this->scoreboardIPField->getValue());
  this->config->write();
}

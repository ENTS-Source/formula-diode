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
    Serial.println("Connecting to TCP server");
    if (!this->tcpClient.connect(this->config->scoreboardIP, 9000)) {
      Serial.println("Error in connection");
      while (true) {
        // Enter error state
        track->setLed(NW_STATUS_LED, CRGB::Red);
        track->render();
        delay(1000);
      }
    }
    Serial.println("Connected to TCP");
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

void Networking::sendPlayerState(int playerNum, byte state[TOHOST_LENGTH]) {
  // TODO: Make this work
  this->tcpClient.print(WS_PLAYER_UPDATE);
  this->tcpClient.print(playerNum);
  for (int i = 0; i < TOHOST_LENGTH; i++) {
    this->tcpClient.print(state[i]);
  }
  this->tcpClient.println();
  this->tcpClient.flush();
}

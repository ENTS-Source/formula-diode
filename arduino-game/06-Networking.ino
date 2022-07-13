#include <ESP8266WiFi.h>
#include <FastLED.h>
#include <WiFiManager.h>

#define NW_STATUS_LED 4
#define FIELD_SCOREBOARD_NAME "scoreboard"
#define FIELD_NUMLEDS_NAME "numLeds"

WiFiManager wm;
WiFiManagerParameter* scoreboardIPField;

void netSetup() {
  WiFi.mode(WIFI_STA);
  leds[NW_STATUS_LED] = CRGB(50, 5, 5);
  trakRender();

  btnUpdateBlocking();
  if (btnPressed) {
    Serial.println("WiFi reset button pressed - clearing settings");
    wm.resetSettings();
    confClear();
  }

  confRead();
  String sbip = scoreboardIp;
  if (sbip == "") {
    sbip = NO_SB_IP;
  }
  scoreboardIPField = new WiFiManagerParameter(FIELD_SCOREBOARD_NAME, "Scoreboard IP", sbip.c_str(), CONF_SB_IP_LEN, "placeholder=\"none\"");

  wm.addParameter(scoreboardIPField);
  wm.setSaveParamsCallback(netSaveWmParams);

  std::vector<const char *> menu = {"wifi", "param", "sep", "restart", "exit"};
  wm.setMenu(menu);

  bool worked = wm.autoConnect("LEDRacerGameBoard");
  if (!worked) {
    Serial.println("Failed to configure wifi");

    // fail loop
    while(true) {
      leds[NW_STATUS_LED] = CRGB::Red;
      trakRender();
      delay(500);
      leds[NW_STATUS_LED] = CRGB::Black;
      trakRender();
      delay(250);
    }
  }

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  netSaveWmParams(); // just in case

  if (scoreboardIp != NO_SB_IP) {
    Serial.println("Validating scoreboard details");
    IPAddress ip;
    if (!ip.fromString(scoreboardIp)) {
      Serial.println("Invalid IP");

      // fail loop
      while(true) {
        leds[NW_STATUS_LED] = CRGB::Red;
        trakRender();
        delay(500);
        leds[NW_STATUS_LED] = CRGB::Black;
        trakRender();
        delay(250);
      }
    }

    Serial.println("Ready to connect to scoreboard");
  }

  leds[NW_STATUS_LED] = CRGB::Black;
  trakRender();
}

void netSaveWmParams() {
  scoreboardIp = String(scoreboardIPField->getValue());
  confWrite();
}
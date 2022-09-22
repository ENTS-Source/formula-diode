#include <FastLED.h>
#include "Button.h"
#include "VibrationMotor.h"
#include "IDLED.h"
#include "GameNet.h"

// Metro Mini / Arduino Uno

#define NUM_PIN_LOW 2
#define NUM_PIN_HIGH 3

Button* accl;
VibrationMotor* motor;
IDLED* led;
GameNet* gamenet;

byte lastR = 0;
byte lastG = 0;
byte lastB = 0;

void setup() {
  pinMode(NUM_PIN_LOW, INPUT);
  pinMode(NUM_PIN_HIGH, INPUT);
  
  Serial.begin(115200);

  int playerNum = 0;
  if (digitalRead(NUM_PIN_LOW) == HIGH) {
    playerNum = playerNum | B00000001;
  }
  if (digitalRead(NUM_PIN_HIGH) == HIGH) {
    playerNum = playerNum | B00000010;
  }
  Serial.print("Player num: ");
  Serial.println(playerNum);
  
  accl = new Button(6);
  motor = new VibrationMotor(5);
  led = new IDLED();
  gamenet = new GameNet(playerNum);
  led->setColor(0, 0, 0);
}

void loop() {
  motor->update();
  accl->update();
  if (accl->isDownTrigger) {
//    motor->onForMs(1000);
    gamenet->acclBtnPresses++;
    Serial.println("Press");
  }
  checkLed();
  if (gamenet->checkResetFlag()) {
    Serial.println("Handling reset");
    gamenet->acclBtnPresses = 0;
    gamenet->secondBtnPresses = 0;
//    motor->off();
  }
}

void checkLed() {
  if (lastR == gamenet->idR && lastG == gamenet->idG && lastB == gamenet->idB) {
    return;
  }

  lastR = gamenet->idR;
  lastG = gamenet->idG;
  lastB = gamenet->idB;
  led->setColor(lastR, lastG, lastB);
}

#include <FastLED.h>
#include "Button.h"
#include "VibrationMotor.h"
#include "IDLED.h"

// Metro Mini / Arduino Uno

Button* accl;
VibrationMotor* motor;
IDLED* led;

void setup() {
  Serial.begin(9600);
  accl = new Button(6);
  motor = new VibrationMotor(5);
  led = new IDLED();
}

void loop() {
  motor->update();
  accl->update();
  if (accl->isDownTrigger) {
    motor->onForMs(1000);
    Serial.println("Press");
  }
  if (accl->isPressed) {
    led->setColor(0, 0, 255);
  } else {
    led->setColor(255, 0, 0);
  }
}

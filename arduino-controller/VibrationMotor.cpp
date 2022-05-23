#include "VibrationMotor.h"

VibrationMotor::VibrationMotor(byte pin) {
  pinMode(pin, OUTPUT);
  this->pin = pin;
  this->untilMs = 0;
}

void VibrationMotor::on() {
  digitalWrite(this->pin, HIGH);
}

void VibrationMotor::off() {
  digitalWrite(this->pin, LOW);
  this->untilMs = 0;
}

void VibrationMotor::onForMs(unsigned long ms) {
  this->on();
  this->untilMs = (millis() + ms);
}

void VibrationMotor::update() {
  if (millis() >= this->untilMs && this->untilMs > 0) {
    this->off();
    this->untilMs = 0;
  }
}

#ifndef VIBRATION_MOTOR_H
#define VIBRATION_MOTOR_H

#include <Arduino.h>

class VibrationMotor {
  private:
    byte pin;
    unsigned long untilMs;

  public:
    VibrationMotor(byte pin);
    void on();
    void off();
    void onForMs(unsigned long ms);
    void update();
};

#endif

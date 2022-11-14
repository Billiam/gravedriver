#ifndef Solenoid_h_
#define Solenoid_h_

#include <RBD_Timer.h>
#include <cstdint>

class Solenoid
{
public:
  Solenoid(uint8_t pin);
  void update(int frequency, int power, unsigned int duration, int curve = 0);
  int spm;
  int pow;
  int freq;
  double spmPercent();

private:
  uint8_t _pin;
  bool _on;
  unsigned long _startMicros;
  RBD::Timer _solenoidTimer;
  void setSolenoid(uint8_t val, uint8_t power);
  void hfWrite(pin_size_t pin, int val, int us = 200);
};

#endif
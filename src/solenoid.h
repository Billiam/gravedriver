#ifndef Solenoid_h_
#define Solenoid_h_

#include <cstdint>
#include <RBD_Timer.h>

class Solenoid
{
  public:
    Solenoid(uint8_t pin);
    void update(int frequency, int power, unsigned int duration);
    int spm;
    int pow;
    int freq;
  private:
    uint8_t _pin;
    bool _on;
    unsigned long _startMicros;
    RBD::Timer _solenoidTimer;
    void setSolenoid(uint8_t val, uint8_t power);
    void hfWrite(pin_size_t pin, int val);
};

#endif
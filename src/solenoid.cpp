#include "solenoid.h"
#include <RBD_Timer.h>
#include <cstdint>

#include "pinDefinitions.h"

#define MIN_FREQUENCY 20
#define MAX_FREQUENCY 2400
#define MIN_POWER 60

Solenoid::Solenoid(uint8_t pin) : _pin(pin) {}

void Solenoid::update(int frequency, int power, unsigned int duration)
{
  pow = power;
  int mappedPower = power == 0 ? 0 : map(power, 1, 1023, MIN_POWER, 128);
  int mappedFrequency = map(frequency, 0, 1023, MIN_FREQUENCY, MAX_FREQUENCY);
  _solenoidTimer.setTimeout(60000/mappedFrequency);

  if (frequency == 0 || power == 0) {
    setSolenoid(LOW, power);
    _solenoidTimer.stop();
  } else if (_solenoidTimer.onRestart()) {
    setSolenoid(HIGH, power);
  } else if (_solenoidTimer.isStopped()) {
    _solenoidTimer.restart();
  } else if (_on && _solenoidTimer.getValue() > duration) {
    // off cycle
    // Serial.println(micros() - _startMicros);
    setSolenoid(LOW, power);
  }
  if (frequency == 0) {
    spm = 0;
  } else {
    spm = mappedFrequency;
  }
}

static int write_resolution = 8;

void Solenoid::setSolenoid(uint8_t val, uint8_t power) {
  _on = val == HIGH;
  // analogWrite(_pin, _on ? power : 0);
  hfWrite(_pin, _on ? power : 0);
}

void Solenoid::hfWrite(pin_size_t pin, int val)
{
  if (pin >= PINS_COUNT) {
    return;
  }

  float percent = (float)val/(float)((1 << write_resolution)-1);
  mbed::PwmOut* pwm = digitalPinToPwm(pin);
  if (pwm == NULL) {
    pwm = new mbed::PwmOut(digitalPinToPinName(pin));
    digitalPinToPwm(pin) = pwm;
    // pwm->period_ms(2); //500Hz
    pwm->period_us(200); //5000Hz
  }
  if (percent < 0) {
    delete pwm;
    digitalPinToPwm(pin) = NULL;
  } else {
    pwm->write(percent);
  }
}

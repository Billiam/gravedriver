#include "solenoid.h"
#include "fast_math.h"
#include <RBD_Timer.h>
#include <cstdint>

#include "pinDefinitions.h"

#define MIN_FREQUENCY 20
#define MAX_FREQUENCY 2600
#define MIN_POWER 60

Solenoid::Solenoid(uint8_t pin) : _pin(pin), _enabled(true) {}

double Solenoid::spmPercent() { return (1.0 * spm) / MAX_FREQUENCY; }

void Solenoid::update(int frequency, int power, unsigned int duration)
{
  int mappedPower = power == 0 ? 0 : map(power, 1, 1023, MIN_POWER, 128);
  // need to handle curve input for power
  // int curveFreq = curveInput(frequency, frequencyCurve);
  int mappedFrequency = map(frequency, 0, 1023, MIN_FREQUENCY, MAX_FREQUENCY);

  // Serial.print(mappedPower);
  // Serial.print("\t");
  // Serial.print(mappedFrequency);
  // Serial.print("\t");
  // Serial.println(curveFreq);

  _solenoidTimer.setTimeout(60000 / mappedFrequency);

  if (frequency == 0 || mappedPower == 0) {
    setSolenoid(LOW, mappedPower);
    _solenoidTimer.stop();
  } else if (_solenoidTimer.onRestart()) {
    setSolenoid(HIGH, mappedPower);
  } else if (_solenoidTimer.isStopped()) {
    _solenoidTimer.restart();
  } else if (_on && _solenoidTimer.getValue() > duration) {
    // off cycle
    // Serial.println(micros() - _startMicros);
    setSolenoid(LOW, mappedPower);
  }

  if (frequency == 0) {
    spm = 0;
  } else {
    spm = mappedFrequency;
  }
  
  if (power == 0) {
    pow = 0;
  } else {
    pow = mappedPower;
  }
}

static int write_resolution = 8;

void Solenoid::setSolenoid(uint8_t val, uint8_t power)
{
  _on = val == HIGH && _enabled;
  hfWrite(_pin, _on ? power : 0, 200);
}

void Solenoid::hfWrite(pin_size_t pin, int val, int us)
{
  if (pin >= PINS_COUNT) {
    return;
  }

  float percent = (float)val / (float)((1 << write_resolution) - 1);
  mbed::PwmOut *pwm = digitalPinToPwm(pin);
  if (pwm == NULL) {
    pwm = new mbed::PwmOut(digitalPinToPinName(pin));
    digitalPinToPwm(pin) = pwm;
    // pwm->period_ms(2); // 500Hz
    pwm->period_us(us);
  }
  if (percent < 0) {
    delete pwm;
    digitalPinToPwm(pin) = NULL;
  } else {
    pwm->write(percent);
  }
}

void Solenoid::disable() { _enabled = false; }
void Solenoid::enable() { _enabled = true; }
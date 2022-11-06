#include "solenoid.h"
#include <RBD_Timer.h>
#include <cstdint>

#include "pinDefinitions.h"

// #define MIN_DELAY 85
// #define MAX_DELAY 1500
#define MIN_FREQUENCY 40
#define MAX_FREQUENCY 3000


Solenoid::Solenoid(uint8_t pin) : _pin(pin) {}
//temp

void Solenoid::update(int frequency, int power, unsigned int duration)
{ 
  // _cycleTime = 
  spm = map(frequency, 0, 1024, MIN_FREQUENCY, MAX_FREQUENCY);
  // _cycleTime = map(frequency, 0, 1024, MIN_DELAY, MAX_DELAY);
  _solenoidTimer.setTimeout(60000/spm);

  if (frequency == 0 || power == 0) {
    if (frequency == 0) {
      spm = 0;
    }
    setSolenoid(LOW, power);
    _solenoidTimer.stop();
  } else if (_solenoidTimer.onRestart()) {
    _startMicros = micros();
    Serial.println("on");
    setSolenoid(HIGH, power);
  } else if (_solenoidTimer.isStopped()) {
    _solenoidTimer.restart();
  } else if (_on && micros() - _startMicros > (duration * 30)) {
    // off cycle
    Serial.println(micros() - _startMicros);
    setSolenoid(LOW, power);
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

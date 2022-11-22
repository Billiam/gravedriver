/*
 * Rotary encoder library for Arduino.
 */

#ifndef STEP_ROTARY_H
#define STEP_ROTARY_H

#include "Arduino.h"

// Values returned by 'process'
// No complete step yet.
#define DIR_NONE 0x0
// Clockwise step.
#define DIR_CW 0x10
// Counter-clockwise step.
#define DIR_CCW 0x20

#define ONE_STEP 1
#define HALF_STEP 2
#define FULL_STEP 3

class StepRotary
{
public:
  StepRotary(char, char, unsigned char = FULL_STEP);
  unsigned char process();
  void begin(bool internalPullup = true, bool flipLogicForPulldown = false);

  inline unsigned char pin_1() const { return pin1; }
  inline unsigned char pin_2() const { return pin2; }
  void setStep(unsigned char);

private:
  unsigned char state;
  unsigned char pin1;
  unsigned char pin2;
  unsigned char inverter;
  const unsigned char (*_table)[4];
};

#endif

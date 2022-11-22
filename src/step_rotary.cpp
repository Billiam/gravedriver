/* StepRotary encoder handler for arduino.
 *
 * Copyright 2011 Ben Buxton. Licenced under the GNU GPL Version 3.
 * Contact: bb@cactii.net
 *
 */

#include "Arduino.h"

#include "step_rotary.h"

/*
 * The below state table has, for each state (row), the new state
 * to set based on the next encoder output. From left to right in,
 * the table, the encoder outputs are 00, 01, 10, 11, and the value
 * in that position is the new state to set.
 */

#define R_START 0x0

const unsigned char one_step[4][4] = {
    {0x00, 0x01 | DIR_CCW, 0x02 | DIR_CW, 0x00},
    {0x00 | DIR_CW, 0x01, 0x01, 0x03 | DIR_CCW},
    {0x00 | DIR_CCW, 0x02, 0x02, 0x03 | DIR_CW},
    {0x03, 0x01 | DIR_CW, 0x02 | DIR_CCW, 0x03},
};
const unsigned char half_step[6][4] = {
    {0x3, 0x2, 0x1, R_START},
    {0x3 | DIR_CCW, R_START, 0x1, R_START},
    {0x3 | DIR_CW, 0x2, R_START, R_START},
    {0x3, 0x5, 0x4, R_START},
    {0x3, 0x3, 0x4, R_START | DIR_CW},
    {0x3, 0x5, 0x3, R_START | DIR_CCW},
};
const unsigned char full_step[7][4] = {
    {R_START, 0x2, 0x4, R_START}, {0x3, R_START, 0x1, R_START | DIR_CW},
    {0x3, 0x2, R_START, R_START}, {0x3, 0x2, 0x1, R_START},
    {0x6, R_START, 0x4, R_START}, {0x6, 0x5, R_START, R_START | DIR_CCW},
    {0x6, 0x5, 0x4, R_START},
};

/*
 * Constructor. Each arg is the pin number for each encoder contact.
 */
StepRotary::StepRotary(char _pin1, char _pin2, unsigned char step_type)
{
  // Assign variables.
  pin1 = _pin1;
  pin2 = _pin2;
  // Initialise state.
  state = R_START;

  setStep(step_type);
  // Don't invert read pin state by default
  inverter = 0;
}

void StepRotary::begin(bool internalPullup, bool flipLogicForPulldown)
{

  if (internalPullup) {
    // Enable weak pullups
    pinMode(pin1, INPUT_PULLUP);
    pinMode(pin2, INPUT_PULLUP);
  } else {
    // Set pins to input.
    pinMode(pin1, INPUT);
    pinMode(pin2, INPUT);
  }
  inverter = flipLogicForPulldown ? 1 : 0;
}

unsigned char StepRotary::process()
{
  // Grab state of input pins.
  unsigned char pinstate =
      ((inverter ^ digitalRead(pin2)) << 1) | (inverter ^ digitalRead(pin1));
  // Determine new state from the pins and state table.
  state = _table[state & 0xf][pinstate];
  // Return emit bits, ie the generated event.
  return state & 0x30;
}

void StepRotary::setStep(unsigned char step_type)
{
  switch (step_type) {
    case ONE_STEP:
      _table = one_step;
      break;
    case HALF_STEP:
      _table = half_step;
      break;
    case FULL_STEP:
    default:
      _table = full_step;
  }
}
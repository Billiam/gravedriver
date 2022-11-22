#include "hold_button.h"
#include <Bounce2.h>

#define BUTTON_READY 0x00
#define BUTTON_HOLD 0x01
#define BUTTON_IGNORE 0x02

HoldButton::HoldButton() : _held(BUTTON_READY) {}

bool HoldButton::wasReleased()
{
  if (_held != BUTTON_READY) {
    return false;
  }

  if (getPressedState() == HIGH) {
    return fell();
  } else {
    return rose();
  }
}
bool HoldButton::wasPressed()
{
  if (_held != BUTTON_READY) {
    return false;
  }
  if (getPressedState() == HIGH) {
    return rose();
  } else {
    return fell();
  }
}

bool HoldButton::update()
{
  bool changed = Bounce2::Button::update();
  if (_held == BUTTON_IGNORE) {
    _held = BUTTON_READY;
  } else if (_held == BUTTON_HOLD && changed && !isPressed()) {
    _held = BUTTON_IGNORE;
  }

  return changed;
}

bool HoldButton::wasHeld(uint16_t time)
{
  if (_held == BUTTON_READY && isPressed() && currentDuration() > time) {
    _held = BUTTON_HOLD;
    return true;
  }
  return false;
}
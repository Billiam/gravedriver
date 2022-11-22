#ifndef hold_button_h_
#define hold_button_h_

#include <Bounce2.h>

class HoldButton : public Bounce2::Button
{
public:
  HoldButton();
  bool wasPressed();
  bool wasReleased();
  bool wasHeld(uint16_t time);
  bool update();

protected:
  unsigned char _held;
};

#endif
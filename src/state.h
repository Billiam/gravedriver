#ifndef statetype_h_
#define statetype_h_

#include "pedal_mode.h"
#include "power_mode.h"
#include "scene.h"

struct stateType {
  Scene scene;
  PowerMode powerMode;
  PedalMode pedalMode;

  unsigned int duration;
  int pedalMin;
  int pedalMax;
  unsigned int frequency;
  bool pedalRead;
  unsigned int power;
  int curve;
};
#endif
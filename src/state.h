#ifndef statetype_h_
#define statetype_h_

#include "power_mode.h"
#include "scene.h"

struct stateType {
  Scene scene;
  PowerMode powerMode;

  unsigned int duration;
  int pedalMin;
  int pedalMax;
  bool pedalRead;
  int power;
  int curve;
};
#endif
#ifndef statetype_h_
#define statetype_h_

#include "pedal_mode.h"
#include "power_mode.h"
#include "scene.h"

struct stateType {
  Scene scene;
  PowerMode powerMode;
  PedalMode pedalMode;
  uint8_t graver;
  uint8_t graverCount;

  unsigned int duration;
  int pedalMin;
  int pedalMax;
  unsigned int frequency;
  unsigned int power;
  unsigned int powerMin;

  unsigned int spmMin;
  unsigned int spmMax;

  int curve;

  char graverName[8];

  float confirmPct;
  bool confirmSelected;
  absolute_time_t confirmTime;
};
#endif
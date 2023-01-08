#ifndef statetype_h_
#define statetype_h_

#include "pedal_mode.h"
#include "power_mode.h"
#include "scene.h"

struct stateType {
  Scene scene;
  Scene lastScene;

  PowerMode powerMode;
  PedalMode pedalMode;

  uint8_t graver;
  bool graverChanged;

  char graverNames[12][8];

  unsigned int duration;
  int pedalMin;
  int pedalMax;
  unsigned int frequency;
  unsigned int power;
  uint8_t powerMin;

  unsigned int spmMin;
  unsigned int spmMax;

  int curve;

  char graverName[8];

  float confirmPct;
  bool confirmSelected;
  absolute_time_t confirmTime;
};
#endif
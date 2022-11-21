#ifndef statetype_h_
#define statetype_h_

struct stateType {
  int scene;

  unsigned int duration;
  int pedalMin;
  int pedalMax;
  bool pedalRead;

  int curve;
};
#endif
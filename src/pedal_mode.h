#ifndef pedal_mode_h_
#define pedal_mode_h_

#include <map>

enum class PedalMode : uint8_t { FREQUENCY = 0,
                                 POWER = 1 };

const std::map<PedalMode, const char *> PedalModeLabel = {
    {PedalMode::FREQUENCY, "frequency"}, {PedalMode::POWER, "power"}};

#endif
#include "graver_menu.h"
#include "live_numeric_menu_item.h"
#include "menu_renderer.h"
#include "scaling_numeric_menu_item.h"
#include "scene.h"
#include "state.h"
#include "store.h"
#include "string"
#include "vendor/MenuSystem.h"

#define DISPLAY_LINES 5

extern stateType state;
extern Store store;

MenuRenderer renderer;
MenuSystem menu = MenuSystem(renderer, DISPLAY_LINES);

void onCurveSelected(MenuComponent *p_menu_component);
void onExitSelected(MenuComponent *p_menu_component);
void onCalibrateSelected(MenuComponent *p_menu_component);
void onModeSelected(MenuComponent *p_menu_component);
void onMinPowerSelected(MenuComponent *p_menu_component);
void onMinPowerChanged(const float value);
void onGraverSelected(MenuComponent *p_menu_component);
void onMinSpmSelected(MenuComponent *p_menu_component);
void onMaxSpmSelected(MenuComponent *p_menu_component);
float spmScaling(const float value);

BackMenuItem mi_exit("back", &onExitSelected, &menu);
TextMenuItem mi_mode("pedal", &onModeSelected, "");
MenuItem mi_curve("input curve", &onCurveSelected);
MenuItem mi_calibrate("calibrate", &onCalibrateSelected);
LiveNumericMenuItem mi_power_min("min power", &onMinPowerSelected, 0, 0, 128, 1.0F, nullptr, &onMinPowerChanged);
TextMenuItem mi_graver("graver", &onGraverSelected, "");
ScalingNumericMenuItem mi_freq_min("min spm", &onMinSpmSelected, 5.0, 5.0, 4000, 5.0, &spmScaling);
ScalingNumericMenuItem mi_freq_max("max spm", &onMaxSpmSelected, 3000, 5.0, 4000, 5.0, &spmScaling);

void buildMenu()
{
  menu.get_root_menu().set_name("OPTIONS");

  mi_mode.set_value(PedalModeLabel.at(PedalMode::FREQUENCY));
  mi_power_min.set_value(state.powerMin);
  mi_graver.set_value(state.graverName);
  mi_freq_min.set_value(state.spmMin);
  mi_freq_max.set_value(state.spmMax);

  Menu *ms = &menu.get_root_menu();

  ms->add_item(&mi_exit);
  ms->add_item(&mi_graver);
  ms->add_item(&mi_mode);
  ms->add_item(&mi_curve);
  ms->add_item(&mi_calibrate);
  ms->add_item(&mi_power_min);
  ms->add_item(&mi_freq_min);
  ms->add_item(&mi_freq_max);
}

void onCurveSelected(MenuComponent *component) { state.scene = Scene::CURVE; }
void onExitSelected(MenuComponent *component) { state.scene = Scene::STATUS; }
void onCalibrateSelected(MenuComponent *component)
{
  state.scene = Scene::CALIBRATE;
}

void onModeSelected(MenuComponent *component)
{
  if (state.pedalMode == PedalMode::FREQUENCY) {
    state.pedalMode = PedalMode::POWER;
  } else {
    state.pedalMode = PedalMode::FREQUENCY;
  }
  store.writeUint(state.graver, FramKey::PEDAL_MODE, static_cast<uint8_t>(state.pedalMode));
  mi_mode.set_value(PedalModeLabel.at(state.pedalMode));
}

void onMinPowerSelected(MenuComponent *component)
{
}

void onMinPowerChanged(const float value)
{
  state.powerMin = value;
  store.writeUint(state.graver, FramKey::POWER_MIN, state.powerMin);
}

void onGraverSelected(MenuComponent *component)
{
  if (state.graverCount == 0) {
    return;
  }

  uint8_t nextGraver = state.graver + 1;
  if (nextGraver > state.graverCount - 1) {
    nextGraver = 0;
  }
  state.graver = nextGraver;
}

void onMinSpmSelected(MenuComponent *component)
{
  float spmMin = static_cast<ScalingNumericMenuItem *>(component)->get_value();
  state.spmMin = min(spmMin, state.spmMax);
  Serial.println(state.spmMin);
  store.writeUint16(state.graver, FramKey::FREQUENCY_MIN, (uint16_t)state.spmMin);
}

void onMaxSpmSelected(MenuComponent *component)
{
  float spmMax = static_cast<ScalingNumericMenuItem *>(component)->get_value();
  state.spmMax = max(spmMax, state.spmMin);
  Serial.println(state.spmMax);
  store.writeUint16(state.graver, FramKey::FREQUENCY_MAX, state.spmMax);
}

float spmScaling(const float val)
{
  if (val < 50) {
    return 1;
  } else if (val < 100) {
    return 5;
  } else if (val < 500) {
    return 10;
  } else if (val < 1000) {
    return 50;
  } else {
    return 100;
  }
}
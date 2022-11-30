#include "graver_menu.h"
#include "MenuSystem.h"
#include "live_numeric_menu_item.h"
#include "menu_renderer.h"
#include "scene.h"
#include "state.h"
#include "store.h"
#include "string"

extern stateType state;
extern Store store;

MenuRenderer renderer;
MenuSystem menu = MenuSystem(renderer);

void onCurveSelected(MenuComponent *p_menu_component);
void onExitSelected(MenuComponent *p_menu_component);
void onCalibrateSelected(MenuComponent *p_menu_component);
void onModeSelected(MenuComponent *p_menu_component);

BackMenuItem mi_exit("back", &onExitSelected, &menu);
MenuItem mi_curve("input curve", &onCurveSelected);
MenuItem mi_calibrate("calibrate", &onCalibrateSelected);
TextMenuItem mi_mode("pedal", &onModeSelected, "");

void buildMenu()
{
  menu.get_root_menu().set_name("options");

  mi_mode.set_value(PedalModeLabel.at(PedalMode::FREQUENCY));

  menu.get_root_menu().add_item(&mi_exit);
  menu.get_root_menu().add_item(&mi_mode);
  menu.get_root_menu().add_item(&mi_curve);
  menu.get_root_menu().add_item(&mi_calibrate);
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
  store.writeUint(FramKey::PEDAL_MODE, static_cast<uint8_t>(state.pedalMode));
  mi_mode.set_value(PedalModeLabel.at(state.pedalMode));
}
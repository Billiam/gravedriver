#include "graver_menu.h"
#include "MenuSystem.h"
#include "live_numeric_menu_item.h"
#include "menu_renderer.h"
#include "scene.h"
#include "state.h"
#include "string"

extern stateType state;

MenuRenderer renderer;
MenuSystem menu = MenuSystem(renderer);

void onCurveSelected(MenuComponent *p_menu_component);
void onExitSelected(MenuComponent *p_menu_component);
void onDurationSelected(MenuComponent *p_menu_component);
void onDurationChanged(float val);

BackMenuItem mi_exit("back", &onExitSelected, &menu);
MenuItem mi_curve("Input curve", &onCurveSelected);
LiveNumericMenuItem mi_duration("on time", nullptr, 5.0, 5.0, 40.0, -1.0,
                                nullptr, &onDurationChanged);

void buildMenu()
{
  menu.get_root_menu().set_name("options");

  menu.get_root_menu().add_item(&mi_exit);
  menu.get_root_menu().add_item(&mi_curve);

  menu.get_root_menu().add_item(&mi_duration);

  //   // initialize values
  mi_duration.set_value(state.duration);
  // TODO: values will come from fram, previous settings
#ifdef MIN_DURATION
  mi_duration.set_min_value(MIN_DURATION);
#endif
#ifdef MAX_DURATION
  mi_duration.set_max_value(MAX_DURATION);
#endif
}

void onCurveSelected(MenuComponent *component)
{
  Serial.println("CURVE SCENE");
  state.scene = SCENE_CURVE;
}
void onExitSelected(MenuComponent *component)
{
  Serial.println("STATUS SCENE");
  state.scene = SCENE_STATUS;
}

void onDurationChanged(float value) { state.duration = value; }
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
void onCalibrateSelected(MenuComponent *p_menu_component);

BackMenuItem mi_exit("back", &onExitSelected, &menu);
MenuItem mi_curve("input curve", &onCurveSelected);
MenuItem mi_calibrate("calibrate", &onCalibrateSelected);

void buildMenu()
{
  menu.get_root_menu().set_name("options");

  menu.get_root_menu().add_item(&mi_exit);
  menu.get_root_menu().add_item(&mi_curve);
  menu.get_root_menu().add_item(&mi_calibrate);
}

void onCurveSelected(MenuComponent *component) { state.scene = SCENE_CURVE; }
void onExitSelected(MenuComponent *component) { state.scene = SCENE_STATUS; }
void onCalibrateSelected(MenuComponent *component)
{
  state.scene = SCENE_CALIBRATE;
}
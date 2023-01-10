#include "graver_menu.h"
#include "constant.h"
#include "live_numeric_menu_item.h"
#include "menu_renderer.h"
#include "rename_menu_item.h"
#include "scaling_numeric_menu_item.h"
#include "scene.h"
#include "state.h"
#include "store.h"
#include "string"
#include "vendor/MenuSystem.h"

#define DISPLAY_LINES 5

extern stateType state;
extern Store store;
extern const int MaxGravers;

MenuRenderer renderer;

MenuSystem menu = MenuSystem(renderer, DISPLAY_LINES);
MenuSystem graverMenu = MenuSystem(renderer, DISPLAY_LINES);

void nullSelection(MenuComponent *component) {}

void onExitSelected(MenuComponent *p_menu_component);
void onCalibrateSelected(MenuComponent *p_menu_component);
void onCurveSelected(MenuComponent *p_menu_component);
void onGraverMenuSelected(MenuComponent *p_menu_component);
void onGraverNameSelected(MenuComponent *p_menu_component);
void onGraverSelected(MenuComponent *p_menu_component);
void onModeSelected(MenuComponent *p_menu_component);

void onMinSpmSelected(MenuComponent *p_menu_component);
void onMaxSpmSelected(MenuComponent *p_menu_component);

void onClearMemory(MenuComponent *p_menu_component);
void onMinPowerChanged(const float value);

float spmScaling(const float value);

BackMenuItem mi_exit("back", &onExitSelected, &menu);
TextMenuItem mi_mode("pedal", &onModeSelected, "");
MenuItem mi_curve("input curve", &onCurveSelected);
MenuItem mi_calibrate("calibrate", &onCalibrateSelected);
LiveNumericMenuItem mi_power_min("min power", &nullSelection, 0, 0, 255, 1.0F, nullptr, &onMinPowerChanged);

ScalingNumericMenuItem mi_freq_min("min spm", &onMinSpmSelected, 5.0, 5.0, 4000, 5.0, &spmScaling);
ScalingNumericMenuItem mi_freq_max("max spm", &onMaxSpmSelected, 3000, 5.0, 4000, 5.0, &spmScaling);
MenuItem mi_clear("clear memory", &onClearMemory);

RenameMenuItem mi_graver_name("name", &onGraverNameSelected, "        ", 8);
Menu m_graver("preset", DISPLAY_LINES, &onGraverMenuSelected);

MenuItem mi_graver1("1", &onGraverSelected);
MenuItem mi_graver2("2", &onGraverSelected);
MenuItem mi_graver3("3", &onGraverSelected);
MenuItem mi_graver4("4", &onGraverSelected);
MenuItem mi_graver5("5", &onGraverSelected);
MenuItem mi_graver6("6", &onGraverSelected);
MenuItem mi_graver7("7", &onGraverSelected);
MenuItem mi_graver8("8", &onGraverSelected);
MenuItem mi_graver9("9", &onGraverSelected);
MenuItem mi_graver10("10", &onGraverSelected);
MenuItem mi_graver11("11", &onGraverSelected);
MenuItem mi_graver12("12", &onGraverSelected);

MenuItem *gravers[MaxGravers] = {
    &mi_graver1, &mi_graver2, &mi_graver3, &mi_graver4, &mi_graver5, &mi_graver6, &mi_graver7, &mi_graver8, &mi_graver9, &mi_graver10, &mi_graver11, &mi_graver12};

void buildMenu()
{
  Menu *ms = &menu.get_root_menu();
  Menu *gms = &graverMenu.get_root_menu();

  ms->set_name("OPTIONS");
  gms->set_name("PRESETS");

  mi_mode.set_value(PedalModeLabel.at(PedalMode::FREQUENCY));

  updateMenuItems();

  ms->add_item(&mi_exit);

  ms->add_menu(&m_graver);

  ms->add_item(&mi_graver_name);
  ms->add_item(&mi_mode);
  ms->add_item(&mi_curve);
  ms->add_item(&mi_calibrate);
  ms->add_item(&mi_power_min);
  ms->add_item(&mi_freq_min);
  ms->add_item(&mi_freq_max);
  ms->add_item(&mi_clear);

  for (int i = 0; i < MaxGravers; i++) {
    m_graver.add_item(gravers[i]);
    gms->add_item(gravers[i]);
  }
}

void updateMenuItems()
{
  mi_power_min.set_value(state.powerMin);
  mi_freq_min.set_value(state.spmMin);
  mi_freq_max.set_value(state.spmMax);
  mi_graver_name.set_value(state.graverNames[state.graver]);
}

void onCurveSelected(MenuComponent *component)
{
  state.scene = Scene::CURVE;
}
void onExitSelected(MenuComponent *component)
{
  state.scene = Scene::STATUS;
}
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

void onMinPowerChanged(const float value)
{
  state.powerMin = value;
  store.writeUint(state.graver, FramKey::POWER_MIN, state.powerMin);
}

void onMinSpmSelected(MenuComponent *component)
{
  float spmMin = static_cast<ScalingNumericMenuItem *>(component)->get_value();
  state.spmMin = min(spmMin, state.spmMax);
  store.writeUint16(state.graver, FramKey::FREQUENCY_MIN, (uint16_t)state.spmMin);
}

void onMaxSpmSelected(MenuComponent *component)
{
  float spmMax = static_cast<ScalingNumericMenuItem *>(component)->get_value();
  state.spmMax = max(spmMax, state.spmMin);
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

void onClearMemory(MenuComponent *component)
{
  state.scene = Scene::RESET;
}

void onGraverSelected(MenuComponent *component)
{
  MenuItem *item = static_cast<MenuItem *>(component);

  for (int i = 0; i < MaxGravers; i++) {
    if (item == gravers[i]) {
      if (state.graver != i) {
        state.graver = i;
        store.writeUint(FramKey::GRAVER, state.graver);
        state.graverChanged = true;
      }
      if (state.scene == Scene::MENU) {
        menu.back();
      } else {
        state.scene = state.lastScene;
      }
      return;
    }
  }
}

void updateGraverLabels()
{
  for (int i = 0; i < MaxGravers; i++) {
    // TODO: show previous selection
    gravers[i]->set_name(state.graverNames[i]);
  }
}

void onGraverMenuSelected(MenuComponent *component)
{
  updateGraverLabels();
  m_graver.set_current_selection(state.graver);
}

void onGraverNameSelected(MenuComponent *component)
{
  RenameMenuItem *item = static_cast<RenameMenuItem *>(component);
  snprintf(state.graverNames[state.graver], 9, "%s", item->get_current_value());
  store.writeChars(state.graver, FramKey::NAME, item->get_current_value(), 8);
}
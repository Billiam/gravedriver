#ifndef graver_menu_h_
#define graver_menu_h_

#include "vendor/MenuSystem.h"

extern MenuSystem menu;
extern MenuSystem graverMenu;

void buildMenu();
void updateMenuItems();
void updateGraverLabels();

#endif
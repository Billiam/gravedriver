#ifndef graver_menu_h_
#define graver_menu_h_

#include "MenuSystem.h"

extern MenuSystem menu;

void buildMenu();

/*

Default: Status
Menu:
    Power Duration, numeric (temporary, will be moved to power dial)

    Input mode -> name (but could be submenu)
    Change Input Curve -> scene
    Calibrate -> scene
        Record min max
    Output limits
        Frequency range
        Power range
    Device: <name or number>
*/
#endif
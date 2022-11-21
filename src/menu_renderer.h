#ifndef menu_renderer_h_
#define menu_renderer_h_

#include <MenuSystem.h>

class MenuRenderer : public MenuComponentRenderer
{
public:
  void render(Menu const &menu) const;
  void render_menu_item(MenuItem const &menu_item) const;
  void render_back_menu_item(BackMenuItem const &menu_item) const;
  void render_numeric_menu_item(NumericMenuItem const &menu_item) const;
  void render_menu(Menu const &menu) const;
};
#endif
#include "text_menu_item.h"
#include "menu_renderer.h"

TextMenuItem::TextMenuItem(const char *name, SelectFnPtr select_fn,
                           const char *value)
    : MenuItem(name, select_fn), _value(value)
{
}

const char *TextMenuItem::get_value() const { return _value; }

void TextMenuItem::set_value(const char *value) { _value = value; }

void TextMenuItem::render(MenuComponentRenderer const &renderer) const
{
  MenuRenderer const &custom_renderer =
      static_cast<MenuRenderer const &>(renderer);
  custom_renderer.render_text_menu_item(*this);
}
#ifndef TEXT_MENU_ITEM_H
#define TEXT_MENU_ITEM_H

#include "vendor/MenuSystem.h"

class TextMenuItem : public MenuItem
{
public:
  TextMenuItem(const char *name, SelectFnPtr select_fn, const char *value);
  void set_value(const char *value);
  const char *get_value() const;

  virtual void render(MenuComponentRenderer const &renderer) const;

protected:
  const char *_value;
};

#endif

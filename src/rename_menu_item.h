#ifndef RENAME_MENU_ITEM_H
#define RENAME_MENU_ITEM_H

#include "vendor/MenuSystem.h"

class RenameMenuItem : public MenuItem
{
public:
  RenameMenuItem(const char *name, SelectFnPtr select_fn, const char *value, uint8_t size);

  void set_value(const char *value);
  const char *get_value() const;
  const char *get_current_value() const;

  uint8_t get_size() const;
  bool char_selected() const;
  uint8_t current_char() const;

  virtual void render(MenuComponentRenderer const &renderer) const;

protected:
  const char *_value;
  char *_current_value;
  uint8_t _size;

  uint8_t _char_index;
  bool _char_selected;

protected:
  virtual bool next(bool loop = false);
  virtual bool prev(bool loop = false);
  virtual void reset();
  virtual Menu *select();
};

#endif

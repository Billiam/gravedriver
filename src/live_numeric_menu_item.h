#ifndef LIVE_NUMERIC_MENU_ITEM_H
#define LIVE_NUMERIC_MENU_ITEM_H

#include "vendor/MenuSystem.h"

class LiveNumericMenuItem : public NumericMenuItem
{
public:
  using ChangeValueFnPtr = void (*)(const float value);

public:
  LiveNumericMenuItem(const char *name, SelectFnPtr select_fn, float value,
                      float minValue, float maxValue, float increment = 1.0,
                      FormatValueFnPtr on_format_value = nullptr,
                      ChangeValueFnPtr on_change_value = nullptr);

  using FormatValueFnPtr = const String (*)(const float value);
  void set_value(float value);

protected:
  ChangeValueFnPtr _change_value_fn;

  virtual bool next(bool loop = false);
  virtual bool prev(bool loop = false);
};

#endif

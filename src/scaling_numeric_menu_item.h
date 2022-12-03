#ifndef SCALING_NUMERIC_MENU_ITEM_H
#define SCALING_NUMERIC_MENU_ITEM_H

#include "vendor/MenuSystem.h"

class ScalingNumericMenuItem : public NumericMenuItem
{
public:
  using ScaleValueFnPtr = float (*)(const float value);
  using FormatValueFnPtr = const String (*)(const float value);

public:
  ScalingNumericMenuItem(const char *name, SelectFnPtr select_fn, float value,
                         float minValue, float maxValue,
                         float increment = 1.0F,
                         ScaleValueFnPtr scale_value = nullptr,
                         FormatValueFnPtr on_format_value = nullptr);

protected:
  ScaleValueFnPtr _scale_value_fn;

  virtual bool next(bool loop = false);
  virtual bool prev(bool loop = false);
};

#endif

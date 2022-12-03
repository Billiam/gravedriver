#include "scaling_numeric_menu_item.h"

ScalingNumericMenuItem::ScalingNumericMenuItem(const char *name,
                                               SelectFnPtr select_fn, float value,
                                               float minValue, float maxValue,
                                               float increment,
                                               ScaleValueFnPtr scale_value,
                                               FormatValueFnPtr on_format_value)
    : NumericMenuItem(name, select_fn, value, minValue, maxValue, increment, on_format_value),
      _scale_value_fn(scale_value)
{
}

bool ScalingNumericMenuItem::next(bool loop)
{
  if (_scale_value_fn != nullptr) {
    _increment = _scale_value_fn(_value);
  }

  return NumericMenuItem::next(loop);
}

bool ScalingNumericMenuItem::prev(bool loop)
{
  if (_scale_value_fn != nullptr) {
    _increment = _scale_value_fn(_value);
  }

  return NumericMenuItem::prev(loop);
}

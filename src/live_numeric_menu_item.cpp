#include "live_numeric_menu_item.h"

LiveNumericMenuItem::LiveNumericMenuItem(const char *name,
                                         SelectFnPtr select_fn, float value,
                                         float minValue, float maxValue,
                                         float increment,
                                         FormatValueFnPtr on_format_value,
                                         ChangeValueFnPtr on_change_value)
    : NumericMenuItem(name, select_fn, value, minValue, maxValue, increment,
                      on_format_value),
      _change_value_fn(on_change_value)
{
}

bool LiveNumericMenuItem::next(bool loop)
{
  bool result = NumericMenuItem::next(loop);
  if (result && _change_value_fn != nullptr) {
    _change_value_fn(_value);
  }
  return result;
}

bool LiveNumericMenuItem::prev(bool loop)
{
  bool result = NumericMenuItem::prev(loop);
  if (result && _change_value_fn != nullptr) {
    _change_value_fn(_value);
  }
  return result;
}

void LiveNumericMenuItem::set_value(float value)
{
  float _previous = _value;
  NumericMenuItem::set_value(value);
  if (_previous != _value && _change_value_fn != nullptr) {
    _change_value_fn(_value);
  }
}
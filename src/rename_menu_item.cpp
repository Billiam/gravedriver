#include "rename_menu_item.h"
#include <algorithm>
#include <iterator>
#include <vector>

std::vector<char> digits{' ', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '!', '#', '-', '.'};

RenameMenuItem::RenameMenuItem(const char *name,
                               SelectFnPtr select_fn,
                               const char *value,
                               uint8_t size)
    : MenuItem(name, select_fn), _value(value), _size(size)
{
  _current_value = new char[_size + 1]();
  snprintf(_current_value, _size + 1, "%s", _value);
}

const char *RenameMenuItem::get_value() const { return _value; }
const char *RenameMenuItem::get_current_value() const { return _current_value; }

uint8_t RenameMenuItem::get_size() const
{
  return _size;
}

bool RenameMenuItem::char_selected() const
{
  return _char_selected;
}

uint8_t RenameMenuItem::current_char() const
{
  return _char_index;
}

void RenameMenuItem::reset()
{
  snprintf(_current_value, _size + 1, "%s", _value);

  _char_selected = false;
  _char_index = 0;
  _has_focus = false;
}

Menu *RenameMenuItem::select()
{
  // focus field if unfocused
  if (!_has_focus) {
    _has_focus = true;
    return nullptr;
  }

  // toggle character selection off, advance to next character
  if (_char_selected) {
    _char_selected = false;
    _char_index++;
    return nullptr;
  }

  if (_char_index < _size) {
    // enable character selection (in character range)
    _char_selected = true;
  } else if (_char_index == _size) {
    // save button
    if (_select_fn != nullptr) {
      _select_fn(this);
    }
    _char_selected = false;
    _char_index = 0;
    _has_focus = false;
  } else if (_char_index == _size + 1) {
    // cancel button
    reset();
  }

  return nullptr;
}

bool RenameMenuItem::next(bool loop)
{
  if (!_has_focus) {
    return true;
  }
  if (!_char_selected) {
    _char_index = (_char_index + 1 + _size + 2) % (_size + 2);
    return true;
  }

  if (_char_index < _size) {
    size_t digitIndex = std::distance(begin(digits), std::find(begin(digits), end(digits), _current_value[_char_index]));

    if (digitIndex == digits.size()) {
      digitIndex = 0;
    }

    digitIndex = (digitIndex + 1 + digits.size()) % digits.size();

    for (uint8_t k = 0; k < _char_index; k++) {
      if (_current_value[k] == '\0') {
        _current_value[k] = ' ';
      }
    }

    _current_value[_char_index] = digits[digitIndex];
  }

  return true;
}

bool RenameMenuItem::prev(bool loop)
{
  if (!_has_focus) {
    return true;
  }
  if (!_char_selected) {
    _char_index = (_char_index - 1 + _size + 2) % (_size + 2);
    return true;
  }

  if (_char_index < _size) {
    size_t digitIndex = std::distance(begin(digits), std::find(begin(digits), end(digits), _current_value[_char_index]));

    if (digitIndex == digits.size()) {
      digitIndex = 0;
    }

    digitIndex = (digitIndex - 1 + digits.size()) % digits.size();

    for (uint8_t k = 0; k < _char_index; k++) {
      if (_current_value[k] == '\0') {
        _current_value[k] = ' ';
      }
    }

    _current_value[_char_index] = digits[digitIndex];
  }
  return true;
}

void RenameMenuItem::set_value(const char *value)
{
  _value = value;
  snprintf(_current_value, _size + 1, "%s", _value);
}

void RenameMenuItem::render(MenuComponentRenderer const &renderer) const
{
  renderer.render_rename_menu_item(*this);
}
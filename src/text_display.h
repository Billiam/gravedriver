#ifndef text_display_h_
#define text_display_h_

#include <ssd1306.h>
#include <utility>

class TextDisplay
{
public:
  TextDisplay(pico_ssd1306::SSD1306 *ssd1306);
  void text(const char *message);
  void text(int fontSize, const char *message);
  void textln(const char *message);
  void textln(int fontSize, const char *message);
  void textf(int fontSize, const char *format, ...);
  void textfln(int fontSize, const char *format, ...);
  int nextline(int fontSize = 1) const;
  uint8_t fontWidth(int fontSize = 1) const;
  void setCursor(int x, int y);
  void moveCursor(int x, int y);
  int getCursorX() const;
  int getCursorY() const;

private:
  int cursorX;
  int cursorY;
  pico_ssd1306::SSD1306 *_display;
};
#endif
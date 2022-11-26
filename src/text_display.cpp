#include "text_display.h"
#include "font_8x6.h"
#include "textRenderer/TextRenderer.h"
#include <cstdarg>

const unsigned char *fonts[1] = {font_8x6};

// TODO: Set font size separately
TextDisplay::TextDisplay(pico_ssd1306::SSD1306 *display) : _display(display) {}

void TextDisplay::text(const char *message) { text(1, message); }
void TextDisplay::text(int fontSize, const char *message)
{
  const unsigned char *font = fonts[fontSize - 1];
  int size = strlen(message);
  uint8_t fontWidth = font[0];
  drawText(_display, font, message, cursorX, cursorY);
  cursorX += size * fontWidth;
}
void TextDisplay::textln(const char *message) { textln(1, message); }
void TextDisplay::textln(int fontSize, const char *message)
{
  const unsigned char *font = fonts[fontSize - 1];

  uint8_t fontHeight = font[1];
  int px = cursorX;

  text(fontSize, message);

  cursorY += fontHeight + 2;
  cursorX = px;
}

void TextDisplay::textf(int fontSize, const char *format, ...)
{
  char buffer[32];
  va_list argptr;
  va_start(argptr, format);
  vsnprintf(buffer, sizeof buffer, format, argptr);
  va_end(argptr);

  text(fontSize, buffer);
}

void TextDisplay::textfln(int fontSize, const char *format, ...)
{
  const unsigned char *font = fonts[fontSize - 1];

  uint8_t fontHeight = font[1];

  int px = cursorX;

  char buffer[32];
  va_list argptr;
  va_start(argptr, format);
  vsnprintf(buffer, sizeof buffer, format, argptr);
  va_end(argptr);

  text(fontSize, buffer);

  cursorY += fontHeight + 2;
  cursorX = px;
}

int TextDisplay::nextline(int fontSize) const
{
  uint8_t fontHeight = fonts[fontSize - 1][1];
  return fontHeight + cursorY + 2;
}

uint8_t TextDisplay::fontWidth(int fontSize) const
{
  return fonts[fontSize - 1][0];
}

void TextDisplay::setCursor(int x, int y)
{
  cursorX = x;
  cursorY = y;
}
void TextDisplay::moveCursor(int x, int y)
{
  cursorX += x;
  cursorY += y;
}

int TextDisplay::getCursorX() const { return cursorX; }
int TextDisplay::getCursorY() const { return cursorY; }

#include "menu_renderer.h"
#include "drawing.h"
#include "font_8x6.h"
#include "rename_menu_item.h"
#include "text_display.h"
#include "text_menu_item.h"
#include <RBD_Timer.h>
#include <shapeRenderer/ShapeRenderer.h>
#include <ssd1306.h>
#include <textRenderer/TextRenderer.h>

extern pico_ssd1306::SSD1306 *display;
extern TextDisplay *textDisplay;

absolute_time_t lastBlink;
bool blink;

#define UI_WIDTH 128
#define UI_HEIGHT 64

void MenuRenderer::render(Menu const &menu) const
{
  absolute_time_t now = get_absolute_time();
  int16_t diff = absolute_time_diff_us(lastBlink, now) / 1000;
  if (diff > 400) {
    lastBlink = now;
    blink = !blink;
  }

  textDisplay->text(menu.get_name());
  drawLine(display, 0, 10, UI_WIDTH, 10);

  uint8_t offset = menu.get_offset();
  uint8_t entries = menu.get_num_components();
  uint8_t lines = menu.get_visible_lines();

  uint8_t lastEntry = min(entries, offset + lines);

  textDisplay->setCursor(8, 13);

  if (offset > 0) {
    drawLine(display, UI_WIDTH / 2 - 4, 10, UI_WIDTH / 2 + 4, 10, pico_ssd1306::WriteMode::SUBTRACT);
    addAdafruitBitmap(display, (UI_WIDTH - 8) / 2 + 1, 8, 8, 8, up_arrow);
  }

  for (int i = offset; i < lastEntry; ++i) {
    MenuComponent const *cp_m_comp = menu.get_menu_component(i);
    int sy = textDisplay->getCursorY();
    if (cp_m_comp->is_current()) {
      addAdafruitBitmap(display, textDisplay->getCursorX() - 8, sy, 8, 8,
                        arrow);
    }
    cp_m_comp->render(*this);
    if (cp_m_comp->is_current() && !cp_m_comp->has_focus()) {
      fillRect(display, 0, sy - 1, 127, textDisplay->getCursorY() - 2,
               pico_ssd1306::WriteMode::INVERT);
    }
  }

  if (offset < entries - lines) {
    addAdafruitBitmap(display, UI_WIDTH / 2 + 1, UI_HEIGHT - 3, 8, 8, down_arrow);
  }
}

void MenuRenderer::render_menu_item(MenuItem const &menu_item) const
{
  textDisplay->textln(1, menu_item.get_name());
}

void MenuRenderer::render_back_menu_item(BackMenuItem const &menu_item) const
{
  textDisplay->textln(1, menu_item.get_name());
}

int digitLength(int value)
{
  if (value == 0) {
    return 1;
  }
  if (value > 0) {
    return int(log10(value) + 1);
  }
  return int(log10(abs(value) + 1)) + 1;
}

void MenuRenderer::render_numeric_menu_item(
    NumericMenuItem const &menu_item) const
{
  int sx = textDisplay->getCursorX();
  int sy = textDisplay->getCursorY();
  int ny = textDisplay->nextline();

  textDisplay->text(menu_item.get_name());

  int val = menu_item.get_value();
  textDisplay->setCursor(UI_WIDTH - digitLength(val) * 6, sy);
  textDisplay->textf(1, "%d", (int)menu_item.get_value());

  if (menu_item.has_focus()) {
    fillRect(display, UI_WIDTH - 30, sy - 1, UI_WIDTH - 1, ny - 2,
             pico_ssd1306::WriteMode::INVERT);
  }
  textDisplay->setCursor(sx, ny);
}

void MenuRenderer::render_rename_menu_item(
    RenameMenuItem const &menu_item) const
{
  int sx = textDisplay->getCursorX();
  int sy = textDisplay->getCursorY();
  int ny = textDisplay->nextline();

  textDisplay->text(menu_item.get_name());

  int chx = UI_WIDTH - 8 * 6 - 16;
  int icon_start = UI_WIDTH - 16;
  textDisplay->setCursor(chx, sy);
  textDisplay->text(menu_item.get_current_value());
  textDisplay->setCursor(sx, ny);

  if (menu_item.has_focus()) {
    addAdafruitBitmap(display, icon_start, sy, 8, 8, check);
    addAdafruitBitmap(display, icon_start + 8, sy, 8, 8, x);

    if (menu_item.char_selected() && blink) {
      return;
    }

    if (menu_item.current_char() < 8) {
      fillRect(
          display,
          chx + menu_item.current_char() * 6 - 1,
          sy - 1,
          chx + (menu_item.current_char() + 1) * 6 - 1,
          ny - 2,
          pico_ssd1306::WriteMode::INVERT);
    } else {
      fillRect(
          display,
          icon_start + 8 * (menu_item.current_char() == 8 ? 0 : 1),
          sy - 1,
          icon_start + 8 * (menu_item.current_char() == 8 ? 1 : 2),
          ny - 2,
          pico_ssd1306::WriteMode::INVERT);
    }
  }
}

void MenuRenderer::render_text_menu_item(TextMenuItem const &menu_item) const
{
  const char *val = menu_item.get_value();
  size_t len = strlen(val);
  int sx = textDisplay->getCursorX();
  int sy = textDisplay->getCursorY();
  int ny = textDisplay->nextline();

  textDisplay->text(menu_item.get_name());
  textDisplay->setCursor(127 - len * textDisplay->fontWidth(), sy);
  textDisplay->text(val);
  textDisplay->setCursor(sx, ny);
}

void MenuRenderer::render_menu(Menu const &menu) const
{
  textDisplay->textln(1, menu.get_name());
}

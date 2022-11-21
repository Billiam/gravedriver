#include "menu_renderer.h"
#include "drawing.h"
#include "font_8x6.h"
#include "text_display.h"
#include <shapeRenderer/ShapeRenderer.h>
#include <ssd1306.h>
#include <textRenderer/TextRenderer.h>

extern pico_ssd1306::SSD1306 *display;
extern TextDisplay *textDisplay;

static const uint8_t arrow[8] = {0x00, 0x10, 0x10, 0x7c,
                                 0x38, 0x10, 0x00, 0x00};

void MenuRenderer::render(Menu const &menu) const
{
  textDisplay->text(menu.get_name());
  drawLine(display, 0, 15, 128, 15);

  textDisplay->setCursor(8, 17);

  for (int i = 0; i < menu.get_num_components(); ++i) {
    MenuComponent const *cp_m_comp = menu.get_menu_component(i);
    int sy = textDisplay->getCursorY();
    if (cp_m_comp->is_current()) {
      addAdafruitBitmap(display, textDisplay->getCursorX() - 8, sy, 8, 8,
                        arrow);
    }
    cp_m_comp->render(*this);
    if (cp_m_comp->is_current() && !cp_m_comp->has_focus()) {
      fillRect(display, 0, sy - 1, 127, textDisplay->getCursorY() - 1,
               pico_ssd1306::WriteMode::INVERT);
    }
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

void MenuRenderer::render_numeric_menu_item(
    NumericMenuItem const &menu_item) const
{
  int sx = textDisplay->getCursorX();
  int sy = textDisplay->getCursorY();
  int ny = textDisplay->nextline();

  textDisplay->text(menu_item.get_name());

  textDisplay->setCursor(110, sy);
  textDisplay->textf(1, "%d", (int)menu_item.get_value());
  if (menu_item.has_focus()) {
    fillRect(display, 109, sy - 1, 127, ny - 1,
             pico_ssd1306::WriteMode::INVERT);
  }
  textDisplay->setCursor(sx, ny);
}

void MenuRenderer::render_menu(Menu const &menu) const
{
  textDisplay->textln(1, menu.get_name());
}
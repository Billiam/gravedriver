#ifndef drawing_h_
#define drawing_h_

#include <ssd1306.h>

void addAdafruitBitmap(
    pico_ssd1306::SSD1306 *display, int16_t anchorX, int16_t anchorY,
    uint8_t image_width, uint8_t image_height, const uint8_t *image,
    pico_ssd1306::WriteMode mode = pico_ssd1306::WriteMode::ADD);

#endif
#ifndef drawing_h_
#define drawing_h_

#include <ssd1306.h>

static const uint8_t arrow[8] = {0x00, 0x10, 0x10, 0x7c,
                                 0x38, 0x10, 0x00, 0x00};

static const uint8_t up_arrow[8] = {0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08, 0x00};
static const uint8_t down_arrow[8] = {0x00, 0x01, 0x02, 0x04, 0x02, 0x01, 0x00, 0x00};
static const uint8_t x[8] = {0x00, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x00};
static const uint8_t check[8] = {0x00, 0x18, 0x30, 0x60, 0x30, 0x18, 0x0c, 0x06};

void addAdafruitBitmap(
    pico_ssd1306::SSD1306 *display,
    int16_t anchorX,
    int16_t anchorY, uint8_t image_width,
    uint8_t image_height,
    const uint8_t *image,
    pico_ssd1306::WriteMode mode = pico_ssd1306::WriteMode::ADD);

#endif
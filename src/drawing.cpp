#include "drawing.h"
#include <ssd1306.h>

void addAdafruitBitmap(pico_ssd1306::SSD1306 *display, int16_t anchorX,
                       int16_t anchorY, uint8_t image_width,
                       uint8_t image_height, unsigned char const *image,
                       pico_ssd1306::WriteMode mode)
{
  int byteHeight = image_height / 8;
  int bytes = (image_width * byteHeight);

  for (uint8_t b = 0; b < bytes; b++) {
    uint8_t byte = image[b];
    if (byte == 0) {
      continue;
    }
    for (uint8_t by = 0; by < 8; by++) {
      if ((byte >> by) & 1) {
        display->setPixel(b % image_width + anchorX,
                          (b / image_width) * 8 + by + anchorY, mode);
      }
    }
  }
}
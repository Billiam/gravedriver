#ifndef rendering_h_
#define rendering_h_

#include <ssd1306.h>

void plotDeadzone(pico_ssd1306::SSD1306 *ssd1306, int frequency, int x, int y,
                  int width, int height);
void plotCurve(pico_ssd1306::SSD1306 *ssd1306, int frequency, int x, int y,
               int width, int height);
void drawWave(pico_ssd1306::SSD1306 *ssd1306, uint8_t y, int frequency, int spm,
              double spmPercent, int duration, int power);
void drawMeter(pico_ssd1306::SSD1306 *ssd1306, int x, int y, int width,
               int height, double percent);
void drawCalibrate(pico_ssd1306::SSD1306 *ssd1306);
void drawMenu(pico_ssd1306::SSD1306 *ssd1306);
void drawCurve(pico_ssd1306::SSD1306 *ssd1306);
void displayLoop();
#endif
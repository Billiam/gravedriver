#include "rendering.h"
#include "drawing.h"
#include "fast_math.h"
#include "solenoid.h"
#include "state.h"
#include "text_display.h"
#include "vendor/MenuSystem.h"
#include <map>
#include <shapeRenderer/ShapeRenderer.h>
#include <ssd1306.h>
#include <textRenderer/TextRenderer.h>

// temp
#define MIN_DURATION 10
#define MAX_DURATION 40

extern stateType state;
extern Solenoid solenoid;
extern MenuSystem menu;

pico_ssd1306::SSD1306 *display;
TextDisplay *textDisplay;

absolute_time_t lastLoopStart;

void plotDeadzone(pico_ssd1306::SSD1306 *ssd1306, int frequency, int x, int y,
                  int width, int height)
{
  int y1 = y + 2;
  int x1 = x + 2;

  int x2 = x + width;
  int y2 = y + height;

  drawLine(ssd1306, x, y2, x + 5, y2);
  drawLine(ssd1306, x, y2, x, y2 - 5);
  drawLine(ssd1306, x2, y, x2 - 5, y);
  drawLine(ssd1306, x2, y, x2, y + 5);

  x2 -= 2;
  y2 -= 2;

  int minX = x1 + map(state.pedalMin, 0, 1023, 2, width - 4);
  int maxX = x1 + map(state.pedalMax, 0, 1023, 2, width - 4);

  // drawLine(ssd1306, x1, y2, minX, y2);
  for (int i = x1; i < minX; i += 2) {
    display->setPixel(i, y2);
  }
  drawLine(ssd1306, minX, y2, maxX, y1);

  // drawLine(ssd1306, maxX, y1, x2, y1);
  for (int i = x2; i >= maxX; i -= 2) {
    display->setPixel(i, y1);
  }
}

// TODO: Configurable screen dimensions
// TODO: Cap duration to frequency percentage or waveTime -
void plotCurve(pico_ssd1306::SSD1306 *ssd1306, int frequency, int x, int y,
               int width, int height)
{
  double pct = frequency / 1024.0;
  int startY = y + height;
  drawRect(ssd1306, x, y, x + width, startY);

  double curve;
  if (state.curve == 0) {
    curve = 1.0;
  } else {
    curve = abs(state.curve);
  }

  if (state.curve == 0) {
    drawLine(ssd1306, x, startY, x + width, y);
  } else if (state.curve > 0) {
    for (int xi = 0; xi < width; xi++) {
      double xPct = (1.0 * xi) / width;
      double yOut = fastPrecisePow(xPct, curve);
      ssd1306->setPixel(xi + x, startY - yOut * height);
    }
  } else {
    // fastPrecicePow has nicer results with positive power, so curve is
    // calculated against Y axis and inverted
    for (int yi = 0; yi < height; yi++) {
      double yPct = (1.0 * yi) / height;
      double xOut = fastPrecisePow(yPct, curve);
      ssd1306->setPixel(x + (xOut * width), startY - yi);
    }
  }
  int indicatorX;
  int indicatorY;

  if (state.curve == 0) {
    indicatorX = x + pct * width;
    indicatorY = startY - pct * width;
  } else {
    indicatorX = x + pct * width;
    curve = state.curve > 0 ? curve : 1.0 / curve;
    indicatorY = startY - fastPrecisePow(pct, curve) * height;
  }
  fillRect(ssd1306, indicatorX - 1, indicatorY - 1, indicatorX + 2,
           indicatorY + 2, pico_ssd1306::WriteMode::INVERT);
}

void drawWave(pico_ssd1306::SSD1306 *ssd1306, uint8_t y, int frequency, int spm,
              double spmPercent, int duration, int power)
{
  const int maxHeight = 14;
  const int minLength = 5;
  const int maxLen = 40;

  int y1 = y + maxHeight;

  if (frequency == 0) {
    drawLine(ssd1306, 5, y1, 123, y1);
    return;
  }

  int height = power == 0 ? 0 : map(power, 1, 1023, 1, maxHeight);
  int wavelength = (1.0 - spmPercent) * (maxLen - minLength) + minLength;
  double waveTime = 60000.0 / spm;
  int pulseWidth = ceil(min(1, duration / waveTime) * wavelength);
  int waves = ceil(118.0 / wavelength);

  int y2 = y1 - height;
  int maxX = 123;

  for (int i = 0; i < waves; i++) {
    int x1 = 5 + i * wavelength;
    int x2 = x1 + wavelength - pulseWidth;
    int x3 = x1 + wavelength;

    drawLine(ssd1306, x1, y1, min(x2, maxX), y1);
    if (x2 < maxX) {
      drawLine(ssd1306, x2, y1, x2, y2);
      drawLine(ssd1306, x2, y2, min(x3, maxX), y2);
      if (x3 < maxX) {
        drawLine(ssd1306, x3, y2, x3, y1);
      }
    }
  }
}

void drawMeter(pico_ssd1306::SSD1306 *ssd1306, int x, int y, int width,
               int height, double percent)
{
  drawRect(ssd1306, x, y, x + width, y + height);
  if (percent > 0) {
    int barWidth = (int)(width - 4) * percent;
    fillRect(ssd1306, x + 2, y + 2, x + 2 + barWidth, y + height - 2);
  }
}

void drawCalibrate(pico_ssd1306::SSD1306 *ssd1306)
{
  textDisplay->text("calibration");
  textDisplay->setCursor(60, 18);

  int pedalState =
      state.pedalMode == PedalMode::FREQUENCY ? state.frequency : state.power;
  plotDeadzone(ssd1306, pedalState, 5, 18, 45, 45);
  textDisplay->textln("move pedal");
  textDisplay->textln("to min/max");

  textDisplay->moveCursor(0, 8);
  textDisplay->text("done");

  fillRect(ssd1306, 59, textDisplay->getCursorY() - 1,
           textDisplay->getCursorX() + 1, textDisplay->getCursorY() + 8,
           pico_ssd1306::WriteMode::INVERT);
}

void drawReset(pico_ssd1306::SSD1306 *ssd1306)
{
  static bool inverted = false;
  if (state.clearConfirmed) {

    uint16_t diff = absolute_time_diff_us(state.confirmTime, lastLoopStart) / 1000;
    if (diff < 100) {
      if (!inverted) {
        inverted = true;
        ssd1306->invertDisplay();
      }
    } else if (diff < 500) {
      if (inverted) {
        inverted = false;
        ssd1306->invertDisplay();
      }

      uint8_t h = 32 * (diff - 100) / 400;
      fillRect(ssd1306, 0, h, 127, 64 - h);
    } else {
      uint8_t x = 64 * min(1, (diff - 500) / 500.0);
      drawLine(ssd1306, x, 32, 128 - x, 32);
    }
    return;
  }

  uint8_t magnitude = state.confirmPct < 0.1 ? 0 : 1 + 4 * state.confirmPct;
  int ssy = random(-magnitude / 2, magnitude * 1.5);
  int ssx = random(-magnitude, magnitude);

  textDisplay->moveCursor(ssx + 1, ssy);
  textDisplay->textln("clear all data?");
  textDisplay->moveCursor(0, 3);

  textDisplay->textln("hold to confirm");
  if (state.confirmSelected) {
    fillRect(ssd1306, max(0, textDisplay->getCursorX() - 1), textDisplay->getCursorY() - 11, 91 + ssx, textDisplay->getCursorY() - 2, pico_ssd1306::WriteMode::INVERT);
  }

  textDisplay->moveCursor(0, 3);

  drawMeter(ssd1306, 6 + ssx, textDisplay->getCursorY(), 120, 20, state.confirmPct);

  textDisplay->setCursor(52 + ssx, 56 + ssy);

  textDisplay->text("back");
  if (!state.confirmSelected) {
    fillRect(ssd1306, 51, textDisplay->getCursorY() - 1, 75, textDisplay->getCursorY() + 8, pico_ssd1306::WriteMode::INVERT);
  }
}

void drawStatus(pico_ssd1306::SSD1306 *ssd1306)
{
  int spm = solenoid.spm;
  int freq = state.frequency;

  int solenoidPower = state.power;

  drawWave(ssd1306, 0, freq, spm, solenoid.spmPercent(), state.duration,
           solenoidPower);

  textDisplay->setCursor(8, 18);
  drawMeter(ssd1306, 80, textDisplay->getCursorY(), 46, 8, freq / 1024.0);
  if (state.pedalMode == PedalMode::POWER &&
      state.powerMode == PowerMode::POWER) {
    addAdafruitBitmap(ssd1306, textDisplay->getCursorX() - 8,
                      textDisplay->getCursorY(), 8, 8, arrow);
  }
  textDisplay->textfln(1, "spm: %d", spm);
  // drawText(ssd1306, font_8x6, buff, 0, 18);

  drawMeter(ssd1306, 80, textDisplay->getCursorY(), 46, 8,
            solenoidPower / 1024.0);
  if (state.pedalMode == PedalMode::FREQUENCY &&
      state.powerMode == PowerMode::POWER) {
    addAdafruitBitmap(ssd1306, textDisplay->getCursorX() - 8,
                      textDisplay->getCursorY(), 8, 8, arrow);
  }
  textDisplay->textfln(1, "pow: %d", solenoidPower);

  drawMeter(ssd1306, 80, textDisplay->getCursorY(), 46, 8,
            (1.0 * state.duration - MIN_DURATION) /
                (MAX_DURATION - MIN_DURATION));
  if (state.powerMode == PowerMode::DURATION) {
    addAdafruitBitmap(ssd1306, textDisplay->getCursorX() - 8,
                      textDisplay->getCursorY(), 8, 8, arrow);
  }
  textDisplay->textfln(1, "dur: %d", state.duration);

  textDisplay->textfln(1, "off: %d",
                       spm > 0 ? 60000 / spm - state.duration : 0);
};

void drawMenu(pico_ssd1306::SSD1306 *ssd1306) { menu.display(); }
void drawCurve(pico_ssd1306::SSD1306 *ssd1306)
{
  textDisplay->text("input curve");

  // TODO: display options or current selection
  textDisplay->setCursor(60, 18);

  int currentPoint;
  if (state.pedalMode == PedalMode::FREQUENCY) {
    textDisplay->textfln(1, "spm: %d", solenoid.spm);
    currentPoint = state.frequency;
  } else {
    textDisplay->textfln(1, "power: %d", solenoid.pow);
    currentPoint = state.power;
  }

  textDisplay->moveCursor(0, 8);

  textDisplay->text("done");
  fillRect(ssd1306, 59, textDisplay->getCursorY() - 1,
           textDisplay->getCursorX() + 1, textDisplay->getCursorY() + 8,
           pico_ssd1306::WriteMode::INVERT);

  plotCurve(ssd1306, currentPoint, 5, 18, 45, 45);
}

void displayLoop()
{
  pico_ssd1306::SSD1306 displayInst =
      pico_ssd1306::SSD1306(i2c0, 0x3C, pico_ssd1306::Size::W128xH64);
  display = &displayInst;
  TextDisplay textDisplayInst = TextDisplay(display);
  textDisplay = &textDisplayInst;

  displayInst.setOrientation(0);
  displayInst.sendBuffer(); // Send buffer to device and show on screen

  while (1) {
    absolute_time_t now = get_absolute_time();
    lastLoopStart = now;

    textDisplay->setCursor(0, 0);
    displayInst.clear();

    switch (state.scene) {
      case Scene::STATUS:
        drawStatus(display);
        break;
      case Scene::MENU:
        drawMenu(display);
        break;
      case Scene::CURVE:
        drawCurve(display);
        break;
      case Scene::CALIBRATE:
        drawCalibrate(display);
        break;
      case Scene::RESET:
        drawReset(display);
        break;
    }

    // display FPS
    textDisplay->setCursor(105, 56);
    // textDisplay->textf(1, "%d", 1000000 / dur);
    textDisplay->textf(1, "%d", to_ms_since_boot(get_absolute_time()) / 1000);

    display->sendBuffer();
  }
}
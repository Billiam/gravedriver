#include <Arduino.h>

#include <Bounce2.h>
#include <RBD_Timer.h>
#include <ResponsiveAnalogRead.h>

#define I2C_PIN_SDA (4u)
#define I2C_PIN_SCL (5u)
#define SOLENOID_PIN (18u)

#define RIGHT_PIN (0u)
#define UP_PIN (1u)

#define POT_PIN A0
#define PEDAL_PIN A2

#define MIN_POWER 64
#define REVERSE_PEDAL true

#define MIN_DURATION 5
#define MAX_DURATION 40

#include "solenoid.h"

#include "fast_math.h"
#include "font_8x6.h"
#include <hardware/i2c.h>
#include <map>
#include <pico/multicore.h>
#include <shapeRenderer/ShapeRenderer.h>
#include <ssd1306.h>
#include <textRenderer/TextRenderer.h>

#include <cmath>

RBD::Timer logger;
Solenoid solenoid = Solenoid(SOLENOID_PIN);

unsigned int solenoidDuration = MIN_DURATION;
int pedalMin;
int pedalMax;
bool pedalRead = false;

int currentCurve = 0;

ResponsiveAnalogRead pedalInput;

Bounce2::Button durationButton;
Bounce2::Button rightButton;
Bounce2::Button upButton;
Bounce2::Button downButton;
Bounce2::Button leftButton;

int solUpdate;

// TODO: Configurable screen dimensions
// TODO: Cap duration to frequency percentage or waveTime -
void plotCurve(pico_ssd1306::SSD1306 *ssd1306, int frequency, int x, int y,
               int width, int height)
{
  double pct = frequency / 1024.0;
  int startY = y + height;
  drawRect(ssd1306, x, y, x + width, startY);

  double curve;
  if (currentCurve == 0) {
    curve = 1.0;
  } else {
    curve = abs(currentCurve);
  }

  if (currentCurve == 0) {
    drawLine(ssd1306, x, startY, x + width, y);
  } else if (currentCurve > 0) {
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

  if (currentCurve == 0) {
    indicatorX = x + pct * width;
    indicatorY = startY - pct * width;
  } else {
    indicatorX = x + pct * width;
    curve = currentCurve > 0 ? curve : 1.0 / curve;
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

  char foo[16];
  snprintf(foo, sizeof foo, "f: %0.2f", spmPercent);
  drawText(ssd1306, font_8x6, foo, 20, 50);

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

  for (int i = 0; i < waves; i++) {
    int x1 = 5 + i * wavelength;
    int x2 = x1 + wavelength - pulseWidth;
    int x3 = x1 + wavelength;
    drawLine(ssd1306, x1, y1, x2, y1);
    drawLine(ssd1306, x2, y1, x2, y2);
    drawLine(ssd1306, x2, y2, x3, y2);
    drawLine(ssd1306, x3, y2, x3, y1);
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

absolute_time_t lastLoopStart;
void displayLoop()
{
  pico_ssd1306::SSD1306 display =
      pico_ssd1306::SSD1306(i2c0, 0x3C, pico_ssd1306::Size::W128xH64);
  display.setOrientation(0);

  display.sendBuffer(); // Send buffer to device and show on screen

  while (1) {
    absolute_time_t now = get_absolute_time();
    int dur = absolute_time_diff_us(lastLoopStart, now);
    lastLoopStart = now;

    display.clear();

    char l1_spm[16];
    int spm = solenoid.spm;
    int freq = solenoid.freq;

    int solenoidPower = solenoid.pow;

    drawWave(&display, 0, freq, spm, solenoid.spmPercent(), solenoidDuration,
             solenoidPower);

    // TODO: track cursor
    snprintf(l1_spm, sizeof l1_spm, "spm: %d", spm);
    drawText(&display, font_8x6, l1_spm, 0, 18);
    drawMeter(&display, 80, 18, 46, 8, freq / 1024.0);

    snprintf(l1_spm, sizeof l1_spm, "pow: %d", solenoidPower);
    drawText(&display, font_8x6, l1_spm, 0, 28);
    drawMeter(&display, 80, 28, 46, 8, solenoidPower / 1024.0);

    snprintf(l1_spm, sizeof l1_spm, "dur: %d", solenoidDuration);
    drawText(&display, font_8x6, l1_spm, 0, 38);
    drawMeter(&display, 80, 38, 46, 8,
              (1.0 * solenoidDuration - MIN_DURATION) /
                  (MAX_DURATION - MIN_DURATION));

    snprintf(l1_spm, sizeof l1_spm, "off: %d", 60000 / spm - solenoidDuration);
    drawText(&display, font_8x6, l1_spm, 0, 48);

    // plotCurve(&display, freq, 5, 5, 55, 55);

    // snprintf(l1_spm, sizeof l1_spm, "%d", solUpdate);
    // drawText(&display, font_8x6, l1_spm, 105, 40);

    snprintf(l1_spm, sizeof l1_spm, "%d", 1000000 / dur);
    drawText(&display, font_8x6, l1_spm, 105, 52);

    display.sendBuffer();
  }
}

void setup()
{
  Serial.begin(9600);
  pinMode(SOLENOID_PIN, OUTPUT);

  rightButton.attach(RIGHT_PIN, INPUT_PULLUP);
  upButton.attach(UP_PIN, INPUT_PULLUP);

  _i2c_init(i2c0, 1000000); // Use i2c port with baud rate of 1Mhz
  // Set pins for I2C operation
  gpio_set_function(I2C_PIN_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_PIN_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_PIN_SDA);
  gpio_pull_up(I2C_PIN_SCL);

  sleep_ms(500);
  multicore_launch_core1(displayLoop);
  sleep_ms(500);
  char test[16];
  snprintf(test, sizeof test, "SPM: %d", solenoid.spm);

  pedalInput.enableSleep();
  pedalInput.setSnapMultiplier(0.01);
  pedalInput.enableEdgeSnap();
  pedalInput.setActivityThreshold(10.0);

  logger.setTimeout(100);
}

int readPedal()
{
  pedalInput.update(analogRead(PEDAL_PIN));
  int val = pedalInput.getValue();
  // if (REVERSE_PEDAL) {
  // val = 1024 - val;
  // }

  if (!pedalRead) {
    pedalRead = true;
    pedalMin = val;
    pedalMax = val;
  }

  if (val > pedalMax) {
    pedalMax = val;
  }
  if (val < pedalMin) {
    pedalMin = val;
  }

  int range = pedalMax - pedalMin;
  if (range < 20) {
    return 0;
  }
  int minThreshold = (int)pedalMin + range * 0.15;
  int maxThreshold = (int)pedalMin + range * 0.9;
  // Serial.print(pedalMin);
  // Serial.print("\t");
  // Serial.print(minThreshold);
  // Serial.print("\t");
  // Serial.print(maxThreshold);
  // Serial.print("\t");
  // Serial.print(pedalMax);
  // Serial.print("\t");
  // Serial.println(val);

  if (val < minThreshold) {
    return 0;
  } else if (val > maxThreshold) {
    return 1023;
  }
  return constrain(map(val, minThreshold, maxThreshold, 0, 1023), 0, 1023);
}

int readPot()
{
  int val = analogRead(POT_PIN);
  if (val < 50) {
    return 0;
  }
  return constrain(map(val, 20, 975, 0, 1023), 0, 1023);
}

int readDuration()
{
  leftButton.update();
  return leftButton.fell();
}

int readCurve()
{
  upButton.update();
  return upButton.fell();
}

void loop()
{
  int pedal = readPedal();
  int pot = readPot();
  int solStart = micros();
  solenoid.update(pedal, pot, solenoidDuration, currentCurve);
  solUpdate = micros() - solStart;

  if (readDuration()) {
    int nextDuration =
        solenoidDuration + (solenoidDuration < MIN_DURATION + 15 ? 1 : 5);
    if (nextDuration > MAX_DURATION) {
      nextDuration = MIN_DURATION;
    }
    solenoidDuration = nextDuration;
  }

  if (readCurve()) {
    int nextCurve = currentCurve + 1;
    if (nextCurve > 6) {
      nextCurve = -6;
    }
    if (abs(nextCurve) == 1) {
      nextCurve++;
    }
    currentCurve = nextCurve;
  }
  // updateDisplay(pedal, pot, solenoidDuration, solenoid.spm);
  // if (logger.onRestart()) {
  //   Serial.println(pedal);
  //   Serial.print("\t");
  //   Serial.print(pot);
  //   Serial.print("\t");
  //   Serial.print(solenoidDuration);
  //   Serial.print("\t");
  //   Serial.println(solenoid.spm);
  //   Serial.println(micros() - t);
  // }
}
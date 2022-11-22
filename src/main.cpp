#include <Arduino.h>

#define I2C_PIN_SDA (4u)
#define I2C_PIN_SCL (5u)
#define SOLENOID_PIN (18u)

#define UP_PIN (3u)
#define RIGHT_PIN (0u)
#define DOWN_PIN (1u)
#define LEFT_PIN (2u)

#define POT_PIN A0
#define PEDAL_PIN A2

#define MIN_POWER 64
#define REVERSE_PEDAL true

#define MIN_DURATION 10
#define MAX_DURATION 40

#define HOLD_BUTTON_DURATION 400

#define MODE_POWER 0x00
#define MODE_DURATION 0x01

#include "drawing.h"
#include "fast_math.h"
#include "font_8x6.h"
#include "graver_menu.h"
#include "hold_button.h"
#include "scene.h"
#include "solenoid.h"
#include "state.h"
#include "step_rotary.h"
#include "text_display.h"
#include <Bounce2.h>
#include <MenuSystem.h>
#include <RBD_Timer.h>
#include <ResponsiveAnalogRead.h>
#include <cmath>
#include <hardware/i2c.h>
#include <map>
#include <pico/multicore.h>
#include <shapeRenderer/ShapeRenderer.h>
#include <ssd1306.h>
#include <textRenderer/TextRenderer.h>

RBD::Timer logger;
Solenoid solenoid = Solenoid(SOLENOID_PIN);

ResponsiveAnalogRead pedalInput;

StepRotary menuKnob(8u, 7u);
// TODO: Make step size configurable at update time?
// or maybe just ignore some number of steps
StepRotary powerKnob(2u, 1u, ONE_STEP);

HoldButton powerKnobButton;
HoldButton menuKnobButton;

stateType state;

/* Issue:
menu needs to be global, because it will be accessed from display and main
loop


Menu needs initializing with renderer
  renderer needs global display
Menu items need global actions
  actions need state
    state must be global

*/

pico_ssd1306::SSD1306 *display;
TextDisplay *textDisplay;

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

void drawStatus(pico_ssd1306::SSD1306 *ssd1306)
{
  int spm = solenoid.spm;
  int freq = solenoid.freq;

  int solenoidPower = state.power;

  drawWave(ssd1306, 0, freq, spm, solenoid.spmPercent(), state.duration,
           solenoidPower);

  textDisplay->setCursor(8, 18);
  drawMeter(ssd1306, 80, textDisplay->getCursorY(), 46, 8, freq / 1024.0);
  textDisplay->textfln(1, "spm: %d", spm);
  // drawText(ssd1306, font_8x6, buff, 0, 18);

  drawMeter(ssd1306, 80, textDisplay->getCursorY(), 46, 8,
            solenoidPower / 1024.0);
  if (state.powerMode == MODE_POWER) {
    addAdafruitBitmap(ssd1306, textDisplay->getCursorX() - 8,
                      textDisplay->getCursorY(), 8, 8, arrow);
  }
  textDisplay->textfln(1, "pow: %d", solenoidPower);

  drawMeter(ssd1306, 80, textDisplay->getCursorY(), 46, 8,
            (1.0 * state.duration - MIN_DURATION) /
                (MAX_DURATION - MIN_DURATION));
  if (state.powerMode == MODE_DURATION) {
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
  plotCurve(ssd1306, solenoid.freq, 5, 5, 55, 55);
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
    int dur = absolute_time_diff_us(lastLoopStart, now);
    lastLoopStart = now;

    textDisplay->setCursor(0, 0);
    displayInst.clear();

    switch (state.scene) {
      case SCENE_STATUS:
        drawStatus(display);
        break;
      case SCENE_MENU:
        drawMenu(display);
        break;
      case SCENE_CURVE:
        drawCurve(display);
        break;
    }

    // display FPS
    textDisplay->setCursor(105, 56);
    textDisplay->textf(1, "%d", 1000000 / dur);

    display->sendBuffer();
  }
}

void setup()
{
  Serial.begin(9600);
  pinMode(SOLENOID_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(1u, INPUT_PULLUP);
  pinMode(2u, INPUT_PULLUP);

  state.scene = SCENE_STATUS;
  state.duration = MIN_DURATION;
  state.pedalRead = false;
  state.curve = 0;
  state.power = 0;
  state.powerMode = MODE_POWER;

  menuKnob.begin();
  powerKnob.begin();

  powerKnobButton.attach(3u, INPUT_PULLUP);
  powerKnobButton.setPressedState(LOW);
  menuKnobButton.attach(6u, INPUT_PULLUP);
  menuKnobButton.setPressedState(LOW);

  _i2c_init(i2c0, 1000000); // Use i2c port with baud rate of 1Mhz
  // Set pins for I2C operation
  gpio_set_function(I2C_PIN_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_PIN_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_PIN_SDA);
  gpio_pull_up(I2C_PIN_SCL);

  buildMenu();

  pedalInput.enableSleep();
  pedalInput.setSnapMultiplier(0.01);
  pedalInput.enableEdgeSnap();
  pedalInput.setActivityThreshold(10.0);

  sleep_ms(500);
  multicore_launch_core1(displayLoop);
  sleep_ms(500);
  // logger.setTimeout(100);
}

int readPedal()
{
  pedalInput.update(analogRead(PEDAL_PIN));
  int val = pedalInput.getValue();
  // if (REVERSE_PEDAL) {
  // val = 1024 - val;
  // }

  if (!state.pedalRead) {
    state.pedalRead = true;
    state.pedalMin = val;
    state.pedalMax = val;
  }

  if (val > state.pedalMax) {
    state.pedalMax = val;
  }
  if (val < state.pedalMin) {
    state.pedalMin = val;
  }

  int range = state.pedalMax - state.pedalMin;
  if (range < 20) {
    return 0;
  }
  int minThreshold = state.pedalMin + range * 0.15;
  int maxThreshold = state.pedalMin + range * 0.9;
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

int loopValue(int val, int amount, int min, int max)
{
  int range = max - min;

  return (val - min + (amount % range) + range) % range + min;
}

void updatePower()
{
  char powerDir = powerKnob.process();
  if (!powerDir) {
    return;
  }

  int dir = powerDir == DIR_CW ? 1 : -1;

  if (state.powerMode == MODE_POWER) {
    state.power = constrain(state.power + dir * 8, 0, 1023);
  } else {
    state.duration =
        constrain(state.duration + dir, MIN_DURATION, MAX_DURATION);
  }
}

void updateButtons()
{
  menuKnobButton.update();
  powerKnobButton.update();
}

void statusLoop()
{
  updateButtons();

  if (menuKnob.process() || menuKnobButton.wasReleased()) {
    Serial.println("Changing to menu");
    state.scene = SCENE_MENU;
  }

  if (powerKnobButton.wasReleased()) {
    if (state.powerMode == MODE_DURATION) {
      state.powerMode = MODE_POWER;
      powerKnob.setStep(ONE_STEP);
    } else {
      state.powerMode = MODE_DURATION;
      powerKnob.setStep(FULL_STEP);
    }
  }
}

void menuLoop()
{
  updateButtons();
  char menuVal = menuKnob.process();

  if (menuVal == DIR_CW) {
    menu.next(true);
  } else if (menuVal == DIR_CCW) {
    menu.prev(true);
  }

  if (menuKnobButton.wasHeld(HOLD_BUTTON_DURATION)) {
    const Menu *current_menu = menu.get_current_menu();
    bool skipBack = false;

    if (current_menu != nullptr) {
      const MenuComponent *current_component =
          current_menu->get_current_component();
      if (current_component != nullptr) {
        if (current_component->has_focus()) {
          skipBack = true;
          menu.select();
        }
      }
    }

    if (!skipBack && !menu.back()) {
      state.scene = SCENE_STATUS;
    }
  } else if (menuKnobButton.wasReleased()) {
    Serial.println("Select");
    menu.select();
  }
}

void curveLoop()
{
  updateButtons();
  char menuVal = menuKnob.process();

  if (menuVal) {
    int dir = menuVal == DIR_CW ? 1 : -1;
    int nextCurve = state.curve + dir;
    nextCurve = (nextCurve + 19) % 13 - 6;
    if (abs(nextCurve) == 1) {
      nextCurve += dir;
    }
    Serial.println(nextCurve);
    state.curve = nextCurve;
  }

  if (menuKnobButton.wasReleased()) {
    Serial.println("Back to menu");
    state.scene = SCENE_MENU;
  }
}

void updateSolenoid()
{
  updatePower();

  int pedal = readPedal();
  solenoid.update(pedal, state.power, state.duration, state.curve);
}

// TODO: Could change a pointer to a different scene instead
void loop()
{
  updateSolenoid();

  switch (state.scene) {
    case SCENE_STATUS:
      // code block
      statusLoop();
      break;
    case SCENE_MENU:
      menuLoop();
      // code block
      break;
    case SCENE_CURVE:
      curveLoop();
      break;
  }

  // updateDisplay(pedal, pot, state.duration, solenoid.spm);
  // if (logger.onRestart()) {
  //   Serial.println(pedal);
  //   Serial.print("\t");
  //   Serial.print(pot);
  //   Serial.print("\t");
  //   Serial.print(state.duration);
  //   Serial.print("\t");
  //   Serial.println(solenoid.spm);
  //   Serial.println(micros() - t);
  // }
}
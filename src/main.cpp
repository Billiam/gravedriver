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

// TODO: Move to state and fram
#define MIN_DURATION 10
#define MAX_DURATION 40

#define HOLD_BUTTON_DURATION 400

#include "graver_menu.h"
#include "hold_button.h"
#include "power_mode.h"
#include "rendering.h"
#include "scene.h"
#include "solenoid.h"
#include "state.h"
#include "step_rotary.h"
#include <Bounce2.h>
#include <MenuSystem.h>
#include <RBD_Timer.h>
#include <ResponsiveAnalogRead.h>
#include <cmath>
#include <hardware/i2c.h>
#include <map>
#include <pico/multicore.h>

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

void setup()
{
  Serial.begin(9600);
  pinMode(SOLENOID_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(1u, INPUT_PULLUP);
  pinMode(2u, INPUT_PULLUP);

  state.scene = Scene::STATUS;
  state.duration = MIN_DURATION;
  state.pedalRead = false;
  state.curve = 0;
  state.power = 0;
  state.powerMode = PowerMode::POWER;
  state.pedalMin = 0;
  state.pedalMax = 1024;

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

  if (val < state.pedalMin) {
    return 0;
  } else if (val > state.pedalMax) {
    return 1023;
  }
  return constrain(map(val, state.pedalMin, state.pedalMax, 0, 1023), 0, 1023);
}

int loopValue(int val, int amount, int min, int max)
{
  int range = max - min + 1;

  return (val - min + (amount % range) + range) % range + min;
}

void updatePower()
{
  char powerDir = powerKnob.process();
  if (!powerDir) {
    return;
  }

  int dir = powerDir == DIR_CW ? 1 : -1;

  if (state.powerMode == PowerMode::POWER) {
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
    state.scene = Scene::MENU;
  }

  if (powerKnobButton.wasReleased()) {
    if (state.powerMode == PowerMode::DURATION) {
      state.powerMode = PowerMode::POWER;
      powerKnob.setStep(ONE_STEP);
    } else {
      state.powerMode = PowerMode::DURATION;
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
      state.scene = Scene::STATUS;
    }
  } else if (menuKnobButton.wasReleased()) {
    menu.select();
  }
}

void curveLoop()
{
  updateButtons();
  char menuVal = menuKnob.process();

  if (menuVal) {
    int dir = menuVal == DIR_CW ? 1 : -1;
    int nextCurve = loopValue(state.curve, dir, -6, 6);
    if (abs(nextCurve) == 1) {
      nextCurve += dir;
    }
    state.curve = nextCurve;
  }

  if (menuKnobButton.wasReleased()) {
    state.scene = Scene::MENU;
  }
}

void calibrateLoop()
{
  static bool calibrationActive = false;

  pedalInput.update(analogRead(PEDAL_PIN));
  updateButtons();

  int val = pedalInput.getValue();

  // TODO: Add start/cancel calibration buttons/submenu
  if (!calibrationActive) {
    solenoid.disable();
    calibrationActive = true;
    state.pedalMin = val;
    state.pedalMax = val;
  }

  if (val > state.pedalMax) {
    state.pedalMax = val;
  }
  if (val < state.pedalMin) {
    state.pedalMin = val;
  }

  if (menuKnobButton.wasReleased()) {
    calibrationActive = false;
    solenoid.enable();
    state.scene = Scene::MENU;
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
    case Scene::STATUS:
      statusLoop();
      break;
    case Scene::MENU:
      menuLoop();
      break;
    case Scene::CURVE:
      curveLoop();
      break;
    case Scene::CALIBRATE:
      calibrateLoop();
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
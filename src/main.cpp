#include <Arduino.h>

#define I2C_PIN_SDA (4u)
#define I2C_PIN_SCL (5u)
#define SOLENOID_PIN (21u)

#define POT_PIN A0
#define PEDAL_PIN A2

#define REVERSE_PEDAL true

#define POWER_PIN_1 (2u)
#define POWER_PIN_2 (1u)
#define POWER_BUTTON_PIN (3u)

#define MENU_PIN_1 (8u)
#define MENU_PIN_2 (7u)
#define MENU_BUTTON_PIN (6u)

#define SPI_CCK_PIN (18u)
#define SPI_MOSI_PIN (19u)
#define SPI_MISO_PIN (16u)

#define OLED_RESET_PIN (15u)

#define FRAM_CS_PIN (20u)

// TODO: Move to state and fram
#define MIN_DURATION 2
#define MAX_DURATION 40

#define MIN_CURVE -6
#define MAX_CURVE 6

#define HOLD_BUTTON_DURATION 400

#define SPI_INTERFACES_COUNT 1

#include "Adafruit_FRAM_SPI.h"
#include "fast_math.h"
#include "graver_menu.h"
#include "hold_button.h"
#include "power_mode.h"
#include "rendering.h"
#include "scene.h"
#include "solenoid.h"
#include "state.h"
#include "step_rotary.h"
#include "store.h"
#include "vendor/MenuSystem.h"
#include <Bounce2.h>
#include <RBD_Timer.h>
#include <ResponsiveAnalogRead.h>
#include <SPI.h>
#include <cmath>
#include <hardware/i2c.h>
#include <map>
#include <mbed.h>
#include <pico/multicore.h>

RBD::Timer logger;
RBD::Timer resetTimer(1100);

Solenoid solenoid = Solenoid(SOLENOID_PIN);

ResponsiveAnalogRead pedalInput;

StepRotary menuKnob(MENU_PIN_1, MENU_PIN_2);
// TODO: Make step size configurable at update time?
// or maybe just ignore some number of steps
StepRotary powerKnob(POWER_PIN_1, POWER_PIN_2, ONE_STEP);

HoldButton powerKnobButton;
HoldButton menuKnobButton;

stateType state;

Adafruit_FRAM_SPI fram = Adafruit_FRAM_SPI(FRAM_CS_PIN);
Store store = Store(&fram);

void initializeState()
{
  state.confirmPct = 0;
  state.confirmSelected = true;
}

void initializeFram()
{
  if (fram.begin(2)) {
    Serial.println("SRAM found");
  } else {
    Serial.println("NO SRAM found");
  }

  state.graverCount = constrain(store.readUint(FramKey::GRAVER_COUNT), 1, 4);
  state.graver = min(store.readUint(FramKey::GRAVER), state.graverCount - 1);

  char name[] = "";
  store.readChars(state.graver, FramKey::NAME, name, 8);
  sleep_ms(3000);

  if (strlen(name) == 0) {
    snprintf(name, 8, "#%d", state.graver + 1);
  }
  strncpy(state.graverName, name, 8);

  // initialize from fram
  state.scene = Scene::STATUS;
  state.power = 0;

  state.powerMode = store.readUint(state.graver, FramKey::POWER_MODE) ? PowerMode::DURATION : PowerMode::POWER;
  state.pedalMode = store.readUint(state.graver, FramKey::PEDAL_MODE) ? PedalMode::POWER : PedalMode::FREQUENCY;

  int8_t curve = store.readUint(state.graver, FramKey::CURVE) + MIN_CURVE;
  state.curve = constrain(curve, MIN_CURVE, MAX_CURVE);

  uint16_t power = min(store.readUint16(state.graver, FramKey::POWER), 1023);
  if (state.pedalMode == PedalMode::FREQUENCY) {
    state.power = power;
  } else {
    state.frequency = power;
  }

  state.duration = constrain(store.readUint(state.graver, FramKey::DURATION), MIN_DURATION, MAX_DURATION);
  state.pedalMax = min(1023, store.readUint16(FramKey::PEDAL_MAX));
  state.pedalMin = min(store.readUint16(FramKey::PEDAL_MIN), state.pedalMax);
  state.powerMin = min(128, store.readUint(state.graver, FramKey::POWER_MIN));

  uint16_t spmMax = store.readUint16(state.graver, FramKey::FREQUENCY_MAX);
  state.spmMin = constrain(store.readUint16(state.graver, FramKey::FREQUENCY_MIN), 5, 4000);
  state.spmMax = constrain(spmMax, state.spmMin, 4000);
}

void setup()
{
  Serial.begin(9600);

  pinMode(SOLENOID_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(OLED_RESET_PIN, OUTPUT);
  digitalWrite(OLED_RESET_PIN, LOW);

  menuKnob.begin();
  powerKnob.begin();

  powerKnobButton.attach(POWER_BUTTON_PIN, INPUT_PULLUP);
  powerKnobButton.setPressedState(LOW);
  menuKnobButton.attach(MENU_BUTTON_PIN, INPUT_PULLUP);
  menuKnobButton.setPressedState(LOW);

  _i2c_init(i2c0, 700000); // Use i2c port with baud rate of 1Mhz
  // Set pins for I2C operation
  gpio_set_function(I2C_PIN_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_PIN_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_PIN_SDA);
  gpio_pull_up(I2C_PIN_SCL);

  // Initialize SPI pins
  gpio_set_dir(FRAM_CS_PIN, GPIO_OUT);
  gpio_put(FRAM_CS_PIN, 1);

  gpio_set_function(SPI_CCK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(SPI_MISO_PIN, GPIO_FUNC_SPI);
  gpio_set_function(SPI_MOSI_PIN, GPIO_FUNC_SPI);

  initializeState();
  initializeFram();
  buildMenu();

  pedalInput.enableSleep();
  pedalInput.setSnapMultiplier(0.01);
  pedalInput.enableEdgeSnap();
  pedalInput.setActivityThreshold(10.0);

  digitalWrite(OLED_RESET_PIN, HIGH);
  sleep_ms(500);

  multicore_launch_core1(displayLoop);
  sleep_ms(500);
}

void updatePedal()
{
  pedalInput.update(analogRead(PEDAL_PIN));
  int val = pedalInput.getValue();

  // if (REVERSE_PEDAL) {
  // val = 1024 - val;
  // }
  int output = constrain(
      val < 1 ? 0 : map(val, state.pedalMin, state.pedalMax, 1, 1023), 0, 1023);
  if (state.pedalMode == PedalMode::FREQUENCY) {
    state.frequency = output;
  } else {
    state.power = output;
  }
}

int loopValue(int val, int amount, int min, int max)
{
  int range = max - min + 1;

  return (val - min + (amount % range) + range) % range + min;
}

unsigned int unsignedConstrain(unsigned int val, unsigned int max = 1023)
{
  if (val > 65000) {
    return 0;
  }
  if (val > max) {
    return max;
  }
  return val;
}

void updatePower()
{
  char powerDir = powerKnob.process();
  if (!powerDir) {
    return;
  }

  int dir = powerDir == DIR_CW ? 1 : -1;

  if (state.powerMode == PowerMode::POWER) {
    if (state.pedalMode == PedalMode::FREQUENCY) {
      state.power = unsignedConstrain(state.power + 8 * dir);
      store.writeUint16(state.graver, FramKey::POWER, state.power);
    } else {
      state.frequency = unsignedConstrain(state.frequency + 8 * dir);
      store.writeUint16(state.graver, FramKey::POWER, state.frequency);
    }
  } else {
    state.duration =
        constrain(state.duration + dir, MIN_DURATION, MAX_DURATION);
    store.writeUint(state.graver, FramKey::DURATION, state.duration);
  }
}

void updateButtons()
{
  menuKnobButton.update();
  powerKnobButton.update();
}

void setPowerMode(PowerMode mode)
{
  state.powerMode = mode;

  if (mode == PowerMode::POWER) {
    powerKnob.setStep(ONE_STEP);
  } else {
    powerKnob.setStep(FULL_STEP);
  }
}

void statusLoop()
{
  updateButtons();
  PowerMode oldPower = state.powerMode;
  PedalMode oldPedal = state.pedalMode;

  if (menuKnob.process() || menuKnobButton.wasReleased()) {
    state.scene = Scene::MENU;
  }
  if (powerKnobButton.wasHeld(HOLD_BUTTON_DURATION)) {
    // todo: share state toggle methods
    if (state.pedalMode == PedalMode::FREQUENCY) {
      state.pedalMode = PedalMode::POWER;
    } else {
      state.pedalMode = PedalMode::FREQUENCY;
    }

    // set power mode back to power after changing pedal mode
    if (state.powerMode == PowerMode::DURATION) {
      setPowerMode(PowerMode::POWER);
    }
  } else if (powerKnobButton.wasReleased()) {
    if (state.powerMode == PowerMode::DURATION) {
      setPowerMode(PowerMode::POWER);
    } else {
      setPowerMode(PowerMode::DURATION);
    }
  }

  if (state.powerMode != oldPower) {
    store.writeUint(state.graver, FramKey::POWER_MODE, static_cast<uint8_t>(state.powerMode));
  }
  if (state.pedalMode != oldPedal) {
    store.writeUint(state.graver, FramKey::PEDAL_MODE, static_cast<uint8_t>(state.pedalMode));
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
    int nextCurve = loopValue(state.curve, dir, MIN_CURVE, MAX_CURVE);
    if (abs(nextCurve) == 1) {
      nextCurve += dir;
    }
    state.curve = nextCurve;
    store.writeUint(state.graver, FramKey::CURVE, state.curve - MIN_CURVE);
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

    store.writeUint16(FramKey::PEDAL_MIN, val);
    store.writeUint16(FramKey::PEDAL_MAX, val);
  }

  if (val > state.pedalMax) {
    state.pedalMax = val;
    store.writeUint16(FramKey::PEDAL_MAX, val);
  }
  if (val < state.pedalMin) {
    state.pedalMin = val;
    store.writeUint16(FramKey::PEDAL_MIN, val);
  }

  if (menuKnobButton.wasReleased()) {
    calibrationActive = false;
    solenoid.enable();
    state.scene = Scene::MENU;
  }
}

void shutdownLoop()
{
  static bool once = []() {
    solenoid.disable();
    resetTimer.restart();
    store.clear();
    return true;
  }();

  if (resetTimer.onExpired()) {
    mbed::Watchdog::get_instance()
        .start(1);
    while (1)
      ;
  }
}

void resetLoop()
{
  updateButtons();

  if (menuKnob.process()) {
    state.confirmSelected = !state.confirmSelected;
  }

  if (!state.confirmSelected) {
    state.confirmPct = 0;
    if (menuKnobButton.wasReleased()) {
      state.scene = Scene::MENU;
    }
    return;
  }

  if (menuKnobButton.wasHeld(2000)) {
    state.confirmTime = get_absolute_time();
    state.scene = Scene::SHUTDOWN;
  } else if (menuKnobButton.isPressed()) {
    state.confirmPct = min(menuKnobButton.currentDuration() / 2000.0, 1.0);
  } else {
    state.confirmPct = 0;
  }
}

int curveInput(int val, int curve)
{
  if (curve == 0) {
    return val;
  }

  double exp = curve > 0 ? curve : -1.0 / curve;
  return fastPrecisePow(val / 1024.0, exp) * 1024;
}

void updateSolenoid()
{
  updatePower();
  updatePedal();

  int frequency = state.frequency;
  int power = state.power;
  if (state.pedalMode == PedalMode::FREQUENCY) {
    frequency = curveInput(frequency, state.curve);
  } else {
    power = curveInput(power, state.curve);
  }

  solenoid.update(frequency, power, state.duration);
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
    case Scene::RESET:
      resetLoop();
      break;
    case Scene::SHUTDOWN:
      shutdownLoop();
      break;
  }
}

#include <Arduino.h>

#include <RBD_Timer.h>
#include <Bounce2.h>
#include <Smoothed.h>

#define I2C_PIN_SDA (4u)
#define I2C_PIN_SCL (5u)
#define SOLENOID_PIN (18u)
#define DURATION_PIN (15u)

#define POT_PIN A0
#define PEDAL_PIN A1

#define MIN_POWER 64
#define REVERSE_PEDAL true

#include "solenoid.h"

#include <pico/multicore.h>
#include <ssd1306.h>
#include <textRenderer/TextRenderer.h>
#include <shapeRenderer/ShapeRenderer.h>
#include <hardware/i2c.h>
#include "font_8x6.h"

RBD::Timer logger;
Solenoid solenoid = Solenoid(SOLENOID_PIN);

unsigned int solenoidDuration = 10;
int pedalMin;
int pedalMax;
bool pedalRead = false;
Smoothed <int> pedalInput;

Bounce2::Button durationButton = Bounce2::Button();

void displayLoop() {
  pico_ssd1306::SSD1306 display = pico_ssd1306::SSD1306(i2c0, 0x3C, pico_ssd1306::Size::W128xH64);
  display.setOrientation(0);

  display.sendBuffer(); //Send buffer to device and show on screen

  while(1) {
    display.clear();

    char l1_spm[16];
    int solenoidVal = solenoid.spm;
    int solenoidPower = solenoid.pow;
    snprintf(l1_spm, sizeof l1_spm, "spm: %d", solenoidVal);
    drawText(&display, font_8x6, l1_spm, 0, 0);
    snprintf(l1_spm, sizeof l1_spm, "pow: %d", solenoidPower);
    drawText(&display, font_8x6, l1_spm, 0, 10);
    snprintf(l1_spm, sizeof l1_spm, "dur: %d", solenoidDuration);
    drawText(&display, font_8x6, l1_spm, 0, 20);

    drawRect(&display, 80,0,127,8);
    if (solenoidVal > 0) {
      int barWidth = (int)((125 - 82) * (solenoidVal/2400.0));
      fillRect(&display, 82, 2, 82 + barWidth, 6);
    }
    
    display.sendBuffer();
  }
}


void setup() {
  Serial.begin(9600);
  pinMode(SOLENOID_PIN, OUTPUT);

  durationButton.attach(DURATION_PIN, INPUT_PULLUP);

  _i2c_init(i2c0, 1000000); //Use i2c port with baud rate of 1Mhz
  //Set pins for I2C operation
  gpio_set_function(I2C_PIN_SDA, GPIO_FUNC_I2C);
  gpio_set_function(I2C_PIN_SCL, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_PIN_SDA);
  gpio_pull_up(I2C_PIN_SCL);

  sleep_ms(500);
  multicore_launch_core1(displayLoop);
  sleep_ms(500);
  char test[16];
  snprintf(test, sizeof test, "SPM: %d", solenoid.spm);

  pedalInput.begin(SMOOTHED_AVERAGE, 10);
}

int readPedal() {
  pedalInput.add(analogRead(PEDAL_PIN));
  int val = pedalInput.get();

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

int readPot() {
  int val = analogRead(POT_PIN);
  if (val < 50) {
    return 0;
  }
  return constrain(map(val, 20, 975, 0, 1023), 0, 1023);
}

int readDuration() {
  durationButton.update();
  return durationButton.fell();
}

void loop() {
  int pedal = readPedal();
  int pot = readPot();
  solenoid.update(pedal, pot, solenoidDuration);

  if (readDuration()) {
    solenoidDuration = solenoidDuration + (solenoidDuration < 20 ? 1 : 5);
    if (solenoidDuration > 80) {
      solenoidDuration = 10;
    }
  }

  // updateDisplay(pedal, pot, solenoidDuration, solenoid.spm);
  if (logger.onRestart()) {
    // Serial.println(pedal);
  //   Serial.print("\t");
  //   Serial.print(pot);
  //   Serial.print("\t");
  //   Serial.print(solenoidDuration);
  //   Serial.print("\t");
  //   Serial.println(solenoid.spm);
    // Serial.println(micros() - t);
  }
}
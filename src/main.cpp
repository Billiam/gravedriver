#include <Arduino.h>

#include <RBD_Timer.h>
#include <Bounce2.h>

#define SSD1306_NO_SPLASH true
#define POT_D_PIN (17u)
#define PEDAL_D_PIN (16u)
#define SOLENOID_PIN (18u)
#define DURATION_PIN (15u)

#define POT_PIN A0
#define PEDAL_PIN A1

#define MIN_DELAY 10
#define MAX_DELAY 1500

#define MIN_POWER 64
#define REVERSE_PEDAL true

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "solenoid.h"

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 64, &Wire);

RBD::Timer logger;
Solenoid solenoid = Solenoid(SOLENOID_PIN);

unsigned int solenoidDuration = 75;
int pedalMin;
int pedalMax;
bool pedalRead = false;

Bounce2::Button durationButton = Bounce2::Button();

void setup() {
  Serial.begin(9600);

  // text display tests
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);

  logger.setTimeout(250);

  pinMode(PEDAL_PIN, INPUT_PULLDOWN);
  pinMode(POT_PIN, INPUT);

  pinMode(SOLENOID_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  durationButton.attach(DURATION_PIN, INPUT_PULLUP);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();
}

int readPedal() {
  int val = analogRead(PEDAL_PIN);
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
  int zeroRange = (int)max(range * 0.2, 40);
  int maxRange = (int)min(range * 0.95, 30) + 1023;

  return constrain(map(val, pedalMin, pedalMax, -zeroRange, maxRange), 0, 1023);
}

int readPot() {
  // TODO: use same range for pot
  int val = analogRead(POT_PIN);
  if (val < 60) {
    return 0;
  }
  return constrain(map(val, 20, 975, MIN_POWER, 128), MIN_POWER, 128);
}

int readDuration() {
  durationButton.update();
  return durationButton.fell();
}

void updateDisplay(int frequency, int power, int duration, int spm)
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("SPM: ");
  display.println(spm);
  display.print("Freq: ");
  display.println(frequency);
  display.print("Pow: ");
  display.println(power - MIN_POWER);
  display.print("Dur: ");
  display.println(duration);
  
  display.display();
}

unsigned long lastTime;
void loop() {
  Serial.println(micros() - lastTime);
  lastTime = micros();

  int pedal = readPedal();
  int pot = readPot();
  solenoid.update(pedal, pot, solenoidDuration);

  if (readDuration()) {
    solenoidDuration = solenoidDuration + (solenoidDuration < 10 ? 1 : 5);
    if (solenoidDuration > 120) {
      solenoidDuration = 1;
    }
  }

  updateDisplay(pedal, pot, solenoidDuration, solenoid.spm);

  // if (logger.onRestart()) {   
    // Serial.print(pedal);
    // Serial.print("\t");
    // Serial.print(pot);
    // Serial.print("\t");
    // Serial.println(solenoid.spm);
  // }
}
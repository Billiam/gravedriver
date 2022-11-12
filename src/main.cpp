#include <Arduino.h>

#include <RBD_Timer.h>
#include <Bounce2.h>
#include <ResponsiveAnalogRead.h>

#define I2C_PIN_SDA (4u)
#define I2C_PIN_SCL (5u)
#define SOLENOID_PIN (18u)
#define DURATION_PIN (15u)

#define POT_PIN A0
#define PEDAL_PIN A1

#define MIN_POWER 64
#define REVERSE_PEDAL true

#define MIN_DURATION 10
#define MAX_DURATION 80

#include "solenoid.h"

#include <pico/multicore.h>
#include <ssd1306.h>
#include <textRenderer/TextRenderer.h>
#include <shapeRenderer/ShapeRenderer.h>
#include <hardware/i2c.h>
#include "font_8x6.h"

RBD::Timer logger;
Solenoid solenoid = Solenoid(SOLENOID_PIN);

unsigned int solenoidDuration = MIN_DURATION;
int pedalMin;
int pedalMax;
bool pedalRead = false;

ResponsiveAnalogRead pedalInput;

Bounce2::Button durationButton = Bounce2::Button();

// TODO: Configurable screen dimensions
// TODO: Cap duration to frequency percentage or waveTime -
void drawWave(pico_ssd1306::SSD1306 *ssd1306, uint8_t y, int frequency, int spm, int duration, int power) {
  const int maxHeight = 14;
  const int maxLen = 40;

  int y1 = y + maxHeight;

  if (frequency == 0) {
    drawLine(ssd1306, 5, y1, 123, y1);
    return;
  }

  int height = power == 0 ? 0 : map(power, 1, 1023, 1, maxHeight);
  int wavelength = map(frequency, 1023, 0, 5, maxLen);
  double waveTime = 60000.0/spm;
  int pulseWidth = ceil(min(1, duration/waveTime) * wavelength);
  int waves = ceil(118.0/wavelength);

  int y2 = y1 - height;
  
  for (int i=0;i<waves;i++) {
    int x1 = 5 + i * wavelength;
    int x2 = x1 + wavelength - pulseWidth;
    int x3 = x1 + wavelength;
    drawLine(ssd1306, x1, y1, x2, y1);
    drawLine(ssd1306, x2, y1, x2, y2);
    drawLine(ssd1306, x2, y2, x3, y2);
    drawLine(ssd1306, x3, y2, x3, y1);
  }
}

void drawMeter(pico_ssd1306::SSD1306 *ssd1306, int x, int y, int width, int height, double percent) {
  drawRect(ssd1306, x, y, x + width, y + height);
  if (percent > 0) {
    int barWidth = (int)(width - 4) * percent;
    fillRect(ssd1306, x + 2, y + 2, x + 2 + barWidth, y + height - 2);
  }
}

int lastDur = 0;

void displayLoop() {
  pico_ssd1306::SSD1306 display = pico_ssd1306::SSD1306(i2c0, 0x3C, pico_ssd1306::Size::W128xH64);
  display.setOrientation(0);

  display.sendBuffer(); //Send buffer to device and show on screen
  
  while(1) {
    // unsigned long startTime = micros();
    absolute_time_t startTime = get_absolute_time();
    display.clear();

    char l1_spm[16];
    int spm = solenoid.spm;
    int freq = solenoid.freq;
    
    int solenoidPower = solenoid.pow;

    drawWave(&display, 0, freq, spm, solenoidDuration, solenoidPower);

    // TODO: track cursor
    snprintf(l1_spm, sizeof l1_spm, "spm: %d", spm);
    drawText(&display, font_8x6, l1_spm, 0, 18);
    drawMeter(&display, 80, 18, 46, 8, spm/2400.0);

    snprintf(l1_spm, sizeof l1_spm, "pow: %d", solenoidPower);
    drawText(&display, font_8x6, l1_spm, 0, 28);
    drawMeter(&display, 80, 28, 46, 8, solenoidPower/1024.0);

    snprintf(l1_spm, sizeof l1_spm, "dur: %d", solenoidDuration);
    drawText(&display, font_8x6, l1_spm, 0, 38);
    drawMeter(&display, 80, 38, 46, 8, (1.0 * solenoidDuration - MIN_DURATION)/(MAX_DURATION - MIN_DURATION));

    snprintf(l1_spm, sizeof l1_spm, "off: %d", 60000/spm - solenoidDuration);
    drawText(&display, font_8x6, l1_spm, 0, 48);
    
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

  pedalInput.enableSleep();
  pedalInput.setSnapMultiplier(0.01);
  pedalInput.enableEdgeSnap();
  pedalInput.setActivityThreshold(10.0);

  logger.setTimeout(100);
}

int readPedal() {
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
    solenoidDuration = solenoidDuration + (solenoidDuration < MIN_DURATION + 10 ? 1 : 5);
    if (solenoidDuration > MAX_DURATION) {
      solenoidDuration = MIN_DURATION;
    }
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
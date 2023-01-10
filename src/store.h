#ifndef store_h_
#define store_h_

#include <Adafruit_FRAM_SPI.h>
#include <Arduino.h>
#include <map>

#define GRAVER_SIZE 100
#define GRAVER_START 80

#define FRAM_SIZE 8192

enum class FramKey : const unsigned char {
  NAME = 1,
  POWER = 2,
  POWER_MODE = 3,
  PEDAL_MODE = 4,
  CURVE = 5,
  PEDAL_MIN = 6,
  PEDAL_MAX = 7,
  POWER_MIN = 8,

  FREQUENCY_MIN = 9,
  FREQUENCY_MAX = 10,

  DURATION = 11,
  BRIGHTNESS = 12,
  GRAVER = 13
};

const std::map<FramKey, uint32_t> FramAddr = {
    // {FramKey::GRAVER_COUNT, 0}, // 8
    {FramKey::GRAVER, 3},     // 8
    {FramKey::PEDAL_MIN, 6},  // 16
    {FramKey::PEDAL_MAX, 11}, // 16
    {FramKey::BRIGHTNESS, 17} // 8
};

const std::map<FramKey, uint32_t> GraverAddr = {
    {FramKey::NAME, 0},           // 8 char
    {FramKey::POWER, 17},         // 16
    {FramKey::POWER_MODE, 22},    // 8
    {FramKey::PEDAL_MODE, 25},    // 8
    {FramKey::CURVE, 28},         // 8
    {FramKey::POWER_MIN, 31},     // 8
    {FramKey::FREQUENCY_MIN, 34}, // 16
    {FramKey::FREQUENCY_MAX, 39}, // 16
    {FramKey::DURATION, 44},      // 8
};

class Store
{
public:
  Store(Adafruit_FRAM_SPI *fram);

  uint8_t readUint(uint8_t graver, FramKey key);
  uint8_t readUint(FramKey key);
  uint8_t readUint(uint32_t position);

  uint16_t readUint16(uint8_t graver, FramKey key);
  uint16_t readUint16(FramKey key);
  uint16_t readUint16(uint32_t position);

  void readChars(uint8_t graver, FramKey key, char *result, uint8_t count);
  void readChars(FramKey key, char *result, uint8_t count);
  void readChars(uint32_t position, char *result, uint8_t count);

  void writeUint(uint8_t graver, FramKey key, uint8_t value);
  void writeUint(FramKey key, uint8_t value);
  void writeUint(uint32_t position, uint8_t value);

  void writeChars(uint8_t graver, FramKey key, const char *value, uint8_t count);
  void writeChars(FramKey key, const char *value, uint8_t count);
  void writeChars(uint32_t position, const char *value, uint8_t count);

  void writeUint16(uint8_t graver, FramKey key, uint16_t value);
  void writeUint16(FramKey key, uint16_t value);
  void writeUint16(uint32_t position, uint16_t value);

  void clear();

protected:
  Adafruit_FRAM_SPI *_fram;

  void setReadLocation(uint32_t position, uint8_t offset);
  uint8_t readOffset(uint32_t position, uint8_t size);
  uint8_t nextOffset(uint32_t position, uint8_t size);
  uint32_t graverOffset(uint8_t graver, FramKey key);
};
#endif
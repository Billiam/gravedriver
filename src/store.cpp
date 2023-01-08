#include "store.h"
#include <Adafruit_FRAM_SPI.h>

Store::Store(Adafruit_FRAM_SPI *fram) : _fram(fram){};

uint8_t Store::readOffset(uint32_t position, uint8_t size)
{
  if (_fram->read8(position) > 0) {
    return size + 1;
  } else {
    return 1;
  }
}

uint8_t Store::nextOffset(uint32_t position, uint8_t size)
{
  if (_fram->read8(position) > 0) {
    return 1;
  } else {
    return size + 1;
  }
}

uint32_t Store::graverOffset(uint8_t graver, FramKey key)
{
  return graver * GRAVER_SIZE + GRAVER_START + GraverAddr.at(key);
}

void Store::setReadLocation(uint32_t position, uint8_t offset)
{
  uint8_t next = offset == 1 ? 0 : 1;
  _fram->writeEnable(true);
  _fram->write8(position, next);
  _fram->writeEnable(false);
}

uint8_t Store::readUint(uint8_t graver, FramKey key)
{
  return readUint(graverOffset(graver, key));
}
uint8_t Store::readUint(FramKey key)
{
  return readUint(FramAddr.at(key));
}
uint8_t Store::readUint(uint32_t position)
{
  return _fram->read8(readOffset(position, 1) + position);
}

uint16_t Store::readUint16(uint8_t graver, FramKey key)
{
  return readUint16(graverOffset(graver, key));
}
uint16_t Store::readUint16(FramKey key)
{
  return readUint16(FramAddr.at(key));
}
uint16_t Store::readUint16(uint32_t position)
{
  const uint32_t pos = readOffset(position, 2) + position;

  uint16_t out;
  uint8_t *outPtr = (uint8_t *)&out;
  *outPtr = _fram->read8(pos);
  outPtr++;
  *outPtr = _fram->read8(pos + 1);
  return out;
}

void Store::readChars(uint8_t graver, FramKey key, char *result, uint8_t count)
{
  readChars(graverOffset(graver, key), result, count);
}
void Store::readChars(FramKey key, char *result, uint8_t count)
{
  readChars(FramAddr.at(key), result, count);
}
void Store::readChars(uint32_t position, char *result, uint8_t count)
{
  const uint32_t pos = readOffset(position, count) + position;

  for (uint8_t i = 0; i < count; i++) {
    result[i] = _fram->read8(pos + i);
  }
}

void Store::writeUint(uint8_t graver, FramKey key, uint8_t value)
{
  writeUint(graverOffset(graver, key), value);
}
void Store::writeUint(FramKey key, uint8_t value)
{
  writeUint(FramAddr.at(key), value);
}
void Store::writeUint(uint32_t position, uint8_t value)
{
  const uint8_t offset = nextOffset(position, 1);

  _fram->writeEnable(true);
  _fram->write8(position + offset, value);
  _fram->writeEnable(false);
  setReadLocation(position, offset);
}

void Store::writeUint16(uint8_t graver, FramKey key, uint16_t value)
{
  writeUint16(graverOffset(graver, key), value);
}
void Store::writeUint16(FramKey key, uint16_t value)
{
  writeUint16(FramAddr.at(key), value);
}
void Store::writeUint16(uint32_t position, uint16_t value)
{
  const uint8_t offset = nextOffset(position, 2);

  _fram->writeEnable(true);
  _fram->write(position + offset, (uint8_t *)&value, sizeof(uint16_t));
  _fram->writeEnable(false);

  setReadLocation(position, offset);
}

void Store::writeChars(uint8_t graver, FramKey key, const char *value, uint8_t count)
{
  writeChars(graverOffset(graver, key), value, count);
}
void Store::writeChars(FramKey key, const char *value, uint8_t count)
{
  writeChars(FramAddr.at(key), value, count);
}
void Store::writeChars(uint32_t position, const char *value, uint8_t count)
{
  const uint8_t offset = nextOffset(position, count);
  _fram->writeEnable(true);
  for (uint8_t i = 0; i < count; i++) {
    _fram->write8(position + offset + i, value[i]);
  }
  _fram->writeEnable(false);
  setReadLocation(position, offset);
}

void Store::clear()
{
  uint8_t buff[8];
  std::fill(std::begin(buff), std::begin(buff) + 8, 0);
  _fram->writeEnable(true);
  for (uint32_t i = 0; i < FRAM_SIZE / 8; i++) {
    _fram->write(i * 8, (uint8_t *)&buff, 8);
  }
  _fram->writeEnable(false);
}
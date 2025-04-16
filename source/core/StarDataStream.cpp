#include "StarDataStream.hpp"
#include "StarBytes.hpp"
#include "StarVlqEncoding.hpp"

#include <string.h>

namespace Star {

unsigned const CurrentStreamVersion = 5; // update OpenProtocolVersion too!

DataStream::DataStream()
  : m_byteOrder(ByteOrder::BigEndian),
    m_nullTerminatedStrings(false),
    m_streamCompatibilityVersion(CurrentStreamVersion) {}

ByteOrder DataStream::byteOrder() const {
  return m_byteOrder;
}

void DataStream::setByteOrder(ByteOrder byteOrder) {
  m_byteOrder = byteOrder;
}

bool DataStream::nullTerminatedStrings() const {
  return m_nullTerminatedStrings;
}

void DataStream::setNullTerminatedStrings(bool nullTerminatedStrings) {
  m_nullTerminatedStrings = nullTerminatedStrings;
}

unsigned DataStream::streamCompatibilityVersion() const {
  return m_streamCompatibilityVersion;
}

void DataStream::setStreamCompatibilityVersion(unsigned streamCompatibilityVersion) {
  m_streamCompatibilityVersion = streamCompatibilityVersion;
}

void DataStream::setStreamCompatibilityVersion(NetCompatibilityRules const& rules) {
  m_streamCompatibilityVersion = rules.version();
}

ByteArray DataStream::readBytes(size_t len) {
  ByteArray ba;
  ba.resize(len);
  readData(ba.ptr(), len);
  return ba;
}

void DataStream::writeBytes(ByteArray const& ba) {
  writeData(ba.ptr(), ba.size());
}

DataStream& DataStream::operator<<(bool d) {
  operator<<((uint8_t)d);
  return *this;
}

DataStream& DataStream::operator<<(char c) {
  writeData(&c, 1);
  return *this;
}

DataStream& DataStream::operator<<(int8_t d) {
  writeData((char*)&d, sizeof(d));
  return *this;
}

DataStream& DataStream::operator<<(uint8_t d) {
  writeData((char*)&d, sizeof(d));
  return *this;
}

DataStream& DataStream::operator<<(int16_t d) {
  d = toByteOrder(m_byteOrder, d);
  writeData((char*)&d, sizeof(d));
  return *this;
}

DataStream& DataStream::operator<<(uint16_t d) {
  d = toByteOrder(m_byteOrder, d);
  writeData((char*)&d, sizeof(d));
  return *this;
}

DataStream& DataStream::operator<<(int32_t d) {
  d = toByteOrder(m_byteOrder, d);
  writeData((char*)&d, sizeof(d));
  return *this;
}

DataStream& DataStream::operator<<(uint32_t d) {
  d = toByteOrder(m_byteOrder, d);
  writeData((char*)&d, sizeof(d));
  return *this;
}

DataStream& DataStream::operator<<(int64_t d) {
  d = toByteOrder(m_byteOrder, d);
  writeData((char*)&d, sizeof(d));
  return *this;
}

DataStream& DataStream::operator<<(uint64_t d) {
  d = toByteOrder(m_byteOrder, d);
  writeData((char*)&d, sizeof(d));
  return *this;
}

DataStream& DataStream::operator<<(float d) {
  d = toByteOrder(m_byteOrder, d);
  writeData((char*)&d, sizeof(d));
  return *this;
}

DataStream& DataStream::operator<<(double d) {
  d = toByteOrder(m_byteOrder, d);
  writeData((char*)&d, sizeof(d));
  return *this;
}

DataStream& DataStream::operator>>(bool& d) {
  uint8_t bu;
  readData((char*)&bu, sizeof(bu));
  d = (bool)bu;
  return *this;
}

DataStream& DataStream::operator>>(char& c) {
  readData(&c, 1);
  return *this;
}

DataStream& DataStream::operator>>(int8_t& d) {
  readData((char*)&d, sizeof(d));
  return *this;
}

DataStream& DataStream::operator>>(uint8_t& d) {
  readData((char*)&d, sizeof(d));
  return *this;
}

DataStream& DataStream::operator>>(int16_t& d) {
  readData((char*)&d, sizeof(d));
  d = fromByteOrder(m_byteOrder, d);
  return *this;
}

DataStream& DataStream::operator>>(uint16_t& d) {
  readData((char*)&d, sizeof(d));
  d = fromByteOrder(m_byteOrder, d);
  return *this;
}

DataStream& DataStream::operator>>(int32_t& d) {
  readData((char*)&d, sizeof(d));
  d = fromByteOrder(m_byteOrder, d);
  return *this;
}

DataStream& DataStream::operator>>(uint32_t& d) {
  readData((char*)&d, sizeof(d));
  d = fromByteOrder(m_byteOrder, d);
  return *this;
}

DataStream& DataStream::operator>>(int64_t& d) {
  readData((char*)&d, sizeof(d));
  d = fromByteOrder(m_byteOrder, d);
  return *this;
}

DataStream& DataStream::operator>>(uint64_t& d) {
  readData((char*)&d, sizeof(d));
  d = fromByteOrder(m_byteOrder, d);
  return *this;
}

DataStream& DataStream::operator>>(float& d) {
  readData((char*)&d, sizeof(d));
  d = fromByteOrder(m_byteOrder, d);
  return *this;
}

DataStream& DataStream::operator>>(double& d) {
  readData((char*)&d, sizeof(d));
  d = fromByteOrder(m_byteOrder, d);
  return *this;
}

size_t DataStream::writeVlqU(uint64_t i) {
  return Star::writeVlqU(i, makeFunctionOutputIterator([this](uint8_t b) { *this << b; }));
}

size_t DataStream::writeVlqI(int64_t i) {
  return Star::writeVlqI(i, makeFunctionOutputIterator([this](uint8_t b) { *this << b; }));
}

size_t DataStream::writeVlqS(size_t i) {
  uint64_t i64;
  if (i == NPos)
    i64 = 0;
  else
    i64 = i + 1;
  return writeVlqU(i64);
}

size_t DataStream::readVlqU(uint64_t& i) {
  size_t bytesRead = Star::readVlqU(i, makeFunctionInputIterator([this]() { return this->read<uint8_t>(); }));

  if (bytesRead == NPos)
    throw DataStreamException("Error reading VLQ encoded integer!");

  return bytesRead;
}

size_t DataStream::readVlqI(int64_t& i) {
  size_t bytesRead = Star::readVlqI(i, makeFunctionInputIterator([this]() { return this->read<uint8_t>(); }));

  if (bytesRead == NPos)
    throw DataStreamException("Error reading VLQ encoded integer!");

  return bytesRead;
}

size_t DataStream::readVlqS(size_t& i) {
  uint64_t i64;
  size_t res = readVlqU(i64);
  if (i64 == 0)
    i = NPos;
  else
    i = (size_t)(i64 - 1);
  return res;
}

uint64_t DataStream::readVlqU() {
  uint64_t i;
  readVlqU(i);
  return i;
}

int64_t DataStream::readVlqI() {
  int64_t i;
  readVlqI(i);
  return i;
}

size_t DataStream::readVlqS() {
  size_t i;
  readVlqS(i);
  return i;
}

DataStream& DataStream::operator<<(char const* s) {
  writeStringData(s, strlen(s));
  return *this;
}

DataStream& DataStream::operator<<(std::string const& d) {
  writeStringData(d.c_str(), d.size());
  return *this;
}

DataStream& DataStream::operator<<(const ByteArray& d) {
  writeVlqU(d.size());
  writeData(d.ptr(), d.size());
  return *this;
}

DataStream& DataStream::operator<<(const String& s) {
  writeStringData(s.utf8Ptr(), s.utf8Size());
  return *this;
}

DataStream& DataStream::operator>>(std::string& d) {
  if (m_nullTerminatedStrings) {
    d.clear();
    char c;
    while (true) {
      readData((char*)&c, sizeof(c));
      if (c == '\0')
        break;
      d.push_back(c);
    }
  } else {
    d.resize((size_t)readVlqU());
    readData(&d[0], d.size());
  }
  return *this;
}

DataStream& DataStream::operator>>(ByteArray& d) {
  d.resize((size_t)readVlqU());
  readData(d.ptr(), d.size());
  return *this;
}

DataStream& DataStream::operator>>(String& s) {
  std::string string;
  operator>>(string);
  s = std::move(string);
  return *this;
}

void DataStream::writeStringData(char const* data, size_t len) {
  if (m_nullTerminatedStrings) {
    writeData(data, len);
    operator<<((uint8_t)0x00);
  } else {
    writeVlqU(len);
    writeData(data, len);
  }
}

}

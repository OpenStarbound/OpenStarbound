#pragma once

#include "StarIODevice.hpp"
#include "StarString.hpp"

namespace Star {

STAR_CLASS(Buffer);
STAR_CLASS(ExternalBuffer);

// Wraps a ByteArray to an IODevice
class Buffer : public IODevice {
public:
  // Constructs buffer open ReadWrite
  Buffer();
  Buffer(size_t initialSize);
  Buffer(ByteArray b);
  Buffer(Buffer const& buffer);
  Buffer(Buffer&& buffer);

  StreamOffset pos() override;
  void seek(StreamOffset pos, IOSeek mode = IOSeek::Absolute) override;
  void resize(StreamOffset size) override;
  bool atEnd() override;

  size_t read(char* data, size_t len) override;
  size_t write(char const* data, size_t len) override;

  size_t readAbsolute(StreamOffset readPosition, char* data, size_t len) override;
  size_t writeAbsolute(StreamOffset writePosition, char const* data, size_t len) override;

  void open(IOMode mode) override;

  String deviceName() const override;

  StreamOffset size() override;

  ByteArray& data();
  ByteArray const& data() const;

  // If this class holds the underlying data, then this method is cheap, and
  // will move the data out of this class into the returned array, otherwise,
  // this will incur a copy.  Afterwards, this Buffer will be left empty.
  ByteArray takeData();

  // Returns a pointer to the beginning of the Buffer.
  char* ptr();
  char const* ptr() const;

  // Same thing as size(), just size_t type (since this is in-memory)
  size_t dataSize() const;
  void reserve(size_t size);

  // Clears buffer, moves position to 0.
  void clear();
  bool empty() const;

  // Reset buffer with new contents, moves position to 0.
  void reset(size_t newSize);
  void reset(ByteArray b);

  Buffer& operator=(Buffer const& buffer);
  Buffer& operator=(Buffer&& buffer);

private:
  size_t doRead(size_t pos, char* data, size_t len);
  size_t doWrite(size_t pos, char const* data, size_t len);

  size_t m_pos;
  ByteArray m_bytes;
};

// Wraps an externally held sequence of bytes to a read-only IODevice
class ExternalBuffer : public IODevice {
public:
  // Constructs an empty ReadOnly ExternalBuffer.
  ExternalBuffer();
  // Constructs a ReadOnly ExternalBuffer pointing to the given external data, which
  // must be valid for the lifetime of the ExternalBuffer.
  ExternalBuffer(char const* externalData, size_t len);

  ExternalBuffer(ExternalBuffer const& buffer) = default;
  ExternalBuffer& operator=(ExternalBuffer const& buffer) = default;

  StreamOffset pos() override;
  void seek(StreamOffset pos, IOSeek mode = IOSeek::Absolute) override;
  bool atEnd() override;

  size_t read(char* data, size_t len) override;
  size_t write(char const* data, size_t len) override;

  size_t readAbsolute(StreamOffset readPosition, char* data, size_t len) override;
  size_t writeAbsolute(StreamOffset writePosition, char const* data, size_t len) override;

  String deviceName() const override;

  StreamOffset size() override;

  // Returns a pointer to the beginning of the Buffer.
  char const* ptr() const;

  // Same thing as size(), just size_t type (since this is in-memory)
  size_t dataSize() const;

  // Clears buffer, moves position to 0.
  bool empty() const;

  operator bool() const;

  // Reset buffer with new contents, moves position to 0.
  void reset(char const* externalData, size_t len);

private:
  size_t doRead(size_t pos, char* data, size_t len);

  size_t m_pos;
  char const* m_bytes;
  size_t m_size;
};

}

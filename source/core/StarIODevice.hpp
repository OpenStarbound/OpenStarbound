#ifndef STAR_IO_DEVICE_H
#define STAR_IO_DEVICE_H

#include "StarByteArray.hpp"
#include "StarString.hpp"

namespace Star {

STAR_CLASS(IODevice);

STAR_EXCEPTION(EofException, IOException);

enum class IOMode : uint8_t {
  Closed = 0x0,
  Read = 0x1,
  Write = 0x2,
  ReadWrite = 0x3,
  Append = 0x4,
  Truncate = 0x8,
};

IOMode operator|(IOMode a, IOMode b);
bool operator&(IOMode a, IOMode b);

// Should match SEEK_SET, SEEK_CUR, AND SEEK_END
enum IOSeek : uint8_t {
  Absolute = 0,
  Relative = 1,
  End = 2
};

// Abstract Interface to a random access I/O device.
class IODevice {
public:
  IODevice(IOMode mode = IOMode::Closed);
  virtual ~IODevice();

  // Do a read or write that may result in less data read or written than
  // requested.
  virtual size_t read(char* data, size_t len) = 0;
  virtual size_t write(char const* data, size_t len) = 0;

  virtual StreamOffset pos() = 0;
  virtual void seek(StreamOffset pos, IOSeek mode = IOSeek::Absolute) = 0;

  // Default implementation throws unsupported exception.
  virtual void resize(StreamOffset size);

  // Read / write from an absolute offset in the file without modifying the
  // current file position.  Default implementation stores the file position,
  // then seeks and calls read/write partial, then restores the file position,
  // and is not thread safe.
  virtual size_t readAbsolute(StreamOffset readPosition, char* data, size_t len);
  virtual size_t writeAbsolute(StreamOffset writePosition, char const* data, size_t len);

  // Read and write fully, and throw an exception in every other case.  The
  // default implementations here will call the normal read or write, and if
  // the full amount is not read will throw an exception.
  virtual void readFull(char* data, size_t len);
  virtual void writeFull(char const* data, size_t len);
  virtual void readFullAbsolute(StreamOffset readPosition, char* data, size_t len);
  virtual void writeFullAbsolute(StreamOffset writePosition, char const* data, size_t len);

  // Default implementation throws exception if opening in a different mode
  // than the current mode.
  virtual void open(IOMode mode);

  // Default implementation sets mode equal to Closed
  virtual void close();

  // Default implementation is a no-op
  virtual void sync();

  // Default implementation just prints address of generic IODevice
  virtual String deviceName() const;

  // Is the file position at the end of the file and there is no more to read?
  // This is not the same as feof, which returns true after an unsuccesful read
  // past the end, it should return true after succesfully reading the final
  // byte.  Default implementation returns pos() >= size();
  virtual bool atEnd();

  // Default is to store position, seek end, then restore position.
  virtual StreamOffset size();

  IOMode mode() const;
  bool isOpen() const;
  bool isReadable() const;
  bool isWritable() const;

  ByteArray readBytes(size_t size);
  void writeBytes(ByteArray const& p);

  ByteArray readBytesAbsolute(StreamOffset readPosition, size_t size);
  void writeBytesAbsolute(StreamOffset writePosition, ByteArray const& p);

protected:
  void setMode(IOMode mode);

  IODevice(IODevice const&);
  IODevice& operator=(IODevice const&);

private:
  atomic<IOMode> m_mode;
};

inline IOMode operator|(IOMode a, IOMode b) {
  return (IOMode)((uint8_t)a | (uint8_t)b);
}

inline bool operator&(IOMode a, IOMode b) {
  return (uint8_t)a & (uint8_t)b;
}

inline IOMode IODevice::mode() const {
  return m_mode;
}

inline bool IODevice::isOpen() const {
  return m_mode != IOMode::Closed;
}

inline bool IODevice::isReadable() const {
  return m_mode & IOMode::Read;
}

inline bool IODevice::isWritable() const {
  return m_mode & IOMode::Write;
}

}

#endif

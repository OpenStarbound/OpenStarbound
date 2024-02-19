#include "StarBuffer.hpp"
#include "StarMathCommon.hpp"
#include "StarIODevice.hpp"
#include "StarFormat.hpp"

namespace Star {

Buffer::Buffer()
  : m_pos(0) {
  setMode(IOMode::ReadWrite);
}

Buffer::Buffer(size_t initialSize)
  : Buffer() {
  reset(initialSize);
}

Buffer::Buffer(ByteArray b)
  : Buffer() {
  reset(std::move(b));
}

Buffer::Buffer(Buffer const& buffer)
  : Buffer() {
  operator=(buffer);
}

Buffer::Buffer(Buffer&& buffer)
  : Buffer() {
  operator=(std::move(buffer));
}

StreamOffset Buffer::pos() {
  return m_pos;
}

void Buffer::seek(StreamOffset pos, IOSeek mode) {
  StreamOffset newPos = m_pos;
  if (mode == IOSeek::Absolute)
    newPos = pos;
  else if (mode == IOSeek::Relative)
    newPos += pos;
  else if (mode == IOSeek::End)
    newPos = m_bytes.size() - pos;
  m_pos = newPos;
}

void Buffer::resize(StreamOffset size) {
  data().resize((size_t)size);
}

bool Buffer::atEnd() {
  return m_pos >= m_bytes.size();
}

size_t Buffer::read(char* data, size_t len) {
  size_t l = doRead(m_pos, data, len);
  m_pos += l;
  return l;
}

size_t Buffer::write(char const* data, size_t len) {
  size_t l = doWrite(m_pos, data, len);
  m_pos += l;
  return l;
}

size_t Buffer::readAbsolute(StreamOffset readPosition, char* data, size_t len) {
  size_t rpos = readPosition;
  if ((StreamOffset)rpos != readPosition)
    throw IOException("Error, readPosition out of range");

  return doRead(rpos, data, len);
}

size_t Buffer::writeAbsolute(StreamOffset writePosition, char const* data, size_t len) {
  size_t wpos = writePosition;
  if ((StreamOffset)wpos != writePosition)
    throw IOException("Error, writePosition out of range");

  return doWrite(wpos, data, len);
}

void Buffer::open(IOMode mode) {
  setMode(mode);
  if (mode & IOMode::Write && mode & IOMode::Truncate)
    resize(0);
  if (mode & IOMode::Append)
    seek(0, IOSeek::End);
}

String Buffer::deviceName() const {
  return strf("Buffer <{}>", (void*)this);
}

StreamOffset Buffer::size() {
  return m_bytes.size();
}

ByteArray& Buffer::data() {
  return m_bytes;
}

ByteArray const& Buffer::data() const {
  return m_bytes;
}

ByteArray Buffer::takeData() {
  ByteArray ret = std::move(m_bytes);
  reset(0);
  return ret;
}

char* Buffer::ptr() {
  return data().ptr();
}

char const* Buffer::ptr() const {
  return m_bytes.ptr();
}

size_t Buffer::dataSize() const {
  return m_bytes.size();
}

void Buffer::reserve(size_t size) {
  data().reserve(size);
}

void Buffer::clear() {
  m_pos = 0;
  m_bytes.clear();
}

bool Buffer::empty() const {
  return m_bytes.empty();
}

void Buffer::reset(size_t newSize) {
  m_pos = 0;
  m_bytes.fill(newSize, 0);
}

void Buffer::reset(ByteArray b) {
  m_pos = 0;
  m_bytes = std::move(b);
}

Buffer& Buffer::operator=(Buffer const& buffer) {
  IODevice::operator=(buffer);
  m_pos = buffer.m_pos;
  m_bytes = buffer.m_bytes;
  return *this;
}

Buffer& Buffer::operator=(Buffer&& buffer) {
  IODevice::operator=(buffer);
  m_pos = buffer.m_pos;
  m_bytes = std::move(buffer.m_bytes);

  buffer.m_pos = 0;
  buffer.m_bytes = ByteArray();

  return *this;
}

size_t Buffer::doRead(size_t pos, char* data, size_t len) {
  if (len == 0)
    return 0;

  if (!isReadable())
    throw IOException("Error, read called on non-readable Buffer");

  if (pos >= m_bytes.size())
    return 0;

  size_t l = min(m_bytes.size() - pos, len);
  memcpy(data, m_bytes.ptr() + pos, l);
  return l;
}

size_t Buffer::doWrite(size_t pos, char const* data, size_t len) {
  if (len == 0)
    return 0;

  if (!isWritable())
    throw EofException("Error, write called on non-writable Buffer");

  if (pos + len > m_bytes.size())
    m_bytes.resize(pos + len);

  memcpy(m_bytes.ptr() + pos, data, len);
  return len;
}

ExternalBuffer::ExternalBuffer()
  : m_pos(0), m_bytes(nullptr), m_size(0) {
  setMode(IOMode::Read);
}

ExternalBuffer::ExternalBuffer(char const* externalData, size_t len) : ExternalBuffer() {
  reset(externalData, len);
}

StreamOffset ExternalBuffer::pos() {
  return m_pos;
}

void ExternalBuffer::seek(StreamOffset pos, IOSeek mode) {
  StreamOffset newPos = m_pos;
  if (mode == IOSeek::Absolute)
    newPos = pos;
  else if (mode == IOSeek::Relative)
    newPos += pos;
  else if (mode == IOSeek::End)
    newPos = m_size - pos;
  m_pos = newPos;
}

bool ExternalBuffer::atEnd() {
  return m_pos >= m_size;
}

size_t ExternalBuffer::read(char* data, size_t len) {
  size_t l = doRead(m_pos, data, len);
  m_pos += l;
  return l;
}

size_t ExternalBuffer::write(char const*, size_t) {
  throw IOException("Error, ExternalBuffer is not writable");
}

size_t ExternalBuffer::readAbsolute(StreamOffset readPosition, char* data, size_t len) {
  size_t rpos = readPosition;
  if ((StreamOffset)rpos != readPosition)
    throw IOException("Error, readPosition out of range");

  return doRead(rpos, data, len);
}

size_t ExternalBuffer::writeAbsolute(StreamOffset, char const*, size_t) {
  throw IOException("Error, ExternalBuffer is not writable");
}

String ExternalBuffer::deviceName() const {
  return strf("ExternalBuffer <{}>", (void*)this);
}

StreamOffset ExternalBuffer::size() {
  return m_size;
}

char const* ExternalBuffer::ptr() const {
  return m_bytes;
}

size_t ExternalBuffer::dataSize() const {
  return m_size;
}

bool ExternalBuffer::empty() const {
  return m_size == 0;
}

void ExternalBuffer::reset(char const* externalData, size_t len) {
  m_pos = 0;
  m_bytes = externalData;
  m_size = len;
}

size_t ExternalBuffer::doRead(size_t pos, char* data, size_t len) {
  if (len == 0)
    return 0;

  if (!isReadable())
    throw IOException("Error, read called on non-readable Buffer");

  if (pos >= m_size)
    return 0;

  size_t l = min(m_size - pos, len);
  memcpy(data, m_bytes + pos, l);
  return l;
}

}

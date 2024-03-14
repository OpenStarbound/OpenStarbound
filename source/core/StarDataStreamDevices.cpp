#include "StarDataStreamDevices.hpp"

namespace Star {

DataStreamFunctions::DataStreamFunctions(function<size_t(char*, size_t)> reader, function<size_t(char const*, size_t)> writer)
  : m_reader(std::move(reader)), m_writer(std::move(writer)) {}

void DataStreamFunctions::readData(char* data, size_t len) {
  if (!m_reader)
    throw DataStreamException("DataStreamFunctions no read function given");
  m_reader(data, len);
}

void DataStreamFunctions::writeData(char const* data, size_t len) {
  if (!m_writer)
    throw DataStreamException("DataStreamFunctions no write function given");
  m_writer(data, len);
}

DataStreamIODevice::DataStreamIODevice(IODevicePtr device)
  : m_device(std::move(device)) {}

IODevicePtr const& DataStreamIODevice::device() const {
  return m_device;
}

void DataStreamIODevice::seek(size_t pos, IOSeek mode) {
  m_device->seek(pos, mode);
}

bool DataStreamIODevice::atEnd() {
  return m_device->atEnd();
}

StreamOffset DataStreamIODevice::pos() {
  return m_device->pos();
}

void DataStreamIODevice::readData(char* data, size_t len) {
  return m_device->readFull(data, len);
}

void DataStreamIODevice::writeData(char const* data, size_t len) {
  return m_device->writeFull(data, len);
}

DataStreamBuffer::DataStreamBuffer() {
  m_buffer = make_shared<Buffer>();
}

DataStreamBuffer::DataStreamBuffer(size_t s)
  : DataStreamBuffer() {
  reset(s);
}

DataStreamBuffer::DataStreamBuffer(ByteArray b)
  : DataStreamBuffer() {
  reset(std::move(b));
}

void DataStreamBuffer::resize(size_t size) {
  m_buffer->resize(size);
}

void DataStreamBuffer::reserve(size_t size) {
  m_buffer->reserve(size);
}

void DataStreamBuffer::clear() {
  m_buffer->clear();
}

BufferPtr const& DataStreamBuffer::device() const {
  return m_buffer;
}

ByteArray& DataStreamBuffer::data() {
  return m_buffer->data();
}

ByteArray const& DataStreamBuffer::data() const {
  return m_buffer->data();
}

ByteArray DataStreamBuffer::takeData() {
  return m_buffer->takeData();
}

char* DataStreamBuffer::ptr() {
  return m_buffer->ptr();
}

const char* DataStreamBuffer::ptr() const {
  return m_buffer->ptr();
}

size_t DataStreamBuffer::size() const {
  return m_buffer->dataSize();
}

bool DataStreamBuffer::empty() const {
  return m_buffer->empty();
}

void DataStreamBuffer::seek(size_t pos, IOSeek mode) {
  m_buffer->seek(pos, mode);
}

bool DataStreamBuffer::atEnd() {
  return m_buffer->atEnd();
}

size_t DataStreamBuffer::pos() {
  return (size_t)m_buffer->pos();
}

void DataStreamBuffer::reset(size_t newSize) {
  m_buffer->reset(newSize);
}

void DataStreamBuffer::reset(ByteArray b) {
  m_buffer->reset(std::move(b));
}

void DataStreamBuffer::readData(char* data, size_t len) {
  m_buffer->readFull(data, len);
}

void DataStreamBuffer::writeData(char const* data, size_t len) {
  m_buffer->writeFull(data, len);
}

DataStreamExternalBuffer::DataStreamExternalBuffer() : m_buffer() {}

DataStreamExternalBuffer::DataStreamExternalBuffer(DataStreamBuffer const& buffer) : DataStreamExternalBuffer(buffer.ptr(), buffer.size()) {}

DataStreamExternalBuffer::DataStreamExternalBuffer(char const* externalData, size_t len) : DataStreamExternalBuffer() {
  reset(externalData, len);
}

char const* DataStreamExternalBuffer::ptr() const {
  return m_buffer.ptr();
}

size_t DataStreamExternalBuffer::size() const {
  return m_buffer.dataSize();
}

bool DataStreamExternalBuffer::empty() const {
  return m_buffer.empty();
}

void DataStreamExternalBuffer::seek(size_t pos, IOSeek mode) {
  m_buffer.seek(pos, mode);
}

bool DataStreamExternalBuffer::atEnd() {
  return m_buffer.atEnd();
}

size_t DataStreamExternalBuffer::pos() {
  return m_buffer.pos();
}

size_t DataStreamExternalBuffer::remaining() {
  return m_buffer.dataSize() - m_buffer.pos();
}

void DataStreamExternalBuffer::reset(char const* externalData, size_t len) {
  m_buffer.reset(externalData, len);
}

void DataStreamExternalBuffer::readData(char* data, size_t len) {
  m_buffer.readFull(data, len);
}
void DataStreamExternalBuffer::writeData(char const* data, size_t len) {
  m_buffer.writeFull(data, len);
}

}

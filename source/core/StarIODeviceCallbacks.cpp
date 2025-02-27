#include "StarIODeviceCallbacks.hpp"
#include "vorbis/vorbisfile.h"

namespace Star {

IODeviceCallbacks::IODeviceCallbacks(IODevicePtr device) 
  : m_device(std::move(device)) {
  if (!m_device->isOpen())
    m_device->open(IOMode::Read);
}

IODevicePtr const& IODeviceCallbacks::device() const {
  return m_device;
}

size_t IODeviceCallbacks::readFunc(void* ptr, size_t size, size_t nmemb, void* datasource) {
  auto* callbacks = static_cast<IODeviceCallbacks*>(datasource);
  return callbacks->m_device->read((char*)ptr, size * nmemb) / size;
}

int IODeviceCallbacks::seekFunc(void* datasource, ogg_int64_t offset, int whence) {
  auto* callbacks = static_cast<IODeviceCallbacks*>(datasource);
  callbacks->m_device->seek(offset, (IOSeek)whence);
  return 0;
}

long int IODeviceCallbacks::tellFunc(void* datasource) {
  auto* callbacks = static_cast<IODeviceCallbacks*>(datasource);
  return (long int)callbacks->m_device->pos();
}

void IODeviceCallbacks::setupOggCallbacks(ov_callbacks& callbacks) {
  callbacks.read_func = readFunc;
  callbacks.seek_func = seekFunc;
  callbacks.tell_func = tellFunc;
  callbacks.close_func = nullptr;
}

}
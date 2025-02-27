#pragma once

#include "StarIODevice.hpp"
#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"

namespace Star {

// Provides callbacks for interfacing IODevice with ogg vorbis callbacks
class IODeviceCallbacks {
public:
  explicit IODeviceCallbacks(IODevicePtr device);
  
  // No copying
  IODeviceCallbacks(IODeviceCallbacks const&) = delete;
  IODeviceCallbacks& operator=(IODeviceCallbacks const&) = delete;
  
  // Moving is ok
  IODeviceCallbacks(IODeviceCallbacks&&) = default;
  IODeviceCallbacks& operator=(IODeviceCallbacks&&) = default;

  // Get the underlying device
  IODevicePtr const& device() const;
  
  // Callback functions for Ogg Vorbis
  static size_t readFunc(void* ptr, size_t size, size_t nmemb, void* datasource);
  static int seekFunc(void* datasource, ogg_int64_t offset, int whence);
  static long int tellFunc(void* datasource);
  
  // Sets up callbacks for Ogg Vorbis
  void setupOggCallbacks(ov_callbacks& callbacks);

private:
  IODevicePtr m_device;
};

}

#include "StarDynamicLib.hpp"

#include <dlfcn.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>

namespace Star {

struct PrivateDynLib : public DynamicLib {
  PrivateDynLib(void* handle)
    : m_handle(handle) {}

  ~PrivateDynLib() {
    dlclose(m_handle);
  }

  void* funcPtr(const char* name) {
    return dlsym(m_handle, name);
  }

  void* m_handle;
};

String DynamicLib::libraryExtension() {
#ifdef STAR_SYSTEM_MACOS
  return ".dylib";
#else
  return ".so";
#endif
}

DynamicLibUPtr DynamicLib::loadLibrary(String const& libraryName) {
  void* handle = dlopen(libraryName.utf8Ptr(), RTLD_NOW);
  if (handle == NULL)
    return {};
  return make_unique<PrivateDynLib>(handle);
}

DynamicLibUPtr DynamicLib::currentExecutable() {
  void* handle = dlopen(NULL, 0);
  starAssert(handle);
  return make_unique<PrivateDynLib>(handle);
}

}

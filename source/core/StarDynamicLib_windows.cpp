#include "StarDynamicLib.hpp"
#include "StarFormat.hpp"
#include "StarString_windows.hpp"

#include <windows.h>

namespace Star {

class PrivateDynLib : public DynamicLib {
public:
  PrivateDynLib(void* handle)
    : m_handle(handle) {}

  ~PrivateDynLib() {
    FreeLibrary((HMODULE)m_handle);
  }

  void* funcPtr(const char* name) {
    return (void*)GetProcAddress((HMODULE)m_handle, name);
  }

private:
  void* m_handle;
};

String DynamicLib::libraryExtension() {
  return ".dll";
}

DynamicLibUPtr DynamicLib::loadLibrary(String const& libraryName) {
  void* handle = LoadLibraryW(stringToUtf16(libraryName).get());
  if (handle == NULL)
    return {};
  return make_unique<PrivateDynLib>(handle);
}

DynamicLibUPtr DynamicLib::currentExecutable() {
  void* handle = GetModuleHandle(0);
  starAssert(handle);
  return make_unique<PrivateDynLib>(handle);
}

}

#pragma once

#include "StarString.hpp"

namespace Star {

STAR_CLASS(DynamicLib);

class DynamicLib {
public:
  // Returns the library extension normally used on the current platform
  // including the '.', e.g.  '.dll', '.so', '.dylib'
  static String libraryExtension();

  // Load a dll from the given filename.  If the library is found and
  // succesfully loaded, returns a handle to the library, otherwise nullptr.
  static DynamicLibUPtr loadLibrary(String const& fileName);

  // Load a dll from the given name, minus extension.
  static DynamicLibUPtr loadLibraryBase(String const& baseName);

  // Should return handle to currently running executable.  Will always
  // succeed.
  static DynamicLibUPtr currentExecutable();

  virtual ~DynamicLib() = default;

  virtual void* funcPtr(char const* name) = 0;
};

inline DynamicLibUPtr DynamicLib::loadLibraryBase(String const& baseName) {
  return loadLibrary(baseName + libraryExtension());
}

}

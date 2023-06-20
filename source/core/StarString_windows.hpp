#ifndef STAR_STRING_WINDOWS_HPP
#define STAR_STRING_WINDOWS_HPP

#include <windows.h>

#include "StarString.hpp"

namespace Star {

String utf16ToString(WCHAR const* s);
unique_ptr<WCHAR[]> stringToUtf16(String const& s);

}

#endif

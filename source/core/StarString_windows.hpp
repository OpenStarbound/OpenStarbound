#pragma once

#include <windows.h>

#include "StarString.hpp"

namespace Star {

String utf16ToString(WCHAR const* s);
unique_ptr<WCHAR[]> stringToUtf16(String const& s);

}

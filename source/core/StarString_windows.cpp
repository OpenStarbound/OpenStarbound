#include "StarString_windows.hpp"

namespace Star {

size_t wcharLen(WCHAR const* s) {
  size_t size = 0;
  while (*s) {
    ++size;
    ++s;
  }
  return size;
}

String utf16ToString(WCHAR const* s) {
  if (!s)
    return "";
  int sLen = wcharLen(s);
  int utf8Len = WideCharToMultiByte(CP_UTF8, 0, s, sLen + 1, NULL, 0, NULL, NULL);
  auto utf8Buffer = new char[utf8Len];
  WideCharToMultiByte(CP_UTF8, 0, s, sLen + 1, utf8Buffer, utf8Len, NULL, NULL);
  auto result = String(utf8Buffer, utf8Len - 1);
  delete[] utf8Buffer;
  return result;
}

unique_ptr<WCHAR[]> stringToUtf16(String const& s) {
  int utf16Len = MultiByteToWideChar(CP_UTF8, 0, s.utf8Ptr(), s.utf8Size() + 1, NULL, 0);
  unique_ptr<WCHAR[]> result;
  result.reset(new WCHAR[utf16Len]);
  MultiByteToWideChar(CP_UTF8, 0, s.utf8Ptr(), s.utf8Size() + 1, result.get(), utf16Len);
  return result;
}

}

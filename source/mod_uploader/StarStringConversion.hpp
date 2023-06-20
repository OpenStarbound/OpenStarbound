#ifndef STAR_STRING_CONVERSION
#define STAR_STRING_CONVERSION

#include <QString>

#include "StarString.hpp"

namespace Star {

inline String toSString(QString const& str) {
  return String(str.toUtf8().data());
}

inline QString toQString(String const& str) {
  return QString::fromUtf8(str.utf8Ptr(), str.utf8Size());
}

}

#endif

#include "StarUnicode.hpp"
#include "StarEncode.hpp"

namespace Star {

bool isValidUtf8(const Utf8Type* utf8, size_t remain) {
  bool stopOnNull = remain == NPos;

  while (true) {
    if (remain == 0)
      break;

    if (stopOnNull && utf8[0] == 0)
      break;

    if ((utf8[0] & 0x80) == 0x00) {
      // ASCII
      ++utf8;
      --remain;
      continue;
    }

    if (remain == 1) {
      // 1 invalid character with missing end
      return false;
    }

    if ((utf8[0] & 0xe0) == 0xc0 && (utf8[1] & 0xc0) == 0x80) {
      if (((utf8[0] & 0x1fL) << 6) >= 0x00000080L) {
        utf8 += 2;
        remain -= 2;
        continue;
      } else {
        return false;
      }
    }

    if (remain == 2) {
      // 2 invalid characters with missing end
      return false;
    }

    if ((utf8[0] & 0xf0) == 0xe0 && (utf8[1] & 0xc0) == 0x80 && (utf8[2] & 0xc0) == 0x80) {
      auto utf32char = ((utf8[0] & 0x0fL) << 12) | ((utf8[1] & 0x3fL) << 6);
      // ensure it's not overlong, and ensure it's not a UTF16 surrogate
      if (utf32char >= 0x00000800L && !isUtf16Surrogate(utf32char)) {
        utf8 += 3;
        remain -= 3;
        continue;
      } else {
        return false;
      }
    }

    if (remain == 3) {
      // 3 invalid characters with missing end
      return false;
    }

    if ((utf8[0] & 0xf8) == 0xf0 && (utf8[1] & 0xc0) == 0x80 && (utf8[2] & 0xc0) == 0x80 && (utf8[3] & 0xc0) == 0x80) {
      auto utf32char = ((utf8[0] & 0x07L) << 18) | ((utf8[1] & 0x3fL) << 12);
      if (utf32char >= 0x00010000L && utf32char <= 0x0010ffffL) {
        utf8 += 4;
        remain -= 4;
        continue;
      } else {
        return false;
      }
    } else {
      return false;
    }
  }

  return true;
}

size_t utf8Length(const Utf8Type* utf8, size_t remain) {
  bool stopOnNull = remain == NPos;
  size_t length = 0;

  while (true) {
    if (remain == 0)
      break;

    if (stopOnNull && utf8[0] == 0)
      break;

    if ((utf8[0] & 0x80) == 0x00) {
      ++length;
      ++utf8;
      --remain;
      continue;
    }

    if (remain == 1) {
      // 1 invalid character with missing end
      ++length;
      --remain;
      break;
    }

    if ((utf8[0] & 0xe0) == 0xc0 && (utf8[1] & 0xc0) == 0x80) {
      if (((utf8[0] & 0x1fL) << 6) >= 0x00000080L) {
        ++length;
        utf8 += 2;
        remain -= 2;
        continue;
      } else {
        // invalid, assume a single placeholder character
        ++length;
        ++utf8;
        --remain;
        continue;
      }
    }

    if (remain == 2) {
      // 2 invalid characters with missing end
      // only 1 trailing byte remains so the next character must be either ASCII or invalid
      length += 2;
      remain -= 2;
      break;
    }

    if ((utf8[0] & 0xf0) == 0xe0 && (utf8[1] & 0xc0) == 0x80 && (utf8[2] & 0xc0) == 0x80) {
      auto utf32char = ((utf8[0] & 0x0fL) << 12) | ((utf8[1] & 0x3fL) << 6);
      // ensure it's not overlong, and ensure it's not a UTF16 surrogate
      if (utf32char >= 0x00000800L && !isUtf16Surrogate(utf32char)) {
        ++length;
        utf8 += 3;
        remain -= 3;
        continue;
      } else {
        // invalid, assume a single placeholder character
        ++length;
        ++utf8;
        --remain;
        continue;
      }
    }

    if (remain == 3) {
      // 3 invalid characters with missing end
      // there is the possibility that there is a valid utf-8 sequence after this, so only advance once
      ++length;
      ++utf8;
      --remain;
      continue;
    }

    if ((utf8[0] & 0xf8) == 0xf0 && (utf8[1] & 0xc0) == 0x80 && (utf8[2] & 0xc0) == 0x80 && (utf8[3] & 0xc0) == 0x80) {
      auto utf32char = ((utf8[0] & 0x07L) << 18) | ((utf8[1] & 0x3fL) << 12);
      if (utf32char >= 0x00010000L && utf32char <= 0x0010ffffL) {
        ++length;
        utf8 += 4;
        remain -= 4;
        continue;
      } else {
        // invalid, assume a single placeholder character
        ++length;
        ++utf8;
        --remain;
        continue;
      }
    } else {
      // invalid, assume a single placeholder character
      ++length;
      ++utf8;
      --remain;
      continue;
    }
  }

  return length;
}

size_t utf8DecodeChar(const Utf8Type* utf8, Utf32Type* utf32, size_t remain) {
  const Utf8Type* start = utf8;
  bool stopOnNull = remain == NPos;

  while (true) {
    if (remain == 0)
      break;

    if (stopOnNull && utf8[0] == 0)
      break;

    if ((utf8[0] & 0x80) == 0x00) {
      *utf32 = utf8[0];
      return utf8 - start + 1;
    }

    if (remain == 1) {
      *utf32 = STAR_UTF32_REPLACEMENT_CHAR;
      return utf8 - start + 1;
    }

    if ((utf8[0] & 0xe0) == 0xc0 && (utf8[1] & 0xc0) == 0x80) {
      *utf32 = ((utf8[0] & 0x1fL) << 6) | ((utf8[1] & 0x3fL) << 0);
      if (*utf32 >= 0x00000080L) {
        return utf8 - start + 2;
      } else {
        *utf32 = STAR_UTF32_REPLACEMENT_CHAR;
        return utf8 - start + 1;
      }
    }

    if (remain == 2) {
      *utf32 = STAR_UTF32_REPLACEMENT_CHAR;
      return utf8 - start + 1;
    }

    if ((utf8[0] & 0xf0) == 0xe0 && (utf8[1] & 0xc0) == 0x80 && (utf8[2] & 0xc0) == 0x80) {
      *utf32 = ((utf8[0] & 0x0fL) << 12) | ((utf8[1] & 0x3fL) << 6) | ((utf8[2] & 0x3fL) << 0);
      if (*utf32 >= 0x00000800L && !isUtf16Surrogate(*utf32)) {
        return utf8 - start + 3;
      } else {
        *utf32 = STAR_UTF32_REPLACEMENT_CHAR;
        return utf8 - start + 1;
      }
    }

    if (remain == 3) {
      *utf32 = STAR_UTF32_REPLACEMENT_CHAR;
      return utf8 - start + 1;
    }

    if ((utf8[0] & 0xf8) == 0xf0 && (utf8[1] & 0xc0) == 0x80 && (utf8[2] & 0xc0) == 0x80 && (utf8[3] & 0xc0) == 0x80) {
      *utf32 =
        ((utf8[0] & 0x07L) << 18) | ((utf8[1] & 0x3fL) << 12) | ((utf8[2] & 0x3fL) << 6) | ((utf8[3] & 0x3fL) << 0);
      if (*utf32 >= 0x00010000L && *utf32 <= 0x0010ffffL) {
        return utf8 - start + 4;
      } else {
        *utf32 = STAR_UTF32_REPLACEMENT_CHAR;
        return utf8 - start + 1;
      }
    } else {
      *utf32 = STAR_UTF32_REPLACEMENT_CHAR;
      return utf8 - start + 1;
    }
  }

  return utf8 - start;
}

size_t utf8EncodeChar(Utf8Type* utf8, Utf32Type utf32, size_t len) {
  if (utf32 > 0x10FFFFu || isUtf16Surrogate(utf32))
    return utf8EncodeChar(utf8, STAR_UTF32_REPLACEMENT_CHAR, len);

  if (utf32 <= 0x0000007fL) {
    if (len < 1)
      return 0;

    utf8[0] = utf32;
    return 1;
  } else if (utf32 <= 0x000007ffL) {
    if (len < 2)
      return 0;

    utf8[0] = 0xc0 | ((utf32 >> 6) & 0x1f);
    utf8[1] = 0x80 | ((utf32 >> 0) & 0x3f);

    return 2;
  } else if (utf32 <= 0x0000ffffL) {
    if (len < 3)
      return 0;

    utf8[0] = 0xe0 | ((utf32 >> 12) & 0x0f);
    utf8[1] = 0x80 | ((utf32 >> 6) & 0x3f);
    utf8[2] = 0x80 | ((utf32 >> 0) & 0x3f);

    return 3;
  } else {
    if (len < 4)
      return 0;

    utf8[0] = 0xf0 | ((utf32 >> 18) & 0x07);
    utf8[1] = 0x80 | ((utf32 >> 12) & 0x3f);
    utf8[2] = 0x80 | ((utf32 >> 6) & 0x3f);
    utf8[3] = 0x80 | ((utf32 >> 0) & 0x3f);

    return 4;
  }
}

static const char32_t MIN_LEAD = 0xd800;
static const char32_t MAX_LEAD = 0xdbff;
static const char32_t MIN_TRAIL = 0xdc00;
static const char32_t MAX_TRAIL = 0xdfff;
static const char32_t SURR_MASK = 0x3ff;
static const char32_t MIN_PAIR = 0x10000;
static const char32_t MAX_CODEPOINT = 0x10ffff;

Utf32Type hexStringToUtf32(std::string const& codepoint, Maybe<Utf32Type> previousCodepoint) {
  bool continuation = false;
  if (previousCodepoint && isUtf16LeadSurrogate(*previousCodepoint)) {
    continuation = true;
  }

  auto hexBytes = hexDecode(codepoint);
  if (hexBytes.size() < sizeof(Utf32Type)) {
    ByteArray newHexBytes{(size_t)(sizeof(Utf32Type) - hexBytes.size()), (char)'\0'};
    newHexBytes.append(hexBytes);
    hexBytes = newHexBytes;
  }

  if (hexBytes.size() > sizeof(Utf32Type))
    throw UnicodeException("Codepoint size is too big in parseUnicodeCodepoint");

  auto res = fromBigEndian(*(Utf32Type*)hexBytes.ptr());

  if (continuation) {
    res = utf32FromUtf16SurrogatePair(*previousCodepoint, res);
  }

  return res;
}

std::string hexStringFromUtf32(Utf32Type character) {
  if (character > MAX_CODEPOINT)
    throw UnicodeException("Codepoint too big in hexStringFromUtf32");
  Utf32Type lead;
  Maybe<Utf32Type> trail;
  tie(lead, trail) = utf32ToUtf16SurrogatePair(character);

  char16_t leadOut = toBigEndian((char16_t)lead);
  auto leadHex = hexEncode(reinterpret_cast<char*>(&leadOut), sizeof(leadOut)).takeUtf8();

  starAssert(leadHex.size() == 4);

  if (!trail)
    return leadHex;

  char16_t trailOut = toBigEndian((char16_t)*trail);
  auto trailHex = hexEncode(reinterpret_cast<char*>(&trailOut), sizeof(trailOut));

  starAssert(trailHex.size() == 4);

  return (leadHex + trailHex).takeUtf8();
}

bool isUtf16LeadSurrogate(Utf32Type codepoint) {
  return codepoint >= MIN_LEAD && codepoint <= MAX_LEAD;
}

bool isUtf16TrailSurrogate(Utf32Type codepoint) {
  return codepoint >= MIN_TRAIL && codepoint <= MAX_TRAIL;
}

Utf32Type utf32FromUtf16SurrogatePair(Utf32Type lead, Utf32Type trail) {
  if (!isUtf16LeadSurrogate(lead))
    throw UnicodeException("Invalid lead surrogate passed to utf32FromUtf16SurrogatePair");
  if (!isUtf16TrailSurrogate(trail))
    throw UnicodeException("Invalid trail surrogate passed to utf32FromUtf16SurrogatePair");

  lead -= MIN_LEAD;
  trail -= MIN_TRAIL;

  Utf32Type codepoint = (lead << 10) + trail + MIN_PAIR;

  return codepoint;
}

pair<Utf32Type, Maybe<Utf32Type>> utf32ToUtf16SurrogatePair(Utf32Type codepoint) {
  if (codepoint >= MIN_PAIR) {
    codepoint -= MIN_PAIR;
    Utf32Type lead = (codepoint >> 10) + MIN_LEAD;
    Utf32Type trail = (codepoint & SURR_MASK) + MIN_TRAIL;

    if (!isUtf16LeadSurrogate(lead))
      throw UnicodeException("Invalid codepoint passed to utf32ToUtf16SurrogatePair");

    return {lead, trail};
  }

  return {codepoint, {}};
}

}// namespace Star

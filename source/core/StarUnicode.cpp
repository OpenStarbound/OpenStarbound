#include "StarUnicode.hpp"
#include "StarEncode.hpp"

namespace Star {

void throwInvalidUtf8Sequence() {
  throw UnicodeException("Invalid UTF-8 code unit sequence in utf8Length");
}

void throwMissingUtf8End() {
  throw UnicodeException("UTF-8 string missing trailing code units in utf8Length");
}

void throwInvalidUtf32CodePoint(Utf32Type val) {
  throw UnicodeException::format("Invalid UTF-32 code point {} encountered while trying to encode UTF-8", (int32_t)val);
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

    if (remain == 1)
      throwMissingUtf8End();

    if ((utf8[0] & 0xe0) == 0xc0 && (utf8[1] & 0xc0) == 0x80) {
      if (((utf8[0] & 0x1fL) << 6) >= 0x00000080L) {
        ++length;
        utf8 += 2;
        remain -= 2;
        continue;
      } else {
        throwInvalidUtf8Sequence();
      }
    }

    if (remain == 2)
      throwMissingUtf8End();

    if ((utf8[0] & 0xf0) == 0xe0 && (utf8[1] & 0xc0) == 0x80 && (utf8[2] & 0xc0) == 0x80) {
      if ((((utf8[0] & 0x0fL) << 12) | ((utf8[1] & 0x3fL) << 6)) >= 0x00000800L) {
        ++length;
        utf8 += 3;
        remain -= 3;
        continue;
      } else {
        throwInvalidUtf8Sequence();
      }
    }

    if (remain == 3)
      throwMissingUtf8End();

    if ((utf8[0] & 0xf8) == 0xf0 && (utf8[1] & 0xc0) == 0x80 && (utf8[2] & 0xc0) == 0x80 && (utf8[3] & 0xc0) == 0x80) {
      if ((((utf8[0] & 0x07L) << 18) | ((utf8[1] & 0x3fL) << 12)) >= 0x00010000L) {
        ++length;
        utf8 += 4;
        remain -= 4;
        continue;
      } else {
        throwInvalidUtf8Sequence();
      }
    } else {
      throwInvalidUtf8Sequence();
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

    if (remain == 1)
      throwMissingUtf8End();

    if ((utf8[0] & 0xe0) == 0xc0 && (utf8[1] & 0xc0) == 0x80) {
      *utf32 = ((utf8[0] & 0x1fL) << 6) | ((utf8[1] & 0x3fL) << 0);
      if (*utf32 >= 0x00000080L)
        return utf8 - start + 2;
      else
        throwInvalidUtf8Sequence();
    }

    if (remain == 2)
      throwMissingUtf8End();

    if ((utf8[0] & 0xf0) == 0xe0 && (utf8[1] & 0xc0) == 0x80 && (utf8[2] & 0xc0) == 0x80) {
      *utf32 = ((utf8[0] & 0x0fL) << 12) | ((utf8[1] & 0x3fL) << 6) | ((utf8[2] & 0x3fL) << 0);
      if (*utf32 >= 0x00000800L)
        return utf8 - start + 3;
      else
        throwInvalidUtf8Sequence();
    }

    if (remain == 3)
      throwMissingUtf8End();

    if ((utf8[0] & 0xf8) == 0xf0 && (utf8[1] & 0xc0) == 0x80 && (utf8[2] & 0xc0) == 0x80 && (utf8[3] & 0xc0) == 0x80) {
      *utf32 =
          ((utf8[0] & 0x07L) << 18) | ((utf8[1] & 0x3fL) << 12) | ((utf8[2] & 0x3fL) << 6) | ((utf8[3] & 0x3fL) << 0);
      if (*utf32 >= 0x00010000L)
        return utf8 - start + 4;
      else
        throwInvalidUtf8Sequence();
    } else {
      throwInvalidUtf8Sequence();
    }
  }

  return utf8 - start;
}

size_t utf8EncodeChar(Utf8Type* utf8, Utf32Type utf32, size_t len) {
  if (utf32 > 0x10FFFFu)
    throwInvalidUtf32CodePoint(utf32);

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

}

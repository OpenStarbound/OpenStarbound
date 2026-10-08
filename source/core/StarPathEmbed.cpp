#include "StarPathEmbed.hpp"

namespace Star {
    
EnumMap<EmbeddedCompressionMethod> const EmbeddedCompressionMethodNames {
  {EmbeddedCompressionMethod::None, "None"},
  {EmbeddedCompressionMethod::Gzip, "Gzip"},
  {EmbeddedCompressionMethod::Zlib, "Zlib"},
  {EmbeddedCompressionMethod::Zstd, "Zstd"}
};

String embedAndCompressData(ByteArray const& data, EmbeddedCompressionMethod const& compressionMethod) {
  switch (compressionMethod) {
    case EmbeddedCompressionMethod::None:
      return base64Encode(data,true);
    case EmbeddedCompressionMethod::Gzip:
      throw StarException("Compressing data in gzip is unsupported.");
    case EmbeddedCompressionMethod::Zlib:
      return "zlib;"+base64Encode(compressData(data),true);
    case EmbeddedCompressionMethod::Zstd:
      return "zstd;"+base64Encode(ZstdCompression::compress(data),true);
  }
}
String embedEscapeText(String const& data) {
  // swaps out a few characters for escaped versions.
  // meant to be fast. string is allocated for worst case scenario (entire string is made of escaped characters)
  typedef std::string::const_iterator const_iterator;
  auto out = std::string(data.utf8Size()*2,'\0');
  auto const& in = data.utf8();
  auto outIt = out.begin();
  auto end = in.end();
  size_t len = 0;
  for (const_iterator it = in.begin(); it < end; ++it) {
    switch (*it) {
      case '%':
        len += 2;
        *outIt++ = '%';
        *outIt++ = '%';
        break;
      case ':':
        len += 2;
        *outIt++ = '%';
        *outIt++ = 'c';
        break;
      case '?':
        len += 2;
        *outIt++ = '%';
        *outIt++ = 'q';
        break;
      case ';':
        len += 2;
        *outIt++ = '%';
        *outIt++ = 's';
        break;
      case '/':
        len += 2;
        *outIt++ = '%';
        *outIt++ = 'b';
        break;
      default:
        len++;
        *outIt++ = *it;
        break;
    }
  }
  out.resize(len);
  return String(std::move(out));
}
}

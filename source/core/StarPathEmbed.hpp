#pragma once

#include "StarBiMap.hpp"
#include "StarEncode.hpp"
#include "StarCompression.hpp"
#include "StarZSTDCompression.hpp"

namespace Star {
    
// Some utility functions for embedding data into asset paths.
enum EmbeddedCompressionMethod {
  None,
  Gzip,
  Zlib,
  Zstd
};
extern EnumMap<EmbeddedCompressionMethod> const EmbeddedCompressionMethodNames;

String embedAndCompressData(ByteArray const& data, EmbeddedCompressionMethod const& compressionMethod);
String embedEscapeText(String const& data);

}

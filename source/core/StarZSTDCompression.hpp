#pragma once
#include "StarByteArray.hpp"
#include "StarDataStreamDevices.hpp"

typedef struct ZSTD_CCtx_s ZSTD_CCtx;
typedef struct ZSTD_DCtx_s ZSTD_DCtx;
typedef ZSTD_DCtx ZSTD_DStream;
typedef ZSTD_CCtx ZSTD_CStream;

namespace Star {

class CompressionStream {
public:
  CompressionStream();
  ~CompressionStream();

  ByteArray compress(const char* in, size_t inLen);
  ByteArray compress(ByteArray const& in);

private:
  ZSTD_CStream* m_cStream;
};

inline ByteArray CompressionStream::compress(ByteArray const& in) {
  return compress(in.ptr(), in.size());
}

class DecompressionStream {
public:
  DecompressionStream();
  ~DecompressionStream();

  ByteArray decompress(const char* in, size_t inLen);
  ByteArray decompress(ByteArray const& in);

private:
  ZSTD_DStream* m_dStream;
};

inline ByteArray DecompressionStream::decompress(ByteArray const& in) {
  return decompress(in.ptr(), in.size());
}

}
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

  void compress(const char* in, size_t inLen, ByteArray& out);
  void compress(ByteArray const& in, ByteArray& out);
  ByteArray compress(const char* in, size_t inLen);
  ByteArray compress(ByteArray const& in);

private:
  ZSTD_CStream* m_cStream;
};

class DecompressionStream {
public:
  DecompressionStream();
  ~DecompressionStream();

  void decompress(const char* in, size_t inLen, ByteArray& out);
  void decompress(ByteArray const& in, ByteArray& out);
  ByteArray decompress(const char* in, size_t inLen);
  ByteArray decompress(ByteArray const& in);

private:
  ZSTD_DStream* m_dStream;
};

}
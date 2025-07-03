#include "StarZSTDCompression.hpp"
#include <zstd.h>

namespace Star {

CompressionStream::CompressionStream() : m_cStream(ZSTD_createCStream()) {
  ZSTD_CCtx_setParameter(m_cStream, ZSTD_c_enableLongDistanceMatching, 1);
  ZSTD_CCtx_setParameter(m_cStream, ZSTD_c_windowLog, 24);
  ZSTD_initCStream(m_cStream, 2);
}

CompressionStream::~CompressionStream() { ZSTD_freeCStream(m_cStream); }

void CompressionStream::compress(const char* in, size_t inLen, ByteArray& out) {
  size_t const cOutSize = ZSTD_CStreamOutSize();
  ZSTD_inBuffer inBuffer = {in, inLen, 0};
  size_t written = out.size();
  out.resize(out.size() + cOutSize);
  bool finished = false;
  do {
    ZSTD_outBuffer outBuffer = {out.ptr() + written, min(cOutSize, out.size() - written), 0};
    size_t ret = ZSTD_compressStream2(m_cStream, &outBuffer, &inBuffer, ZSTD_e_flush);
    if (ZSTD_isError(ret)) {
      throw IOException(strf("ZSTD compression error {}", ZSTD_getErrorName(ret)));
      break;
    }

    written += outBuffer.pos;
    if (outBuffer.pos == outBuffer.size) {
      if (written >= out.size())
        out.resize(out.size() * 2);
      continue;
    }

    finished = ret == 0 && inBuffer.pos == inBuffer.size;
  } while (!finished);
  out.resize(written);
}

void CompressionStream::compress(ByteArray const& in, ByteArray& out) {
  return compress(in.ptr(), in.size(), out);
}


ByteArray CompressionStream::compress(const char* in, size_t inLen) {
  ByteArray out;
  compress(in, inLen, out);
  return out;
}

ByteArray CompressionStream::compress(ByteArray const& in) {
  ByteArray out;
  compress(in.ptr(), in.size(), out);
  return out;
}

DecompressionStream::DecompressionStream() : m_dStream(ZSTD_createDStream()) {
  ZSTD_DCtx_setParameter(m_dStream, ZSTD_d_windowLogMax, 25);
  ZSTD_initDStream(m_dStream);
}

DecompressionStream::~DecompressionStream() { ZSTD_freeDStream(m_dStream); }

void DecompressionStream::decompress(const char* in, size_t inLen, ByteArray& out) {
  size_t const dOutSize = ZSTD_DStreamOutSize();
  ZSTD_inBuffer inBuffer = {in, inLen, 0};
  size_t written = out.size();
  out.resize(out.size() + dOutSize);
  bool finished = false;
  do {
    ZSTD_outBuffer outBuffer = {out.ptr() + written, min(dOutSize, out.size() - written), 0};
    size_t ret = ZSTD_decompressStream(m_dStream, &outBuffer, &inBuffer);
    if (ZSTD_isError(ret)) {
      throw IOException(strf("ZSTD decompression error {}", ZSTD_getErrorName(ret)));
      break;
    }

    written += outBuffer.pos;
    if (outBuffer.pos == outBuffer.size) {
      if (written >= out.size())
        out.resize(out.size() * 2);
      continue;
    }
    finished = inBuffer.pos == inBuffer.size;
  } while (!finished);
  out.resize(written);
}

void DecompressionStream::decompress(ByteArray const& in, ByteArray& out) {
  return decompress(in.ptr(), in.size(), out);
}

ByteArray DecompressionStream::decompress(const char* in, size_t inLen) {
  ByteArray out;
  decompress(in, inLen, out);
  return out;
}

ByteArray DecompressionStream::decompress(ByteArray const& in) {
  ByteArray out;
  decompress(in.ptr(), in.size(), out);
  return out;
}

}
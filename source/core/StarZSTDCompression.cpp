#include "StarZSTDCompression.hpp"
#include <zstd.h>

namespace Star {

CompressionStream::CompressionStream() : m_cStream(ZSTD_createCStream()) {
  ZSTD_CCtx_setParameter(m_cStream, ZSTD_c_enableLongDistanceMatching, 1);
  ZSTD_CCtx_setParameter(m_cStream, ZSTD_c_windowLog, 24);
  ZSTD_initCStream(m_cStream, 2);
  m_output.resize(ZSTD_CStreamOutSize());
}

CompressionStream::~CompressionStream() { ZSTD_freeCStream(m_cStream); }

ByteArray CompressionStream::compress(const char* in, size_t inLen) {
  size_t const cOutSize = ZSTD_CStreamOutSize();
  ZSTD_inBuffer inBuffer = {in, inLen, 0};
  size_t written = 0;
  bool finished = false;
  do {
    ZSTD_outBuffer outBuffer = {m_output.ptr() + written, min(cOutSize, m_output.size() - written), 0};
    size_t ret = ZSTD_compressStream2(m_cStream, &outBuffer, &inBuffer, ZSTD_e_flush);
    if (ZSTD_isError(ret)) {
      throw IOException(strf("ZSTD compression error {}", ZSTD_getErrorName(ret)));
      break;
    }

    written += outBuffer.pos;
    if (outBuffer.pos == outBuffer.size) {
      if (written >= m_output.size())
        m_output.resize(m_output.size() * 2);
      continue;
    }

    finished = ret == 0 && inBuffer.pos == inBuffer.size;
  } while (!finished);
  return ByteArray(m_output.ptr(), written);
}

DecompressionStream::DecompressionStream() : m_dStream(ZSTD_createDStream()) {
  ZSTD_DCtx_setParameter(m_dStream, ZSTD_d_windowLogMax, 25);
  ZSTD_initDStream(m_dStream);
  m_output.resize(ZSTD_DStreamOutSize());
}

DecompressionStream::~DecompressionStream() { ZSTD_freeDStream(m_dStream); }

ByteArray DecompressionStream::decompress(const char* in, size_t inLen) {
  size_t const dOutSize = ZSTD_DStreamOutSize();
  ZSTD_inBuffer inBuffer = {in, inLen, 0};
  size_t written = 0;
  bool finished = false;
  do {
    ZSTD_outBuffer outBuffer = {m_output.ptr() + written, min(dOutSize, m_output.size() - written), 0};
    size_t ret = ZSTD_decompressStream(m_dStream, &outBuffer, &inBuffer);
    if (ZSTD_isError(ret)) {
      throw IOException(strf("ZSTD decompression error {}", ZSTD_getErrorName(ret)));
      break;
    }

    written += outBuffer.pos;
    if (outBuffer.pos == outBuffer.size) {
      if (written >= m_output.size())
        m_output.resize(m_output.size() * 2);
      continue;
    }
    finished = inBuffer.pos == inBuffer.size;
  } while (!finished);
  return ByteArray(m_output.ptr(), written);
}

}
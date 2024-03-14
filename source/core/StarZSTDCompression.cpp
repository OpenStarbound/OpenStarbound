#include "StarZSTDCompression.hpp"
#include <zstd.h>

namespace Star {

CompressionStream::CompressionStream() : m_cStream(ZSTD_createCStream()) {
  ZSTD_CCtx_setParameter(m_cStream, ZSTD_c_enableLongDistanceMatching, 1);
  ZSTD_CCtx_setParameter(m_cStream, ZSTD_c_windowLog, 24);
  ZSTD_initCStream(m_cStream, 2);
}

CompressionStream::~CompressionStream() { ZSTD_freeCStream(m_cStream); }

ByteArray CompressionStream::compress(const char* in, size_t inLen) {
  size_t const cInSize  = ZSTD_CStreamInSize ();
  size_t const cOutSize = ZSTD_CStreamOutSize();
  ByteArray output(cOutSize, 0);
  size_t written = 0, read = 0;
  while (read < inLen) {
    ZSTD_inBuffer inBuffer = {in + read, min(cInSize, inLen - read), 0};
    ZSTD_outBuffer outBuffer = {output.ptr() + written, output.size() - written, 0};
    bool finished = false;
    do {
      size_t ret = ZSTD_compressStream2(m_cStream, &outBuffer, &inBuffer, ZSTD_e_flush);
      if (ZSTD_isError(ret)) {
        throw IOException(strf("ZSTD compression error {}", ZSTD_getErrorName(ret)));
        break;
      }

      if (outBuffer.pos == outBuffer.size) {
        output.resize(output.size() * 2);
        outBuffer.dst = output.ptr();
        outBuffer.size = output.size();
        continue;
      }

      finished = ret == 0 && inBuffer.pos == inBuffer.size;
    } while (!finished);

    read += inBuffer.pos;
    written += outBuffer.pos;
  }
  output.resize(written);
  return output;
}

DecompressionStream::DecompressionStream() : m_dStream(ZSTD_createDStream()) {
  ZSTD_DCtx_setParameter(m_dStream, ZSTD_d_windowLogMax, 25);
  ZSTD_initDStream(m_dStream);
}

DecompressionStream::~DecompressionStream() { ZSTD_freeDStream(m_dStream); }

ByteArray DecompressionStream::decompress(const char* in, size_t inLen) {
  size_t const dInSize  = ZSTD_DStreamInSize ();
  size_t const dOutSize = ZSTD_DStreamOutSize();
  ByteArray output(dOutSize, 0);
  size_t written = 0, read = 0;
  while (read < inLen) {
    ZSTD_inBuffer inBuffer = {in + read, min(dInSize, inLen - read), 0};
    ZSTD_outBuffer outBuffer = {output.ptr() + written, output.size() - written, 0};
    do {
      size_t ret = ZSTD_decompressStream(m_dStream, &outBuffer, &inBuffer);
      if (ZSTD_isError(ret)) {
        throw IOException(strf("ZSTD decompression error {}", ZSTD_getErrorName(ret)));
        break;
      }

      if (outBuffer.pos == outBuffer.size) {
        output.resize(output.size() * 2);
        outBuffer.dst = output.ptr();
        outBuffer.size = output.size();
        continue;
      }
    } while (inBuffer.pos < inBuffer.size);

    read += inBuffer.pos;
    written += outBuffer.pos;
  }
  output.resize(written);
  return output;
}

}
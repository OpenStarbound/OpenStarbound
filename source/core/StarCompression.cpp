#include "StarCompression.hpp"
#include "StarFormat.hpp"
#include "StarLexicalCast.hpp"

#include <zlib.h>
#include <errno.h>
#include <string.h>

namespace Star {

void compressData(ByteArray const& in, ByteArray& out, CompressionLevel compression) {
  out.clear();

  if (in.empty())
    return;

  const size_t BUFSIZE = 32 * 1024;
  auto tempBuffer = std::make_unique<unsigned char[]>(BUFSIZE);

  z_stream strm{};
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  int deflate_res = deflateInit(&strm, compression);
  if (deflate_res != Z_OK)
    throw IOException(strf("Failed to initialise deflate ({})", deflate_res));

  strm.next_in = (unsigned char*)in.ptr();
  strm.avail_in = in.size();
  strm.next_out = tempBuffer.get();
  strm.avail_out = BUFSIZE;
  while (deflate_res == Z_OK) {
    deflate_res = deflate(&strm, Z_FINISH);
    if (strm.avail_out == 0) {
      out.append((char const*)tempBuffer.get(), BUFSIZE);
      strm.next_out = tempBuffer.get();
      strm.avail_out = BUFSIZE;
    }
  }
  deflateEnd(&strm);

  if (deflate_res != Z_STREAM_END)
    throw IOException(strf("Internal error in uncompressData, deflate_res is {}", deflate_res));

  out.append((char const*)tempBuffer.get(), BUFSIZE - strm.avail_out);
}

ByteArray compressData(ByteArray const& in, CompressionLevel compression) {
  ByteArray out = ByteArray::withReserve(in.size());
  compressData(in, out, compression);
  return out;
}

void uncompressData(const char* in, size_t inLen, ByteArray& out, size_t limit) {
  out.clear();

  if (!inLen)
    return;

  const size_t BUFSIZE = 32 * 1024;
  auto tempBuffer = std::make_unique<unsigned char[]>(BUFSIZE);

  z_stream strm{};
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  int inflate_res = inflateInit(&strm);
  if (inflate_res != Z_OK)
    throw IOException(strf("Failed to initialise inflate ({})", inflate_res));

  strm.next_in = (unsigned char*)in;
  strm.avail_in = inLen;
  strm.next_out = tempBuffer.get();
  strm.avail_out = BUFSIZE;

  while (inflate_res == Z_OK || inflate_res == Z_BUF_ERROR) {
    inflate_res = inflate(&strm, Z_FINISH);
    if (strm.avail_out == 0) {
      out.append((char const*)tempBuffer.get(), BUFSIZE);
      strm.next_out = tempBuffer.get();
      strm.avail_out = BUFSIZE;
      if (limit && out.size() >= limit) {
        inflateEnd(&strm);
        throw IOException(strf("hit uncompressData limit of {} bytes", limit));
        break;
      }
    } else if (inflate_res == Z_BUF_ERROR) {
      break;
    }
  }
  inflateEnd(&strm);

  if (inflate_res != Z_STREAM_END)
    throw IOException(strf("Internal error in uncompressData, inflate_res is {}", inflate_res));

  out.append((char const*)tempBuffer.get(), BUFSIZE - strm.avail_out);
}

ByteArray uncompressData(const char* in, size_t inLen, size_t limit) {
  ByteArray out = ByteArray::withReserve(inLen);
  uncompressData(in, inLen, out, limit);
  return out;
}

void uncompressData(ByteArray const& in, ByteArray& out, size_t limit) {
  uncompressData(in.ptr(), in.size(), out, limit);
}

ByteArray uncompressData(ByteArray const& in, size_t limit) {
  return uncompressData(in.ptr(), in.size(), limit);
}

CompressedFilePtr CompressedFile::open(String const& filename, IOMode mode, CompressionLevel comp) {
  CompressedFilePtr f = make_shared<CompressedFile>(filename);
  f->open(mode, comp);
  return f;
}

CompressedFile::CompressedFile()
  : IODevice(IOMode::Closed), m_file(0), m_compression(MediumCompression) {}

CompressedFile::CompressedFile(String filename)
  : IODevice(IOMode::Closed), m_file(0), m_compression(MediumCompression) {
  setFilename(std::move(filename));
}

CompressedFile::~CompressedFile() {
  close();
}

StreamOffset CompressedFile::pos() {
  return gztell((gzFile)m_file);
}

void CompressedFile::seek(StreamOffset offset, IOSeek seekMode) {
  StreamOffset begPos = pos();

  int retCode;
  if (seekMode == IOSeek::Relative) {
    retCode = gzseek((gzFile)m_file, (z_off_t)offset, SEEK_CUR);
  } else if (seekMode == IOSeek::Absolute) {
    retCode = gzseek((gzFile)m_file, (z_off_t)offset, SEEK_SET);
  } else {
    throw IOException("Cannot seek with SeekEnd in compressed file");
  }

  StreamOffset endPos = pos();

  if (retCode < 0) {
    throw IOException::format("Seek error: {}", gzerror((gzFile)m_file, 0));
  } else if ((seekMode == IOSeek::Relative && begPos + offset != endPos)
      || (seekMode == IOSeek::Absolute && offset != endPos)) {
    throw EofException("Error, unexpected end of file found");
  }
}

bool CompressedFile::atEnd() {
  return gzeof((gzFile)m_file);
}

size_t CompressedFile::read(char* data, size_t len) {
  if (len == 0)
    return 0;

  int ret = gzread((gzFile)m_file, data, len);
  if (ret == 0)
    throw EofException("Error, unexpected end of file found");
  else if (ret == -1)
    throw IOException::format("Read error: {}", gzerror((gzFile)m_file, 0));
  else
    return (size_t)ret;
}

size_t CompressedFile::write(const char* data, size_t len) {
  if (len == 0)
    return 0;

  int ret = gzwrite((gzFile)m_file, data, len);
  if (ret == 0)
    throw IOException::format("Write error: {}", gzerror((gzFile)m_file, 0));
  else
    return (size_t)ret;
}

void CompressedFile::setFilename(String filename) {
  if (isOpen())
    throw IOException("Cannot call setFilename while CompressedFile is open");
  m_filename = std::move(filename);
}

void CompressedFile::setCompression(CompressionLevel compression) {
  if (isOpen())
    throw IOException("Cannot call setCompression while CompressedFile is open");
  m_compression = compression;
}

void CompressedFile::open(IOMode mode, CompressionLevel compression) {
  close();
  setCompression(compression);
  open(mode);
}

void CompressedFile::sync() {
  gzflush((gzFile)m_file, Z_FULL_FLUSH);
}

void CompressedFile::open(IOMode mode) {
  setMode(mode);
  String modeString;

  if (mode & IOMode::Append) {
    throw IOException("CompressedFile not compatible with Append mode");
  } else if ((mode & IOMode::Read) && (mode & IOMode::Write)) {
    throw IOException("CompressedFile not compatible with ReadWrite mode");
  } else if (mode & IOMode::Write) {
    modeString = "wb";
  } else if (mode & IOMode::Read) {
    modeString = "rb";
  }

  modeString += toString(m_compression);

  m_file = gzopen(m_filename.utf8Ptr(), modeString.utf8Ptr());

  if (!m_file)
    throw IOException::format("Cannot open filename '{}'", m_filename);
}

void CompressedFile::close() {
  if (m_file)
    gzclose((gzFile)m_file);
  m_file = 0;
  setMode(IOMode::Closed);
}

}

#ifndef STAR_COMPRESSION_HPP
#define STAR_COMPRESSION_HPP

#include "StarIODevice.hpp"
#include "StarString.hpp"

namespace Star {

STAR_CLASS(CompressedFile);

// Zlib compression level, ranges from 0 to 9
typedef int CompressionLevel;

CompressionLevel const LowCompression = 2;
CompressionLevel const MediumCompression = 5;
CompressionLevel const HighCompression = 9;

void compressData(ByteArray const& in, ByteArray& out, CompressionLevel compression = MediumCompression);
ByteArray compressData(ByteArray const& in, CompressionLevel compression = MediumCompression);

void uncompressData(ByteArray const& in, ByteArray& out);
ByteArray uncompressData(ByteArray const& in);

// Random access to a (potentially) compressed file.
class CompressedFile : public IODevice {
public:
  static CompressedFilePtr open(String const& filename, IOMode mode, CompressionLevel comp = MediumCompression);

  CompressedFile();
  CompressedFile(String filename);
  virtual ~CompressedFile();

  void setFilename(String filename);
  void setCompression(CompressionLevel compression);

  StreamOffset pos() override;
  // Only seek forward is supported on writes.  Seek is emulated *slowly* on
  // reads.
  void seek(StreamOffset pos, IOSeek seek = IOSeek::Absolute) override;
  bool atEnd() override;
  size_t read(char* data, size_t len) override;
  size_t write(char const* data, size_t len) override;

  void open(IOMode mode) override;
  // Compression is ignored on read.  Always truncates on write
  void open(IOMode mode, CompressionLevel compression);

  void sync() override;
  void close() override;

private:
  String m_filename;
  void* m_file;
  CompressionLevel m_compression;
};

}

#endif

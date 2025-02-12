#include "StarFile.hpp"
#include "StarByteArray.hpp"
#include "StarFormat.hpp"

#include <fstream>
#include <pthread.h>

namespace Star {

void File::makeDirectoryRecursive(String const& fileName) {
  auto parent = dirName(fileName);
  if (!isDirectory(parent))
    makeDirectoryRecursive(parent);
  if (!isDirectory(fileName))
    makeDirectory(fileName);
}

void File::removeDirectoryRecursive(String const& fileName) {
  {
    String fileInDir;
    bool isDir;

    for (auto const& p : dirList(fileName)) {
      std::tie(fileInDir, isDir) = p;

      fileInDir = relativeTo(fileName, fileInDir);

      if (isDir)
        removeDirectoryRecursive(fileInDir);
      else
        remove(fileInDir);
    }
  }

  remove(fileName);
}

void File::copy(String const& source, String const& target) {
  auto sourceFile = File::open(source, IOMode::Read);
  auto targetFile = File::open(target, IOMode::ReadWrite);

  targetFile->resize(0);

  char buf[1024];
  while (!sourceFile->atEnd()) {
    size_t r = sourceFile->read(buf, 1024);
    targetFile->writeFull(buf, r);
  }
}

FilePtr File::open(const String& filename, IOMode mode) {
  auto file = make_shared<File>(filename);
  file->open(mode);
  return file;
}

ByteArray File::readFile(String const& filename) {
  FilePtr file = File::open(filename, IOMode::Read);
  ByteArray bytes;
  while (!file->atEnd()) {
    char buffer[1024];
    size_t r = file->read(buffer, 1024);
    bytes.append(buffer, r);
  }

  return bytes;
}

String File::readFileString(String const& filename) {
  FilePtr file = File::open(filename, IOMode::Read);
  std::string str;
  while (!file->atEnd()) {
    char buffer[1024];
    size_t r = file->read(buffer, 1024);
    for (size_t i = 0; i < r; ++i)
      str.push_back(buffer[i]);
  }

  return str;
}

StreamOffset File::fileSize(String const& filename) {
  return File::open(filename, IOMode::Read)->size();
}

ByteArray File::mmapFile(String const& filename) {
  return ByteArray::fromMMap(filename.utf8Ptr());
}

ByteArray File::mmapFilePartial(String const& filename, size_t offset, size_t len) {
  return ByteArray::fromMMap(filename.utf8Ptr(), offset, len);
}

void File::writeFile(char const* data, size_t len, String const& filename) {
  FilePtr file = File::open(filename, IOMode::Write | IOMode::Truncate);
  file->writeFull(data, len);
}

void File::writeFile(ByteArray const& data, String const& filename) {
  writeFile(data.ptr(), data.size(), filename);
}

void File::writeFile(String const& data, String const& filename) {
  writeFile(data.utf8Ptr(), data.utf8Size(), filename);
}

void File::overwriteFileWithRename(ByteArray const& data, String const& filename, String const& newSuffix) {
  overwriteFileWithRename(data.ptr(), data.size(), filename, newSuffix);
}

void File::overwriteFileWithRename(String const& data, String const& filename, String const& newSuffix) {
  overwriteFileWithRename(data.utf8Ptr(), data.utf8Size(), filename, newSuffix);
}

void File::backupFileInSequence(String const& initialFile, String const& targetFile, unsigned maximumBackups, String const& backupExtensionPrefix) {
  for (unsigned i = maximumBackups; i > 0; --i) {
    bool initial = i == 1;
    String const& sourceFile = initial ? initialFile : targetFile;
    String curExtension = initial ? "" : strf("{}{}", backupExtensionPrefix, i - 1);
    String nextExtension = strf("{}{}", backupExtensionPrefix, i);

    if (File::isFile(sourceFile + curExtension))
      File::copy(sourceFile + curExtension, targetFile + nextExtension);
  }
}

void File::backupFileInSequence(String const& targetFile, unsigned maximumBackups, String const& backupExtensionPrefix) {
  backupFileInSequence(targetFile, targetFile, maximumBackups, backupExtensionPrefix);
}

File::File()
  : IODevice(IOMode::Closed) {
  m_file = 0;
}

File::File(String filename)
  : IODevice(IOMode::Closed), m_filename(std::move(filename)), m_file(0) {}

File::~File() {
  close();
}

StreamOffset File::pos() {
  if (!m_file)
    throw IOException("pos called on closed File");

  return ftell(m_file);
}

void File::seek(StreamOffset offset, IOSeek seekMode) {
  if (!m_file)
    throw IOException("seek called on closed File");

  fseek(m_file, offset, seekMode);
}

StreamOffset File::size() {
  return fsize(m_file);
}

bool File::atEnd() {
  if (!m_file)
    throw IOException("eof called on closed File");

  return ftell(m_file) >= fsize(m_file);
}

size_t File::read(char* data, size_t len) {
  if (!m_file)
    throw IOException("read called on closed File");

  if (!isReadable())
    throw IOException("read called on non-readable File");

  return fread(m_file, data, len);
}

size_t File::write(const char* data, size_t len) {
  if (!m_file)
    throw IOException("write called on closed File");

  if (!isWritable())
    throw IOException("write called on non-writable File");

  return fwrite(m_file, data, len);
}

size_t File::readAbsolute(StreamOffset readPosition, char* data, size_t len) {
  return pread(m_file, data, len, readPosition);
}

size_t File::writeAbsolute(StreamOffset writePosition, char const* data, size_t len) {
  return pwrite(m_file, data, len, writePosition);
}

String File::fileName() const {
  return m_filename;
}

void File::setFilename(String filename) {
  if (isOpen())
    throw IOException("Cannot call setFilename while File is open");
  m_filename = std::move(filename);
}

void File::remove() {
  close();
  if (m_filename.empty())
    throw IOException("Cannot remove file, no filename set");
  remove(m_filename);
}

void File::resize(StreamOffset s) {
  bool tempOpen = false;
  if (!isOpen()) {
    tempOpen = true;
    open(mode());
  }

  File::resize(m_file, s);

  if (tempOpen)
    close();
}

void File::sync() {
  if (!m_file)
    throw IOException("sync called on closed File");

  fsync(m_file);
}

void File::open(IOMode m) {
  close();
  if (m_filename.empty())
    throw IOException("Cannot open file, no filename set");

  m_file = fopen(m_filename.utf8Ptr(), m);
  setMode(m);
}

void File::close() {
  if (m_file)
    fclose(m_file);
  m_file = 0;
  setMode(IOMode::Closed);
}

String File::deviceName() const {
  if (m_filename.empty())
    return "<unnamed temp file>";
  else
    return m_filename;
}

}

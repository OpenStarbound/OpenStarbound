#include "StarFile.hpp"
#include "StarFormat.hpp"
#include "StarRandom.hpp"
#include "StarEncode.hpp"

#include <errno.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <libgen.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef STAR_SYSTEM_MACOSX
#include <mach-o/dyld.h>
#elif defined STAR_SYSTEM_FREEBSD
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

namespace Star {

namespace {
  int fdFromHandle(void* ptr) {
    return (int)(intptr_t)ptr;
  }

  void* handleFromFd(int handle) {
    return (void*)(intptr_t)handle;
  }
}

String File::convertDirSeparators(String const& path) {
  return path.replace("\\", "/");
}

String File::currentDirectory() {
  char buffer[PATH_MAX];
  if (::getcwd(buffer, PATH_MAX) == NULL)
    throw IOException("getcwd failed");

  return String(buffer);
}

void File::changeDirectory(const String& dirName) {
  if (::chdir(dirName.utf8Ptr()) != 0)
    throw IOException(strf("could not change directory to {}", dirName));
}

void File::makeDirectory(String const& dirName) {
  if (::mkdir(dirName.utf8Ptr(), 0777) != 0)
    throw IOException(strf("could not create directory '{}', {}", dirName, strerror(errno)));
}

List<pair<String, bool>> File::dirList(const String& dirName, bool skipDots) {
  List<std::pair<String, bool>> fileList;
  DIR* directory = ::opendir(dirName.utf8Ptr());
  if (directory == NULL)
    throw IOException::format("dirList failed on dir: '{}'", dirName);

  for (dirent* entry = ::readdir(directory); entry != NULL; entry = ::readdir(directory)) {
    String entryString = entry->d_name;
    if (!skipDots || (entryString != "." && entryString != "..")) {
      bool isDirectory = false;
      if (entry->d_type == DT_DIR) {
        isDirectory = true;
      } else if (entry->d_type == DT_LNK || entry->d_type == DT_UNKNOWN) {
        isDirectory = File::isDirectory(File::relativeTo(dirName, entryString));
      }
      fileList.append({entryString, isDirectory});
    }
  }
  ::closedir(directory);

  return fileList;
}

String File::baseName(const String& fileName) {
  String ret;

  std::string file = fileName.utf8();
  char* fn = new char[file.size() + 1];
  std::copy(file.begin(), file.end(), fn);
  fn[file.size()] = 0;
  ret = String(::basename(fn));
  delete[] fn;

  return ret;
}

String File::dirName(const String& fileName) {
  String ret;

  std::string file = fileName.utf8();
  char* fn = new char[file.size() + 1];
  std::copy(file.begin(), file.end(), fn);
  fn[file.size()] = 0;
  ret = String(::dirname(fn));
  delete[] fn;

  return ret;
}

String File::relativeTo(String const& relativeTo, String const& path) {
  if (path.beginsWith("/"))
    return path;
  return relativeTo.trimEnd("/") + '/' + path;
}

String File::fullPath(const String& fileName) {
  char buffer[PATH_MAX];

  if (::realpath(fileName.utf8Ptr(), buffer) == NULL)
    throw IOException::format("realpath failed on file: '{}' problem path was: '{}'", fileName, buffer);

  return String(buffer);
}

String File::temporaryFileName() {
  return relativeTo(P_tmpdir, strf("starbound.tmpfile.{}", hexEncode(Random::randBytes(16))));
}

FilePtr File::temporaryFile() {
  return open(temporaryFileName(), IOMode::ReadWrite);
}

FilePtr File::ephemeralFile() {
  auto file = make_shared<File>();
  ByteArray path = ByteArray::fromCStringWithNull(relativeTo(P_tmpdir, "starbound.tmpfile.XXXXXXXX").utf8Ptr());
  auto res = mkstemp(path.ptr());
  if (res < 0)
    throw IOException::format("tmpfile error: {}", strerror(errno));
  if (::unlink(path.ptr()) < 0)
    throw IOException::format("Could not remove mkstemp file when creating ephemeralFile: {}", strerror(errno));
  file->m_file = handleFromFd(res);
  file->setMode(IOMode::ReadWrite);
  return file;
}

String File::temporaryDirectory() {
  String dirname = relativeTo(P_tmpdir, strf("starbound.tmpdir.{}", hexEncode(Random::randBytes(16))));
  makeDirectory(dirname);
  return dirname;
}

bool File::exists(String const& path) {
  struct stat st_buf;
  int status = stat(path.utf8Ptr(), &st_buf);
  return status == 0;
}

bool File::isFile(String const& path) {
  struct stat st_buf;
  int status = stat(path.utf8Ptr(), &st_buf);
  if (status != 0)
    return false;

  return S_ISREG(st_buf.st_mode);
}

bool File::isDirectory(String const& path) {
  struct stat st_buf;
  int status = stat(path.utf8Ptr(), &st_buf);
  if (status != 0)
    return false;

  return S_ISDIR(st_buf.st_mode);
}

void File::remove(String const& filename) {
  if (::remove(filename.utf8Ptr()) < 0)
    throw IOException::format("remove error: {}", strerror(errno));
}

void File::rename(String const& source, String const& target) {
  if (::rename(source.utf8Ptr(), target.utf8Ptr()) < 0)
    throw IOException::format("rename error: {}", strerror(errno));
}

void File::overwriteFileWithRename(char const* data, size_t len, String const& filename, String const& newSuffix) {
  String newFile = filename + newSuffix;
  writeFile(data, len, newFile);
  File::rename(newFile, filename);
}

void* File::fopen(char const* filename, IOMode mode) {
  int oflag = 0;

  if (mode & IOMode::Read && mode & IOMode::Write)
    oflag |= O_RDWR | O_CREAT;
  else if (mode & IOMode::Read)
    oflag |= O_RDONLY;
  else if (mode & IOMode::Write)
    oflag |= O_WRONLY | O_CREAT;

  if (mode & IOMode::Truncate)
    oflag |= O_TRUNC;

  int fd = ::open(filename, oflag, 0666);
  if (fd < 0)
    throw IOException::format("Error opening file '{}', error: {}", filename, strerror(errno));

  if (mode & IOMode::Append) {
    if (lseek(fd, 0, SEEK_END) < 0)
      throw IOException::format("Error opening file '{}', cannot seek: {}", filename, strerror(errno));
  }

  return handleFromFd(fd);
}

void File::fseek(void* f, StreamOffset offset, IOSeek seekMode) {
  auto fd = fdFromHandle(f);
  int retCode;
  if (seekMode == IOSeek::Relative)
    retCode = lseek(fd, offset, SEEK_CUR);
  else if (seekMode == IOSeek::Absolute)
    retCode = lseek(fd, offset, SEEK_SET);
  else
    retCode = lseek(fd, offset, SEEK_END);

  if (retCode < 0)
    throw IOException::format("Seek error: {}", strerror(errno));
}

StreamOffset File::ftell(void* f) {
  return lseek(fdFromHandle(f), 0, SEEK_CUR);
}

size_t File::fread(void* file, char* data, size_t len) {
  if (len == 0)
    return 0;

  auto fd = fdFromHandle(file);
  auto ret = ::read(fd, data, len);
  if (ret < 0) {
    if (errno == EAGAIN || errno == EINTR)
      return 0;
    throw IOException::format("Read error: {}", strerror(errno));
  } else {
    return ret;
  }
}

size_t File::fwrite(void* file, char const* data, size_t len) {
  if (len == 0)
    return 0;

  auto fd = fdFromHandle(file);
  auto ret = ::write(fd, data, len);
  if (ret < 0) {
    if (errno == EAGAIN || errno == EINTR)
      return 0;
    throw IOException::format("Write error: {}", strerror(errno));
  } else {
    return ret;
  }
}

void File::fsync(void* file) {
  auto fd = fdFromHandle(file);
#ifdef STAR_SYSTEM_LINUX
  ::fdatasync(fd);
#else
  ::fsync(fd);
#endif
}

void File::fclose(void* file) {
  if (::close(fdFromHandle(file)) < 0)
    throw IOException::format("Close error: {}", strerror(errno));
}

StreamOffset File::fsize(void* file) {
  StreamOffset pos = ftell(file);
  StreamOffset size = lseek(fdFromHandle(file), 0, SEEK_END);
  lseek(fdFromHandle(file), pos, SEEK_SET);
  return size;
}

size_t File::pread(void* file, char* data, size_t len, StreamOffset position) {
  return ::pread(fdFromHandle(file), data, len, position);
}

size_t File::pwrite(void* file, char const* data, size_t len, StreamOffset position) {
  return ::pwrite(fdFromHandle(file), data, len, position);
}

void File::resize(void* f, StreamOffset size) {
  if (::ftruncate(fdFromHandle(f), size) < 0)
    throw IOException::format("resize error: {}", strerror(errno));
}

}

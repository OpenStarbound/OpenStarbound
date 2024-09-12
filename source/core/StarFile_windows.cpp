#include "StarFile.hpp"
#include "StarFormat.hpp"
#include "StarRandom.hpp"
#include "StarEncode.hpp"
#include "StarMathCommon.hpp"
#include "StarThread.hpp"

#include "StarString_windows.hpp"

#include <errno.h>
#include <io.h>
#include <stdio.h>
#include <windows.h>

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

namespace Star {

OVERLAPPED makeOverlapped(StreamOffset offset) {
  OVERLAPPED overlapped = {};
  overlapped.Offset = offset;
  overlapped.OffsetHigh = offset >> 32;
  return overlapped;
}

String File::convertDirSeparators(String const& path) {
  return path.replace("/", "\\");
}

String File::currentDirectory() {
  WCHAR buffer[MAX_PATH];
  size_t len = GetCurrentDirectoryW(MAX_PATH, buffer);
  if (len == 0)
    throw IOException("GetCurrentDirectory failed");

  return utf16ToString(buffer);
}

void File::changeDirectory(const String& dirName) {
  if (!SetCurrentDirectoryW(stringToUtf16(dirName).get()))
    throw IOException(strf("could not change directory to {}", dirName));
}

void File::makeDirectory(String const& dirName) {
  if (CreateDirectoryW(stringToUtf16(dirName).get(), NULL) == 0) {
    auto error = GetLastError();
    throw IOException(strf("could not create directory '{}', {}", dirName, error));
  }
}

bool File::exists(String const& path) {
  WIN32_FIND_DATAW findFileData;
  const HANDLE handle = FindFirstFileW(stringToUtf16(path).get(), &findFileData);
  if (handle == INVALID_HANDLE_VALUE)
    return false;
  FindClose(handle);
  return true;
}

bool File::isFile(String const& path) {
  WIN32_FIND_DATAW findFileData;
  const HANDLE handle = FindFirstFileW(stringToUtf16(path).get(), &findFileData);
  if (handle == INVALID_HANDLE_VALUE)
    return false;
  FindClose(handle);
  return (FILE_ATTRIBUTE_DIRECTORY & findFileData.dwFileAttributes) == 0;
}

bool File::isDirectory(String const& path) {
  DWORD attribs = GetFileAttributesW(stringToUtf16(path.trimEnd("\\/")).get());
  if (attribs == INVALID_FILE_ATTRIBUTES)
    return false;
  return attribs & FILE_ATTRIBUTE_DIRECTORY;
}

String File::fullPath(const String& path) {
  WCHAR buffer[MAX_PATH];

  size_t fullpath_size;
  WCHAR* lpszLastNamePart;

  fullpath_size = GetFullPathNameW(stringToUtf16(path).get(), (DWORD)MAX_PATH, buffer, (WCHAR**)&lpszLastNamePart);
  if (0 == fullpath_size)
    throw IOException::format("GetFullPathName failed on path: '{}'", path);
  if (fullpath_size >= MAX_PATH)
    throw IOException::format("GetFullPathName failed on path: '{}'", path);

  return utf16ToString(buffer);
}

List<std::pair<String, bool>> File::dirList(const String& dirName, bool skipDots) {
  List<std::pair<String, bool>> fileList;
  WIN32_FIND_DATAW findFileData;
  HANDLE hFind;

  hFind = FindFirstFileW(stringToUtf16(File::relativeTo(dirName, "*")).get(), &findFileData);
  if (hFind == INVALID_HANDLE_VALUE)
    throw IOException(strf("Invalid file handle in dirList of '{}', error is %u", dirName, GetLastError()));

  while (true) {
    String entry = utf16ToString(findFileData.cFileName);
    if (!skipDots || (entry != "." && entry != ".."))
      fileList.append({entry, (FILE_ATTRIBUTE_DIRECTORY & findFileData.dwFileAttributes) != 0});
    if (!FindNextFileW(hFind, &findFileData))
      break;
  }

  DWORD dwError = GetLastError();
  FindClose(hFind);

  if ((dwError != ERROR_NO_MORE_FILES) && (dwError != NO_ERROR))
    throw IOException(strf("FindNextFile error in dirList of '{}'.  Error is %u", dirName, dwError));

  return fileList;
}

String File::baseName(const String& fileName) {
  return String(fileName).rextract("\\/");
}

String File::dirName(const String& fileName) {
  if (fileName == "\\" || fileName == "/")
    return "\\";

  String directory = fileName;
  directory.rextract("\\/");
  if (directory.empty())
    return ".";
  else
    return directory;
}

String File::relativeTo(String const& relativeTo, String const& path) {
  if (path.beginsWith('/') || path.beginsWith('\\') || path.regexMatch("^[a-z]:", false, false))
    return path;

  String finalPath;
  if (relativeTo.endsWith('\\') || relativeTo.endsWith('/'))
    finalPath = relativeTo.substr(0, relativeTo.size() - 1);
  else if (relativeTo.endsWith("\\.") || relativeTo.endsWith("/."))
    finalPath = relativeTo.substr(0, relativeTo.size() - 2);
  else
    finalPath = relativeTo;

  if (path.beginsWith(".\\") || path.beginsWith("./"))
    finalPath += '\\' + path.substr(2);
  else
    finalPath += '\\' + path;

  return finalPath;
}

String File::temporaryFileName() {
  WCHAR tempPath[MAX_PATH];
  if (!GetTempPathW(MAX_PATH, tempPath)) {
    auto error = GetLastError();
    throw IOException(strf("Could not call GetTempPath {}", error));
  }

  return relativeTo(utf16ToString(tempPath), strf("starbound.tmpfile.{}", hexEncode(Random::randBytes(16))));
}

FilePtr File::temporaryFile() {
  return open(temporaryFileName(), IOMode::ReadWrite);
}

FilePtr File::ephemeralFile() {
  auto file = temporaryFile();
  DeleteFileW(stringToUtf16(file->fileName()).get());
  file->m_filename = "";
  return file;
}

String File::temporaryDirectory() {
  WCHAR tempPath[MAX_PATH];
  if (!GetTempPathW(MAX_PATH, tempPath)) {
    auto error = GetLastError();
    throw IOException(strf("Could not call GetTempPath {}", error));
  }

  String dirname = relativeTo(utf16ToString(tempPath), strf("starbound.tmpdir.{}", hexEncode(Random::randBytes(16))));
  makeDirectory(dirname);
  return dirname;
}

void File::remove(String const& filename) {
  if (isDirectory(filename)) {
    if (!RemoveDirectoryW(stringToUtf16(filename).get())) {
      auto error = GetLastError();
      throw IOException(strf("Rename error: {}", error));
    }
  } else if (::_wremove(stringToUtf16(filename).get()) < 0) {
    auto error = errno;
    throw IOException::format("remove error: {}", strerror(error));
  }
}

void File::rename(String const& source, String const& target) {
  bool replace = File::exists(target);
  auto temp = target + ".tmp";

  if (replace) {
	  if (!DeleteFileW(stringToUtf16(temp).get())) {
      auto error = GetLastError();
      if (error != ERROR_FILE_NOT_FOUND)
        throw IOException(strf("error deleting existing temp file: {}", error));
    }
    if (!MoveFileExW(stringToUtf16(target).get(), stringToUtf16(temp).get(), MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH)) {
      auto error = GetLastError();
      throw IOException(strf("error temporary file '{}': {}", temp, GetLastError()));
    }
  }

  if (!MoveFileExW(stringToUtf16(source).get(), stringToUtf16(target).get(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH)) {
    auto error = GetLastError();
    throw IOException(strf("Rename error: {}", error));
  }

  if (replace && !DeleteFileW(stringToUtf16(temp).get())) {
    auto error = GetLastError();
    throw IOException(strf("error deleting temp file '{}': {}", temp, GetLastError()));
  }
}

void File::overwriteFileWithRename(char const* data, size_t len, String const& filename, String const& newSuffix) {
  String newFile = filename + newSuffix;

  try {
    auto file = File::open(newFile, IOMode::Write | IOMode::Truncate);
    file->writeFull(data, len);
    file->sync();
    file->close();

    File::rename(newFile, filename);
  } catch (IOException const&) {
    // HACK: Been having trouble on windows with the write / flush / move
    // sequence, try super hard to just write the file non-atomically in case
    // of weird file locking problems instead of erroring.

    // Ignore any error on removal of the maybe existing newFile
    ::_wremove(stringToUtf16(newFile).get());

    writeFile(data, len, filename);
  }
}

void* File::fopen(char const* filename, IOMode mode) {
  DWORD desiredAccess = 0;
  if (mode & IOMode::Read)
    desiredAccess |= GENERIC_READ;
  if (mode & IOMode::Write)
    desiredAccess |= GENERIC_WRITE;

  DWORD creationDisposition = 0;
  if (mode & IOMode::Write)
    creationDisposition = OPEN_ALWAYS;
  else
    creationDisposition = OPEN_EXISTING;

  HANDLE file = CreateFileW(stringToUtf16(String(filename)).get(),
      desiredAccess, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
      creationDisposition, 0, NULL);

  if (file == INVALID_HANDLE_VALUE)
    throw IOException::format("could not open file '{}' {}", filename, GetLastError());

  LARGE_INTEGER szero;
  szero.QuadPart = 0;
  if (!SetFilePointerEx(file, szero, NULL, 0)) {
    CloseHandle(file);
    throw IOException::format("could not set file pointer in fopen '{}' {}", filename, GetLastError());
  }

  if (mode & IOMode::Truncate) {
    if (!SetEndOfFile(file)) {
      CloseHandle(file);
      throw IOException::format("could not set end of file in fopen '{}' {}", filename, GetLastError());
    }
  }

  if (mode & IOMode::Append) {
    LARGE_INTEGER size;
    if (GetFileSizeEx(file, &size) == 0) {
      CloseHandle(file);
      throw IOException::format("could not get file size in fopen '{}' {}", filename, GetLastError());
    }
    if (!SetFilePointerEx(file, size, NULL, 0)) {
      CloseHandle(file);
      throw IOException::format("could not set file pointer in fopen '{}' {}", filename, GetLastError());
    }
  }

  return (void*)file;
}

void File::fseek(void* f, StreamOffset offset, IOSeek seekMode) {
  HANDLE file = (HANDLE)f;

  LARGE_INTEGER loffset;
  loffset.QuadPart = offset;

  if (seekMode == IOSeek::Relative)
    SetFilePointerEx(file, loffset, nullptr, FILE_CURRENT);
  else if (seekMode == IOSeek::Absolute)
    SetFilePointerEx(file, loffset, nullptr, FILE_BEGIN);
  else
    SetFilePointerEx(file, loffset, nullptr, FILE_END);
}

StreamOffset File::ftell(void* f) {
  HANDLE file = (HANDLE)f;
  LARGE_INTEGER pos;
  LARGE_INTEGER szero;
  szero.QuadPart = 0;
  SetFilePointerEx(file, szero, &pos, FILE_CURRENT);
  return pos.QuadPart;
}

size_t File::fread(void* f, char* data, size_t len) {
  if (len == 0)
    return 0;

  HANDLE file = (HANDLE)f;

  DWORD numRead = 0;
  int ret = ReadFile(file, data, len, &numRead, nullptr);
  if (ret == 0) {
    auto err = GetLastError();
    if (err != ERROR_IO_PENDING)
      throw IOException::format("read error {}", err);
  }

  return numRead;
}

size_t File::fwrite(void* f, char const* data, size_t len) {
  if (len == 0)
    return 0;

  HANDLE file = (HANDLE)f;

  DWORD numWritten = 0;
  int ret = WriteFile(file, data, len, &numWritten, nullptr);
  if (ret == 0) {
    auto err = GetLastError();
    if (err != ERROR_IO_PENDING)
      throw IOException::format("write error {}", err);
  }

  return numWritten;
}

void File::fsync(void* f) {
  HANDLE file = (HANDLE)f;
  if (FlushFileBuffers(file) == 0)
    throw IOException::format("fsync error {}", GetLastError());
}

void File::fclose(void* f) {
  HANDLE file = (HANDLE)f;
  CloseHandle(file);
}

StreamOffset File::fsize(void* f) {
  HANDLE file = (HANDLE)f;
  LARGE_INTEGER size;
  if (GetFileSizeEx(file, &size) == 0)
    throw IOException::format("could not get file size in fsize {}", GetLastError());
  return size.QuadPart;
}

size_t File::pread(void* f, char* data, size_t len, StreamOffset position) {
  HANDLE file = (HANDLE)f;
  DWORD numRead = 0;
  OVERLAPPED overlapped = makeOverlapped(position);
  int ret = ReadFile(file, data, len, &numRead, &overlapped);
  fseek(f, -(StreamOffset)numRead, IOSeek::Relative);
  if (ret == 0) {
    auto err = GetLastError();
    if (err != ERROR_IO_PENDING)
      throw IOException::format("pread error {}", err);
  }

  return numRead;
}

size_t File::pwrite(void* f, char const* data, size_t len, StreamOffset position) {
  HANDLE file = (HANDLE)f;
  DWORD numWritten = 0;
  OVERLAPPED overlapped = makeOverlapped(position);
  int ret = WriteFile(file, data, len, &numWritten, &overlapped);
  fseek(f, -(StreamOffset)numWritten, IOSeek::Relative);
  if (ret == 0) {
    auto err = GetLastError();
    if (err != ERROR_IO_PENDING)
      throw IOException::format("pwrite error {}", err);
  }

  return numWritten;
}

void File::resize(void* f, StreamOffset size) {
  HANDLE file = (HANDLE)f;
  LARGE_INTEGER s;
  s.QuadPart = size;
  SetFilePointerEx(file, s, NULL, 0);
  SetEndOfFile(file);
}

}

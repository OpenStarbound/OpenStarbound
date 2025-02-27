#pragma once

#include "StarIODevice.hpp"
#include "StarString.hpp"

namespace Star {

STAR_CLASS(File);

// All file methods are thread safe.
class File : public IODevice {
public:
  // Converts the passed in path to use the platform specific directory
  // separators only (Windows supports '/' just fine, this is mostly for
  // uniform appearance).  Does *nothing else* (no validity checks, etc).
  static String convertDirSeparators(String const& path);

  // All static file operations here throw IOException on error.
  // get the current working directory
  static String currentDirectory();
  // set the current working directory.
  static void changeDirectory(String const& dirName);
  static void makeDirectory(String const& dirName);
  static void makeDirectoryRecursive(String const& dirName);

  // List all files or directories under given directory.  skipDots skips the
  // special '.' and '..' entries.  Bool value is true for directories.
  static List<pair<String, bool>> dirList(String const& dirName, bool skipDots = true);

  // Returns the final component of the given path with no directory separators
  static String baseName(String const& fileName);
  // All components of the given path minus the final component, separated by
  // the directory separator
  static String dirName(String const& fileName);

  // Resolve a path relative to another path.  If the given path is absolute,
  // then the given path is returned unmodified.
  static String relativeTo(String const& relativeTo, String const& path);

  // Resolve the given possibly relative path into an absolute path.
  static String fullPath(String const& path);

  static String temporaryFileName();

  // Creates and opens a new ReadWrite temporary file with a real path that can
  // be closed and re-opened.  Will not be removed automatically.
  static FilePtr temporaryFile();

  // Creates and opens new ReadWrite temporary file and opens it.  This file
  // has no filename and will be removed on close.
  static FilePtr ephemeralFile();

  // Creates a new temporary directory and reutrns the path.  Will not be
  // removed automatically.
  static String temporaryDirectory();

  static bool exists(String const& path);

  // Does the file exist and is it a regular file (not a directory or special
  // file)?
  static bool isFile(String const& path);
  // Is the file a directory?
  static bool isDirectory(String const& path);

  static void remove(String const& filename);
  static void removeDirectoryRecursive(String const& filename);

  // Moves the source file to the target path, overwriting the target path if
  // it already exists.
  static void rename(String const& source, String const& target);

  // Copies the source file to the target, overwriting the target path if it
  // already exists.
  static void copy(String const& source, String const& target);

  static ByteArray readFile(String const& filename);
  static String readFileString(String const& filename);
  static StreamOffset fileSize(String const& filename);

  static void writeFile(char const* data, size_t len, String const& filename);
  static void writeFile(ByteArray const& data, String const& filename);
  static void writeFile(String const& data, String const& filename);

  // Write a new file, potentially overwriting an existing file, in the safest
  // way possible while preserving the old file in the same directory until the
  // operation completes.  Writes to the same path as the existing file to
  // avoid different partition copying.  This may clobber anything in the given
  // path that matches filename + newSuffix.
  static void overwriteFileWithRename(char const* data, size_t len, String const& filename, String const& newSuffix = ".new");
  static void overwriteFileWithRename(ByteArray const& data, String const& filename, String const& newSuffix = ".new");
  static void overwriteFileWithRename(String const& data, String const& filename, String const& newSuffix = ".new");

  static void backupFileInSequence(String const& initialFile, String const& targetFile, unsigned maximumBackups, String const& backupExtensionPrefix = ".");
  static void backupFileInSequence(String const& targetFile, unsigned maximumBackups, String const& backupExtensionPrefix = ".");

  static FilePtr open(String const& filename, IOMode mode);

  File();
  File(String filename);
  virtual ~File();

  String fileName() const;
  void setFilename(String filename);

  // File is closed before removal.
  void remove();

  StreamOffset pos() override;
  void seek(StreamOffset pos, IOSeek seek = IOSeek::Absolute) override;
  void resize(StreamOffset size) override;
  StreamOffset size() override;
  bool atEnd() override;
  size_t read(char* data, size_t len) override;
  size_t write(char const* data, size_t len) override;

  // Do an immediate read / write of an absolute location in the file, without
  // modifying the current file cursor.  Safe to call in a threaded context
  // with other reads and writes, but not safe vs changing the File state like
  // open and close.
  size_t readAbsolute(StreamOffset readPosition, char* data, size_t len) override;
  size_t writeAbsolute(StreamOffset writePosition, char const* data, size_t len) override;

  void open(IOMode mode) override;
  void close() override;

  void sync() override;

  String deviceName() const override;

  IODevicePtr clone() override;

private:
  static void* fopen(char const* filename, IOMode mode);
  static void fseek(void* file, StreamOffset offset, IOSeek seek);
  static StreamOffset ftell(void* file);
  static size_t fread(void* file, char* data, size_t len);
  static size_t fwrite(void* file, char const* data, size_t len);
  static void fsync(void* file);
  static void fclose(void* file);
  static StreamOffset fsize(void* file);
  static size_t pread(void* file, char* data, size_t len, StreamOffset absPosition);
  static size_t pwrite(void* file, char const* data, size_t len, StreamOffset absPosition);
  static void resize(void* file, StreamOffset size);

  String m_filename;
  void* m_file;
};

}

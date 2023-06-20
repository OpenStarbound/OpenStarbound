#include "StarFile.hpp"
#include "StarString.hpp"
#include "StarFormat.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(FileTest, All) {
  auto file = File::ephemeralFile();
  file->resize(1000);
  file->resize(0);
  file->resize(500);
  EXPECT_EQ(file->size(), 500);

  auto dir = File::temporaryDirectory();
  File::makeDirectory(File::relativeTo(dir, "inner"));
  EXPECT_TRUE(File::isDirectory(File::relativeTo(dir, "inner") + "/"));
  File::removeDirectoryRecursive(dir);

#ifdef STAR_SYSTEM_FAMILY_WINDOWS
  EXPECT_EQ(File::baseName("/foo/bar"), "bar");
  EXPECT_EQ(File::baseName("\\foo\\bar\\"), "bar");
  EXPECT_EQ(File::baseName("/foo/bar/baz"), "baz");
  EXPECT_EQ(File::dirName("\\foo\\bar"), "\\foo");
  EXPECT_EQ(File::dirName("/foo\\bar/"), "/foo");
  EXPECT_EQ(File::dirName("/foo/bar\\baz"), "/foo/bar");
  EXPECT_EQ(File::dirName("foo/bar/baz"), "foo/bar");

  EXPECT_EQ(File::relativeTo("c:\\foo\\", "bar"), "c:\\foo\\bar");
  EXPECT_EQ(File::relativeTo("c:\\foo", "bar"), "c:\\foo\\bar");
  EXPECT_EQ(File::relativeTo("c:\\foo\\", "\\bar"), "\\bar");
  EXPECT_EQ(File::relativeTo("c:\\foo\\", ".\\bar"), "c:\\foo\\bar");
  EXPECT_EQ(File::relativeTo("c:\\foo\\.", ".\\bar"), "c:\\foo\\bar");
  EXPECT_EQ(File::relativeTo("c:\\foo\\.", "c:\\bar"), "c:\\bar");
  EXPECT_EQ(File::relativeTo("c:\\foo\\.", "c:bar\\"), "c:bar\\");
  EXPECT_EQ(File::relativeTo("c:\\foo.", "bar"), "c:\\foo.\\bar");
#else
  EXPECT_EQ(File::baseName("/foo/bar"), "bar");
  EXPECT_EQ(File::baseName("/foo/bar/"), "bar");
  EXPECT_EQ(File::baseName("/foo/bar/baz"), "baz");
  EXPECT_EQ(File::dirName("/foo/bar"), "/foo");
  EXPECT_EQ(File::dirName("/foo/bar/"), "/foo");
  EXPECT_EQ(File::dirName("/foo/bar/baz"), "/foo/bar");
  EXPECT_EQ(File::dirName("foo/bar/baz"), "foo/bar");

  EXPECT_EQ(File::relativeTo("/foo", "bar"), "/foo/bar");
  EXPECT_EQ(File::relativeTo("/foo", "bar/"), "/foo/bar/");
  EXPECT_EQ(File::relativeTo("/foo", "/bar/"), "/bar/");
#endif
}

#include "StarDataStreamDevices.hpp"

#include "gtest/gtest.h"

using namespace Star;

template <typename T>
void testMap(T const& map) {
  auto byteArray = DataStreamBuffer::serializeMapContainer(map);
  auto mapOut = DataStreamBuffer::deserializeMapContainer<T>(byteArray);
  EXPECT_EQ(map, mapOut);
}

TEST(DataStreamTest, All) {
  Map<int, int> map1 = {
    {1, 2},
    {3, 4},
    {5, 6},
  };

  Map<String, int> map2 = {
    {"asdf", 1},
    {"asdf1", 2},
    {"omg", 2},
  };

  Map<String, int> map3 = {};

  testMap(map1);
  testMap(map2);
  testMap(map3);

  // Update this if String serialization ever changes.
  DataStreamBuffer ds;
  ds.setStreamCompatibilityVersion(20);
  std::string testStr = "\x80\x80";
  std::string outStdStr;
  String outStr;
  String sanitized = String(testStr).sanitize();

  ds << testStr.c_str();
  ds << testStr;
  ds << String(testStr);
  ds.seek(0);
  ds >> outStdStr;
  EXPECT_EQ(outStdStr, sanitized.utf8());
  ds >> outStdStr;
  EXPECT_EQ(outStdStr, sanitized.utf8());
  ds >> outStr;
  EXPECT_EQ(outStr, sanitized);

  ds.seek(0);
  ds.setStreamCompatibilityVersion(21);

  ds << testStr.c_str();
  ds << testStr;
  ds << String(testStr);
  ds.seek(0);
  ds >> outStdStr;
  EXPECT_EQ(outStdStr, testStr);
  ds >> outStdStr;
  EXPECT_EQ(outStdStr, testStr);
  ds >> outStr;
  EXPECT_EQ(outStr, String(testStr));
}

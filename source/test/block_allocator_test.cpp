#include "StarBlockAllocator.hpp"
#include "StarMap.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(BlockAllocatorTest, All) {
  HashMap<int, int, std::hash<int>, std::equal_to<int>, BlockAllocator<pair<int const, int>, 4096>> testMap;
  testMap.reserve(32768);
  for (size_t i = 0; i < 65536; ++i)
    testMap[i] = i;

  for (auto const& p : testMap)
    EXPECT_EQ(p.first, p.second);
}

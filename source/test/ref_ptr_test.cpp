#include "StarRefPtr.hpp"
#include "StarCasting.hpp"

#include "gtest/gtest.h"

#ifdef STAR_COMPILER_CLANG
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif 

using namespace Star;

int g_test1Count = 0;
int g_test2Count = 0;

struct Base : RefCounter {};

struct Test1 : Base {
  Test1() {
    ++g_test1Count;
  }

  ~Test1() {
    --g_test1Count;
  }
};

struct Test2 : Base {
  Test2() {
    ++g_test2Count;
  }

  ~Test2() {
    --g_test2Count;
  }
};

TEST(IntrusivePtr, All) {
  {
    RefPtr<Base> p1 = make_ref<Test1>();
    RefPtr<Base> p2 = make_ref<Test2>();

    EXPECT_TRUE(is<Test1>(p1));
    EXPECT_FALSE(is<Test2>(p1));
    EXPECT_TRUE(is<Test2>(p2));
    EXPECT_FALSE(is<Test1>(p2));

    RefPtr<Base> p3 = p1;
    RefPtr<Base> p4 = p2;

    p3 = p3;
    swap(p3, p4);

    EXPECT_TRUE(is<Test1>(p4));
    EXPECT_FALSE(is<Test2>(p4));
    EXPECT_TRUE(is<Test2>(p3));
    EXPECT_FALSE(is<Test1>(p3));

    EXPECT_EQ(p3, p2);
    EXPECT_EQ(p4, p1);

    swap(p3, p4);

    EXPECT_EQ(p3, p1);
    EXPECT_EQ(p4, p2);

    RefPtr<Base> p5;
    EXPECT_FALSE(p5);

    EXPECT_NE(p4, p1);
    EXPECT_NE(p3, p2);
    EXPECT_NE(p3, p5);

    p5 = p2;
    p2 = std::move(p5);
    EXPECT_TRUE(is<Test2>(p2));

    RefPtr<Test1> p6 = as<Test1>(p1);
    RefPtr<Test2> p7 = as<Test2>(p2);
    RefPtr<Test2> p8 = as<Test2>(p1);
    EXPECT_TRUE((bool)p6);
    EXPECT_TRUE((bool)p7);
    EXPECT_FALSE((bool)p8);
  }

  EXPECT_EQ(0, g_test1Count);
  EXPECT_EQ(0, g_test2Count);
}

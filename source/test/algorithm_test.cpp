#include "StarList.hpp"
#include "StarMultiArray.hpp"

#include <list>

#include "gtest/gtest.h"

using namespace Star;

TEST(any, allTests) {
  int a = 60;
  int asdf[] = {1, 2, 3, 4, 5, 6};

  EXPECT_TRUE(any(asdf, [&](int b) { return b < a; }));
  EXPECT_FALSE(any(asdf, [&](int b) { return b > a; }));
  EXPECT_TRUE(any(asdf, [&](int b) { return a % b == 0; }));

  bool b[] = {false, false, false, true};
  bool c[] = {false, false, false, false};
  bool d[] = {false, false, true, true};
  bool e[] = {true, true, true, true};
  int f[] = {0, 1, 0, 0, 0, 3};

  EXPECT_TRUE(any(b));
  EXPECT_FALSE(any(c));
  EXPECT_TRUE(any(d));
  EXPECT_TRUE(any(e));
  EXPECT_TRUE(any(f));
}

TEST(all, allTests) {
  int a = 60;
  int asdf[] = {1, 2, 3, 4, 5, 6};

  EXPECT_TRUE(all(asdf, [&](int b) { return b < a; }));
  EXPECT_FALSE(all(asdf, [&](int b) { return b > a; }));
  EXPECT_TRUE(all(asdf, [&](int b) { return a % b == 0; }));

  bool b[] = {false, false, false, true};
  bool c[] = {false, false, false, false};
  bool d[] = {false, false, true, true};
  bool e[] = {true, true, true, true};
  int f[] = {0, 1, 0, 0, 0, 3};

  EXPECT_FALSE(all(b));
  EXPECT_FALSE(all(c));
  EXPECT_FALSE(all(d));
  EXPECT_TRUE(all(e));
  EXPECT_FALSE(all(f));
}

TEST(ContainerOperators, allTests) {
  List<bool> a{false, false, true, false};
  List<int> b{1, 1, 0, 1};
  List<int> c = a.transformed([](bool a) { return a ? 0 : 1; });
  List<int> d{1, 2, 3, 5};
  List<int> e{1, 3, 5};
  List<int> f = d.filtered([](int i) { return i % 2 == 1; });

  EXPECT_TRUE(a.any());
  EXPECT_FALSE(a.all());
  EXPECT_EQ(b, c);
  EXPECT_EQ(e, f);
}

template <int Amount>
struct Times {
  template <typename T>
  T operator()(T t) {
    return t * Amount;
  }
};

template <int Amount>
struct Add {
  template <typename T>
  T operator()(T t) {
    return t + Amount;
  }
};

struct AddTogether {
  int operator()(int8_t& a, int64_t& b) {
    return a + b;
  }
};

TEST(TupleOperators, allTests) {
  tuple<int8_t, int64_t> t1{3, 5};
  tuple<int8_t, int64_t> t2{6, 10};

  auto t3 = tupleApplyFunction(Times<2>(), t1);
  EXPECT_EQ(t2, t3);

  int r = tupleUnpackFunction(AddTogether(), t2);
  EXPECT_EQ(r, 16);

  auto f = compose(Times<2>(), Add<1>(), Times<2>(), Add<3>());
  EXPECT_EQ(f(5), 34);
}

TEST(ZipTest, All) {
  List<int> a{1, 2, 3};
  std::vector<uint64_t> b{5, 4, 3, 2, 1};
  std::deque<int64_t> c{3, 2, 2};
  std::list<unsigned> d{0, 0, 0, 0, 4, 8};

  auto zipResult = zip(a, b, c, d);

  EXPECT_EQ(zipResult.size(), 3u);
  EXPECT_EQ(get<0>(zipResult[0]), 1);
  EXPECT_EQ(get<1>(zipResult[0]), 5u);
  EXPECT_EQ(get<2>(zipResult[0]), 3);
  EXPECT_EQ(get<3>(zipResult[0]), 0u);
  EXPECT_EQ(get<0>(zipResult[2]), 3);
  EXPECT_EQ(get<1>(zipResult[2]), 3u);
  EXPECT_EQ(get<2>(zipResult[2]), 2);
  EXPECT_EQ(get<3>(zipResult[2]), 0u);
}

TEST(ZipWith, All) {
  List<int> a{1, 1, 2, 3, 5, 8};
  List<int> b{5, 4, 3, 2, 1, 0};
  List<int> c{6, 5, 5, 5, 6, 8};
  List<int> d = zipWith<List<int>>(std::plus<int>(), a, b);
  EXPECT_EQ(c, d);
}

TEST(TupleFunctions, All) {
  std::vector<int> a;
  std::vector<int> b = {1, 2, 3, 4, 5, 6, 7, 8};
  tupleCallFunction(make_tuple(1, 2, 3, 4, 5, 6, 7, 8), [&a](int i) { a.push_back(i); });

  EXPECT_EQ(a, b);
}

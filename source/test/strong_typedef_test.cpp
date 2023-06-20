#include "StarStrongTypedef.hpp"

#include "gtest/gtest.h"

struct BaseType {};
strong_typedef(BaseType, DerivedType1);
strong_typedef(BaseType, DerivedType2);
void func(DerivedType1) {}

strong_typedef_builtin(int, AlsoInt);

TEST(StrongTypedefTest, All) {
  AlsoInt i = AlsoInt(0);
  ++i;
  i -= 5;
  EXPECT_EQ(i, -4);

  func((DerivedType1)BaseType());
  func(DerivedType1());

  // Shouldn't compile!  Can't test this automatically!
  // func(BaseType());
  // func(DerivedType2());
  // DerivedType1 dt1 = Basetype();
  // DerivedType2 dt2 = DerivedType1();
}

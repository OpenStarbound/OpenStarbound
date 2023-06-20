#include "StarThread.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(Thread, InvokeErrors) {
  struct TestException {};

  auto function = Thread::invoke("test", []() {
      throw TestException();
    });

  EXPECT_THROW(function.finish(), TestException);
}

TEST(Thread, InvokeReturn) {
  auto functionRet = Thread::invoke("test", []() {
      return String("TestValue");
    });

  EXPECT_EQ(functionRet.finish(), String("TestValue"));
  EXPECT_THROW(functionRet.finish(), InvalidMaybeAccessException);
}

TEST(Thread, ReadersWriterMutex) {
  ReadersWriterMutex mutex;
  ReadLocker rl1(mutex);
  ReadLocker rl2(mutex);
  WriteLocker wl(mutex, false);
  EXPECT_FALSE(wl.tryLock());
  rl1.unlock();
  EXPECT_FALSE(wl.tryLock());
  rl2.unlock();
  EXPECT_TRUE(wl.tryLock());
}

#include "StarWorkerPool.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(WorkerPoolTest, All) {
  int counter = 0;
  Mutex counterMutex;

  auto incCounter = [&counter, &counterMutex]() {
    Thread::sleep(100);
    MutexLocker locker(counterMutex);
    counter += 1;
  };

  Deque<WorkerPoolHandle> handles;

  WorkerPool workerPool("WorkerPoolTest");
  for (size_t i = 0; i < 10; ++i)
    handles.append(workerPool.addWork(incCounter));

  workerPool.start(10);

  for (size_t i = 0; i < 90; ++i)
    handles.append(workerPool.addWork(incCounter));

  while (handles.size() > 20)
    handles.takeFirst().finish();

  workerPool.finish();

  EXPECT_EQ(counter, 100);
}

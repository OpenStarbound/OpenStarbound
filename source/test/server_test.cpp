#include "StarUniverseServer.hpp"
#include "StarRoot.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(ServerTest, Run) {
  UniverseServer server(Root::singleton().toStoragePath("universe"));
  server.start();
  server.stop();
  server.join();
}

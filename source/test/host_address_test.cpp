#include "StarHostAddress.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(HostAddress, All) {
  EXPECT_TRUE(HostAddress::localhost(NetworkMode::IPv4).isLocalHost());
  EXPECT_TRUE(HostAddress::localhost(NetworkMode::IPv6).isLocalHost());
  EXPECT_TRUE(HostAddress("*").isZero());
  EXPECT_TRUE(HostAddress("::").isZero());
  EXPECT_TRUE(HostAddress("127.0.0.1").isLocalHost());
  EXPECT_TRUE(HostAddress("::1").isLocalHost());
  EXPECT_EQ(HostAddress("*").mode(), NetworkMode::IPv4);
  EXPECT_EQ(HostAddress("::").mode(), NetworkMode::IPv6);
}

TEST(HostAddressWithPort, All) {
  EXPECT_EQ(HostAddressWithPort("*:80").port(), 80);
  EXPECT_EQ(HostAddressWithPort(":::80").port(), 80);
  EXPECT_EQ(HostAddressWithPort("[::]:80").port(), 80);
  EXPECT_TRUE(HostAddressWithPort("[::]:80").address().isZero());
  EXPECT_TRUE(HostAddressWithPort("[::1]:80").address().isLocalHost());
}

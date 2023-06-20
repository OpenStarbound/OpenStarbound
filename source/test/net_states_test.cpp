#include "StarNetElementSystem.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(NetElements, DataRounding) {
  NetElementFloat masterField1;
  NetElementFloat masterField2;

  NetElementTop<NetElementGroup> master;
  master.addNetElement(&masterField1);
  master.addNetElement(&masterField2);

  masterField1.setFixedPointBase(0.1f);
  masterField2.setFixedPointBase(0.5);

  masterField1.set(2.1999f);
  masterField2.set(100.04);

  // Check to make sure encoded data is actually sent with the expected
  // limitations to the client side.

  NetElementFloat slaveField1;
  NetElementFloat slaveField2;

  NetElementTop<NetElementGroup> slave;
  slave.addNetElement(&slaveField1);
  slave.addNetElement(&slaveField2);

  slaveField1.setFixedPointBase(0.1f);
  slaveField2.setFixedPointBase(0.5);

  auto masterUpdate1 = master.writeNetState();
  slave.readNetState(masterUpdate1.first);

  EXPECT_LT(fabs(slaveField1.get() - 2.2f), 0.00001f);
  EXPECT_LT(fabs(slaveField2.get() - 100), 0.0000001);

  // Make sure that jittering a fixed point or limited value doesn't cause
  // extra deltas

  masterField1.set(2.155f);
  masterField1.set(2.24f);
  masterField2.set(99.96);
  masterField2.set(100.00);

  auto masterUpdate2 = master.writeNetState(masterUpdate1.second);
  EXPECT_TRUE(masterUpdate2.first.empty());
  slave.readNetState(masterUpdate2.first);

  masterField1.set(10.0f);
  masterField2.set(50.0f);

  auto masterUpdate3 = master.writeNetState(masterUpdate2.second);
  EXPECT_FALSE(masterUpdate3.first.empty());
  slave.readNetState(masterUpdate3.first);

  EXPECT_LT(fabs(slaveField1.get() - 10.0f), 0.00001f);
  EXPECT_LT(fabs(slaveField2.get() - 50.0f), 0.0000001);
}

TEST(NetElements, DirectWriteRead) {
  NetElementUInt masterField1;
  NetElementUInt masterField2;
  NetElementUInt masterField3;
  NetElementUInt masterField4;

  NetElementTop<NetElementGroup> master;
  master.addNetElement(&masterField1);
  master.addNetElement(&masterField2);
  master.addNetElement(&masterField3);
  master.addNetElement(&masterField4);

  masterField1.set(1);
  masterField2.set(2);
  masterField3.set(3);
  masterField4.set(4);

  NetElementUInt slaveField1;
  NetElementUInt slaveField2;
  NetElementUInt slaveField3;
  NetElementUInt slaveField4;

  NetElementTop<NetElementGroup> slave;
  slave.addNetElement(&slaveField1);
  slave.addNetElement(&slaveField2);
  slave.addNetElement(&slaveField3);
  slave.addNetElement(&slaveField4);

  auto masterUpdate1 = master.writeNetState();
  slave.readNetState(masterUpdate1.first);

  EXPECT_EQ(slaveField1.get(), 1u);
  EXPECT_EQ(slaveField2.get(), 2u);
  EXPECT_EQ(slaveField3.get(), 3u);
  EXPECT_EQ(slaveField4.get(), 4u);

  masterField1.set(10);
  masterField3.set(30);

  auto masterUpdate2 = master.writeNetState(masterUpdate1.second);
  slave.readNetState(masterUpdate2.first);

  EXPECT_EQ(slaveField1.get(), 10u);
  EXPECT_EQ(slaveField2.get(), 2u);
  EXPECT_EQ(slaveField3.get(), 30u);
  EXPECT_EQ(slaveField4.get(), 4u);

  masterField2.set(20);
  masterField4.set(40);

  auto masterUpdate3 = master.writeNetState(masterUpdate2.second);
  slave.readNetState(masterUpdate3.first);

  EXPECT_EQ(slaveField1.get(), 10u);
  EXPECT_EQ(slaveField2.get(), 20u);
  EXPECT_EQ(slaveField3.get(), 30u);
  EXPECT_EQ(slaveField4.get(), 40u);
}

TEST(NetElements, DeltaSize) {
  NetElementInt masterField1;
  NetElementUInt masterField2;
  NetElementUInt masterField3;
  NetElementSize masterField4;

  NetElementTop<NetElementGroup> master;
  master.addNetElement(&masterField1);
  master.addNetElement(&masterField2);
  master.addNetElement(&masterField3);
  master.addNetElement(&masterField4);

  NetElementInt slaveField1;
  NetElementUInt slaveField2;
  NetElementUInt slaveField3;
  NetElementSize slaveField4;

  NetElementTop<NetElementGroup> slave;
  slave.addNetElement(&slaveField1);
  slave.addNetElement(&slaveField2);
  slave.addNetElement(&slaveField3);
  slave.addNetElement(&slaveField4);

  masterField1.set(10);
  masterField2.set(20);
  masterField3.set(30);
  masterField4.set(40);

  EXPECT_EQ(masterField1.get(), 10);
  EXPECT_EQ(masterField2.get(), 20u);
  EXPECT_EQ(masterField3.get(), 30u);
  EXPECT_EQ(masterField4.get(), 40u);

  auto masterUpdate1 = master.writeNetState();

  // Initial state should be 5 bytes, 1 byte for header, then 4 1 byte values.
  EXPECT_EQ(masterUpdate1.first.size(), 5u);

  slave.readNetState(masterUpdate1.first);
  EXPECT_EQ(slaveField1.get(), 10);
  EXPECT_EQ(slaveField2.get(), 20u);
  EXPECT_EQ(slaveField3.get(), 30u);
  EXPECT_EQ(slaveField4.get(), 40u);

  masterField1.set(50);
  auto masterUpdate2 = master.writeNetState(masterUpdate1.second);

  // Second delta should be not include any other data than the single 1 byte
  // changed state, so make sure that it is 1 byte for header, 1 byte for field
  // number, 1 byte for state, 1 byte for end marker.
  EXPECT_EQ(masterUpdate2.first.size(), 4u);

  slave.readNetState(masterUpdate2.first);
  EXPECT_EQ(slaveField1.get(), 50);
}

TEST(NetElements, Forwarding) {
  NetElementInt masterField1;
  NetElementData<String> masterField2;

  NetElementTop<NetElementGroup> master;
  master.addNetElement(&masterField1);
  master.addNetElement(&masterField2);

  NetElementInt forwarderField1;
  NetElementData<String> forwarderField2;

  NetElementTop<NetElementGroup> forwarder;
  forwarder.addNetElement(&forwarderField1);
  forwarder.addNetElement(&forwarderField2);

  NetElementInt slaveField1;
  NetElementData<String> slaveField2;

  NetElementTop<NetElementGroup> slave;
  slave.addNetElement(&slaveField1);
  slave.addNetElement(&slaveField2);

  masterField1.set(413);
  masterField2.set("foo");

  auto masterUpdate1 = master.writeNetState();
  forwarder.readNetState(masterUpdate1.first);

  auto forwarderUpdate1 = forwarder.writeNetState();
  slave.readNetState(forwarderUpdate1.first);

  EXPECT_EQ(forwarderField1.get(), 413);
  EXPECT_EQ(forwarderField2.get(), "foo");
  EXPECT_EQ(slaveField1.get(), 413);
  EXPECT_EQ(slaveField2.get(), "foo");
}

TEST(NetElements, StepForwarding) {
  NetElementInt masterField1;
  NetElementInt masterField2;

  NetElementTop<NetElementGroup> master;
  master.addNetElement(&masterField1);
  master.addNetElement(&masterField2);

  NetElementInt forwarderField1;
  NetElementInt forwarderField2;

  NetElementTop<NetElementGroup> forwarder;
  forwarder.addNetElement(&forwarderField1);
  forwarder.addNetElement(&forwarderField2);

  NetElementInt slaveField1;
  NetElementInt slaveField2;

  NetElementTop<NetElementGroup> slave;
  slave.addNetElement(&slaveField1);
  slave.addNetElement(&slaveField2);

  // First emulate store / load that would happen in entity initialization.

  masterField1.set(10);
  masterField2.set(20);

  auto masterUpdate1 = master.writeNetState();
  forwarder.readNetState(masterUpdate1.first);

  auto forwarderUpdate1 = forwarder.writeNetState();
  slave.readNetState(forwarderUpdate1.first);

  EXPECT_EQ(forwarderField1.get(), 10);
  EXPECT_EQ(forwarderField2.get(), 20);

  EXPECT_EQ(slaveField1.get(), 10);
  EXPECT_EQ(slaveField2.get(), 20);

  // Then, update one field and transmit that delta[ to the forwarder.

  masterField1.set(413);

  auto masterUpdate2 = master.writeNetState(masterUpdate1.second);
  forwarder.readNetState(masterUpdate2.first);
  EXPECT_EQ(forwarderField1.get(), 413);

  auto forwarderUpdate2 = forwarder.writeNetState(forwarderUpdate1.second);
  slave.readNetState(forwarderUpdate2.first);
  EXPECT_EQ(slaveField1.get(), 413);

  EXPECT_EQ(forwarderField1.get(), 413);
  EXPECT_EQ(forwarderField2.get(), 20);

  EXPECT_EQ(slaveField1.get(), 413);
  EXPECT_EQ(slaveField2.get(), 20);
}

TEST(NetElements, InterpolationForwarding) {
  NetElementInt masterField1;
  NetElementFloat masterField2;
  NetElementData<String> masterField3;

  NetElementTop<NetElementGroup> master;
  master.addNetElement(&masterField1);
  master.addNetElement(&masterField2);
  master.addNetElement(&masterField3);

  NetElementInt forwarderField1;
  NetElementFloat forwarderField2;
  NetElementData<String> forwarderField3;

  NetElementTop<NetElementGroup> forwarder;
  forwarder.addNetElement(&forwarderField1);
  forwarder.addNetElement(&forwarderField2);
  forwarder.addNetElement(&forwarderField3);

  NetElementInt slaveField1;
  NetElementFloat slaveField2;
  NetElementData<String> slaveField3;

  NetElementTop<NetElementGroup> slave;
  slave.addNetElement(&slaveField1);
  slave.addNetElement(&slaveField2);
  slave.addNetElement(&slaveField3);

  forwarder.enableNetInterpolation();
  slave.enableNetInterpolation();

  masterField1.set(10);
  masterField2.set(10.0f);
  masterField3.set("10");

  auto masterUpdate1 = master.writeNetState();
  forwarder.readNetState(masterUpdate1.first);

  auto forwarderUpdate1 = forwarder.writeNetState();
  slave.readNetState(forwarderUpdate1.first);

  EXPECT_EQ(forwarderField1.get(), 10);
  EXPECT_EQ(forwarderField2.get(), 10.0f);
  EXPECT_EQ(forwarderField3.get(), "10");

  EXPECT_EQ(slaveField1.get(), 10);
  EXPECT_EQ(slaveField2.get(), 10.0f);
  EXPECT_EQ(slaveField3.get(), "10");

  masterField1.set(20);
  masterField2.set(20.0f);
  masterField3.set("20");

  auto masterUpdate2 = master.writeNetState(masterUpdate1.second);
  forwarder.readNetState(masterUpdate2.first, 1.0f);

  // Forwarder should not be updated yet, still 1.0f interpolationTime behind
  EXPECT_EQ(forwarderField1.get(), 10);
  EXPECT_EQ(forwarderField2.get(), 10.0f);
  EXPECT_EQ(forwarderField3.get(), "10");

  // But the forwarder should STILL forward the absolute latest data to the
  // slave
  auto forwarderUpdate2 = forwarder.writeNetState(forwarderUpdate1.second);
  slave.readNetState(forwarderUpdate2.first, 1.0f);

  // Slave should not be updated yet, still 1.0f interpolationTime behind
  EXPECT_EQ(slaveField1.get(), 10);
  EXPECT_EQ(slaveField2.get(), 10.0f);
  EXPECT_EQ(slaveField3.get(), "10");

  // After ticking forward interpolation, both the forwarder and the slave
  // should both pick up the new values.

  forwarder.tickNetInterpolation(1.0f);
  EXPECT_EQ(forwarderField1.get(), 20);
  EXPECT_EQ(forwarderField2.get(), 20.0f);
  EXPECT_EQ(forwarderField3.get(), "20");

  slave.tickNetInterpolation(1.0f);
  EXPECT_EQ(slaveField1.get(), 20);
  EXPECT_EQ(slaveField2.get(), 20.0f);
  EXPECT_EQ(slaveField3.get(), "20");
}

TEST(NetElements, MasterSetGet) {
  // Make sure that Master mode sets, gets, and pullEventOccurred work
  // properly.

  NetElementInt masterField1;
  NetElementData<String> masterField2;
  NetElementEvent masterField3;

  NetElementTop<NetElementGroup> master;
  master.addNetElement(&masterField1);
  master.addNetElement(&masterField2);
  master.addNetElement(&masterField3);

  EXPECT_EQ(masterField3.pullOccurrences(), 0u);

  masterField1.set(10);
  masterField2.set("foo");

  EXPECT_EQ(masterField1.get(), 10);
  EXPECT_EQ(masterField2.get(), "foo");

  masterField3.trigger();
  masterField3.trigger();
  EXPECT_EQ(masterField3.pullOccurrences(), 2u);
  EXPECT_EQ(masterField3.pullOccurrences(), 0u);
}

TEST(NetElements, IsolatedSetGet) {
  // Make sure fields work without being connected

  NetElementInt masterField1;
  NetElementData<String> masterField2;
  NetElementEvent masterField3;

  EXPECT_EQ(masterField2.pullUpdated(), false);
  EXPECT_EQ(masterField3.pullOccurrences(), 0u);

  masterField1.set(10);
  masterField2.set("foo");

  EXPECT_EQ(masterField2.pullUpdated(), true);
  EXPECT_EQ(masterField2.pullUpdated(), false);

  EXPECT_EQ(masterField1.get(), 10);
  EXPECT_EQ(masterField2.get(), "foo");

  masterField3.trigger();
  masterField3.trigger();
  EXPECT_EQ(masterField3.pullOccurrences(), 2u);
  EXPECT_EQ(masterField3.pullOccurrences(), 0u);

  masterField1.set(20);
  masterField2.set("bar");

  masterField3.trigger();
  masterField3.trigger();
  EXPECT_EQ(masterField3.pullOccurrences(), 2u);
}

TEST(NetElements, EventTest) {
  NetElementData<String> masterField1;
  NetElementEvent masterField2;

  NetElementTop<NetElementGroup> master;
  master.addNetElement(&masterField1);
  master.addNetElement(&masterField2);

  NetElementData<String> slaveField1;
  NetElementEvent slaveField2;

  NetElementTop<NetElementGroup> slave;
  slave.addNetElement(&slaveField1);
  slave.addNetElement(&slaveField2);

  // Events should always start with occurred false.
  EXPECT_EQ(slaveField1.pullUpdated(), false);
  EXPECT_EQ(slaveField2.pullOccurrences(), 0u);

  masterField1.set("foo");
  masterField2.trigger();
  EXPECT_TRUE(masterField1.pullUpdated());
  EXPECT_FALSE(masterField1.pullUpdated());

  auto masterUpdate1 = master.writeNetState();
  slave.readNetState(masterUpdate1.first);

  EXPECT_TRUE(slaveField1.pullUpdated());
  EXPECT_FALSE(slaveField1.pullUpdated());
  EXPECT_EQ(slaveField2.pullOccurrences(), 1u);
  EXPECT_EQ(slaveField2.pullOccurrences(), 0u);

  // Delta should be empty, nothing happened on step 1.
  auto masterUpdate2 = master.writeNetState(masterUpdate1.second);
  EXPECT_TRUE(masterUpdate2.first.empty());
  slave.readNetState(masterUpdate2.first);

  EXPECT_EQ(slaveField2.pullOccurrences(), 0u);

  masterField2.trigger();

  auto masterUpdate3 = master.writeNetState(masterUpdate2.second);
  EXPECT_FALSE(masterUpdate3.first.empty());
  slave.readNetState(masterUpdate3.first);

  EXPECT_EQ(slaveField2.pullOccurrences(), 1u);
  EXPECT_EQ(slaveField2.pullOccurrences(), 0u);
}

TEST(NetElements, FieldUpdated) {
  NetElementData<String> masterField1;
  NetElementInt masterField2;
  NetElementEvent masterField3;
  NetElementEvent masterField4;

  NetElementTop<NetElementGroup> master;
  master.addNetElement(&masterField1);
  master.addNetElement(&masterField2);
  master.addNetElement(&masterField3);
  master.addNetElement(&masterField4);

  NetElementData<String> slaveField1;
  NetElementInt slaveField2;
  NetElementEvent slaveField3;
  NetElementEvent slaveField4;

  NetElementTop<NetElementGroup> slave;
  slave.addNetElement(&slaveField1);
  slave.addNetElement(&slaveField2);
  slave.addNetElement(&slaveField3);
  slave.addNetElement(&slaveField4);

  slaveField4.setIgnoreOccurrencesOnNetLoad(true);

  masterField1.set("foo");

  masterField3.trigger();
  masterField4.trigger();

  auto masterUpdate1 = master.writeNetState();
  slave.readNetState(masterUpdate1.first);

  EXPECT_EQ(slaveField1.pullUpdated(), true);
  EXPECT_EQ(slaveField1.get(), "foo");
  EXPECT_EQ(slaveField1.pullUpdated(), false);

  EXPECT_EQ(masterField3.pullOccurrences(), 1u);
  EXPECT_EQ(slaveField3.pullOccurrences(), 1u);

  // Ignore occurrences on full load should stop slaveField4 from getting any
  // occurrences
  EXPECT_EQ(masterField4.pullOccurrences(), 1u);
  EXPECT_EQ(slaveField4.pullOccurrences(), 0u);

  masterField1.set("baz");

  auto masterUpdate2 = master.writeNetState(masterUpdate1.second);
  EXPECT_FALSE(masterUpdate2.first.empty());
  slave.readNetState(masterUpdate2.first);

  EXPECT_EQ(slaveField1.get(), "baz");
  EXPECT_EQ(slaveField1.pullUpdated(), true);

  masterField1.set("bar");

  auto masterUpdate3 = master.writeNetState(masterUpdate2.second);
  EXPECT_FALSE(masterUpdate3.first.empty());
  slave.readNetState(masterUpdate3.first);

  EXPECT_EQ(slaveField1.get(), "bar");
  EXPECT_EQ(slaveField1.pullUpdated(), true);

  masterField1.push("bar");

  auto masterUpdate4 = master.writeNetState(masterUpdate3.second);
  EXPECT_FALSE(masterUpdate4.first.empty());
  slave.readNetState(masterUpdate4.first);

  EXPECT_EQ(slaveField1.get(), "bar");
  EXPECT_EQ(slaveField1.pullUpdated(), true);

  auto masterUpdate5 = master.writeNetState(masterUpdate4.second);
  EXPECT_TRUE(masterUpdate5.first.empty());
  slave.readNetState(masterUpdate5.first);

  EXPECT_EQ(slaveField1.get(), "bar");
  EXPECT_EQ(slaveField1.pullUpdated(), false);

  masterField3.trigger();
  masterField3.trigger();

  auto masterUpdate6 = master.writeNetState(masterUpdate5.second);
  EXPECT_FALSE(masterUpdate6.first.empty());
  slave.readNetState(masterUpdate6.first);

  slaveField3.ignoreOccurrences();
  // occurrence should not come through after "ignoreOccurrences"
  EXPECT_EQ(slaveField3.pullOccurrences(), 0u);
  EXPECT_EQ(masterField3.pullOccurrences(), 2u);

  masterField3.trigger();
  masterField3.trigger();
  masterField3.ignoreOccurrences();

  auto masterUpdate7 = master.writeNetState(masterUpdate6.second);
  EXPECT_FALSE(masterUpdate7.first.empty());
  slave.readNetState(masterUpdate7.first);

  // ignoreOccurrences is LOCAL only, so events should still go through to the
  // slave
  EXPECT_EQ(masterField3.pullOccurrences(), 0u);
  EXPECT_EQ(slaveField3.pullOccurrences(), 2u);
  EXPECT_EQ(slaveField3.pullOccurrences(), 0u);
}

TEST(NetElements, Interpolation) {
  NetElementFloat masterField1;
  NetElementData<String> masterField2;

  NetElementTop<NetElementGroup> master;
  master.addNetElement(&masterField1);
  master.addNetElement(&masterField2);

  masterField1.set(1.0f);
  masterField2.set("yes");

  NetElementFloat slaveField1;
  NetElementData<String> slaveField2;

  NetElementTop<NetElementGroup> slave;
  slave.addNetElement(&slaveField1);
  slave.addNetElement(&slaveField2);

  slaveField1.setInterpolator(lerp<double, double>);

  slave.enableNetInterpolation();

  auto masterUpdate1 = master.writeNetState();
  slave.readNetState(masterUpdate1.first);

  masterField1.set(2.0f);
  masterField2.set("no");
  auto masterUpdate2 = master.writeNetState(masterUpdate1.second);
  slave.readNetState(masterUpdate2.first, 2.0);

  masterField1.set(3.0f);
  masterField2.set("yes");
  auto masterUpdate3 = master.writeNetState(masterUpdate2.second);
  slave.readNetState(masterUpdate3.first, 4.0);

  masterField1.set(4.0f);
  masterField2.set("no");
  auto masterUpdate4 = master.writeNetState(masterUpdate3.second);
  slave.readNetState(masterUpdate4.first, 6.0);

  EXPECT_TRUE(fabs(slaveField1.get() - 1.0) < 0.001);
  EXPECT_EQ(slaveField2.get(), "yes");
  EXPECT_EQ(slaveField2.pullUpdated(), true);

  slave.tickNetInterpolation(1.0f);
  EXPECT_TRUE(fabs(slaveField1.get() - 1.5) < 0.001);
  EXPECT_EQ(slaveField2.get(), "yes");
  EXPECT_EQ(slaveField2.pullUpdated(), false);

  slave.tickNetInterpolation(1.0f);
  EXPECT_TRUE(fabs(slaveField1.get() - 2.0) < 0.001);
  EXPECT_EQ(slaveField2.get(), "no");
  EXPECT_EQ(slaveField2.pullUpdated(), true);

  slave.tickNetInterpolation(1.0f);
  EXPECT_TRUE(fabs(slaveField1.get() - 2.5) < 0.001);
  EXPECT_EQ(slaveField2.get(), "no");
  EXPECT_EQ(slaveField2.pullUpdated(), false);

  slave.tickNetInterpolation(1.0f);
  EXPECT_TRUE(fabs(slaveField1.get() - 3.0) < 0.001);
  EXPECT_EQ(slaveField2.get(), "yes");
  EXPECT_EQ(slaveField2.pullUpdated(), true);

  slave.tickNetInterpolation(1.0f);
  EXPECT_TRUE(fabs(slaveField1.get() - 3.5) < 0.001);
  EXPECT_EQ(slaveField2.get(), "yes");
  EXPECT_EQ(slaveField2.pullUpdated(), false);

  slave.tickNetInterpolation(1.0f);
  EXPECT_TRUE(fabs(slaveField1.get() - 4.0) < 0.001);
  EXPECT_EQ(slaveField2.pullUpdated(), true);
  EXPECT_EQ(slaveField2.get(), "no");
}

enum class TestEnum {
  Value1,
  Value2,
  Value3
};

TEST(NetElements, AllTypes) {
  NetElementInt masterField1;
  NetElementUInt masterField2;
  NetElementSize masterField3;
  NetElementFloat masterField4;
  NetElementDouble masterField5;
  NetElementFloat masterField6;
  masterField6.setFixedPointBase(0.01f);
  NetElementDouble masterField7;
  masterField7.setFixedPointBase(0.01);
  NetElementBool masterField8;
  NetElementEnum<TestEnum> masterField9;
  NetElementEvent masterField10;
  NetElementData<Vec2F> masterField11;

  NetElementTop<NetElementGroup> master;
  master.addNetElement(&masterField1);
  master.addNetElement(&masterField2);
  master.addNetElement(&masterField3);
  master.addNetElement(&masterField4);
  master.addNetElement(&masterField5);
  master.addNetElement(&masterField6);
  master.addNetElement(&masterField7);
  master.addNetElement(&masterField8);
  master.addNetElement(&masterField9);
  master.addNetElement(&masterField10);
  master.addNetElement(&masterField11);

  masterField1.set(567);
  masterField2.set(17000);
  masterField3.set(22222);
  masterField4.set(1.55f);
  masterField5.set(1.12345678910111213);
  masterField6.set(2000.62f);
  masterField7.set(2000.62);
  masterField8.set(true);
  masterField9.set(TestEnum::Value2);
  masterField10.trigger();
  masterField11.set(Vec2F(2.0f, 2.0f));

  EXPECT_EQ(masterField1.get(), 567);
  EXPECT_EQ(masterField2.get(), 17000u);
  EXPECT_EQ(masterField3.get(), 22222u);
  EXPECT_FLOAT_EQ(masterField4.get(), 1.55f);
  EXPECT_FLOAT_EQ(masterField5.get(), 1.12345678910111213);
  EXPECT_FLOAT_EQ(masterField6.get(), 2000.62f);
  EXPECT_FLOAT_EQ(masterField7.get(), 2000.62);
  EXPECT_EQ(masterField8.get(), true);
  EXPECT_EQ(masterField9.get(), TestEnum::Value2);
  EXPECT_TRUE(masterField10.pullOccurred());
  EXPECT_TRUE(masterField11.get() == Vec2F(2.0f, 2.0f));

  NetElementInt slaveField1;
  NetElementUInt slaveField2;
  NetElementSize slaveField3;
  NetElementFloat slaveField4;
  NetElementDouble slaveField5;
  NetElementFloat slaveField6;
  slaveField6.setFixedPointBase(0.01f);
  NetElementDouble slaveField7;
  slaveField7.setFixedPointBase(0.01);
  NetElementBool slaveField8;
  NetElementEnum<TestEnum> slaveField9;
  NetElementEvent slaveField10;
  NetElementData<Vec2F> slaveField11;

  NetElementTop<NetElementGroup> slave;
  slave.addNetElement(&slaveField1);
  slave.addNetElement(&slaveField2);
  slave.addNetElement(&slaveField3);
  slave.addNetElement(&slaveField4);
  slave.addNetElement(&slaveField5);
  slave.addNetElement(&slaveField6);
  slave.addNetElement(&slaveField7);
  slave.addNetElement(&slaveField8);
  slave.addNetElement(&slaveField9);
  slave.addNetElement(&slaveField10);
  slave.addNetElement(&slaveField11);

  slave.readNetState(master.writeNetState().first);

  EXPECT_EQ(slaveField1.get(), 567);
  EXPECT_EQ(slaveField2.get(), 17000u);
  EXPECT_EQ(slaveField3.get(), 22222u);
  EXPECT_FLOAT_EQ(slaveField4.get(), 1.55f);
  EXPECT_FLOAT_EQ(slaveField5.get(), 1.12345678910111213);
  EXPECT_FLOAT_EQ(slaveField6.get(), 2000.62f);
  EXPECT_FLOAT_EQ(slaveField7.get(), 2000.62);
  EXPECT_EQ(slaveField8.get(), true);
  EXPECT_EQ(slaveField9.get(), TestEnum::Value2);
  EXPECT_TRUE(slaveField10.pullOccurred());
  EXPECT_TRUE(slaveField11.get() == Vec2F(2.0f, 2.0f));
}

TEST(NetElements, NetElementDynamicGroup) {
  class TestElement : public NetElementGroup {
  public:
    TestElement(int value = 0) {
      addNetElement(&dataState);
      dataState.set(value);
    }

    void setData(int value) {
      dataState.set(value);
    }

    int getData() const {
      return dataState.get();
    }

  private:
    NetElementInt dataState;
  };

  NetElementTop<NetElementDynamicGroup<TestElement>> masterGroup;

  NetElementTop<NetElementDynamicGroup<TestElement>> slaveGroup;
  uint64_t lastSlaveUpdateVersion = 0;
  auto sendSlaveUpdate = [&]() {
    auto p = masterGroup.writeNetState(lastSlaveUpdateVersion);
    lastSlaveUpdateVersion = p.second;
    slaveGroup.readNetState(p.first);
  };

  auto objId1 = masterGroup.addNetElement(make_shared<TestElement>(1000));
  auto objId2 = masterGroup.addNetElement(make_shared<TestElement>(2000));

  EXPECT_EQ(masterGroup.netElementIds().size(), 2u);
  EXPECT_EQ(masterGroup.getNetElement(objId1)->getData(), 1000);
  EXPECT_EQ(masterGroup.getNetElement(objId2)->getData(), 2000);

  sendSlaveUpdate();

  EXPECT_EQ(slaveGroup.netElementIds().size(), 2u);
  EXPECT_EQ(slaveGroup.getNetElement(objId1)->getData(), 1000);
  EXPECT_EQ(slaveGroup.getNetElement(objId2)->getData(), 2000);

  masterGroup.getNetElement(objId1)->setData(1001);
  masterGroup.getNetElement(objId2)->setData(2001);

  sendSlaveUpdate();

  EXPECT_EQ(slaveGroup.getNetElement(objId1)->getData(), 1001);
  EXPECT_EQ(slaveGroup.getNetElement(objId2)->getData(), 2001);

  masterGroup.getNetElement(objId1)->setData(1002);
  masterGroup.getNetElement(objId2)->setData(2002);

  sendSlaveUpdate();

  EXPECT_EQ(masterGroup.getNetElement(objId1)->getData(), 1002);
  EXPECT_EQ(masterGroup.getNetElement(objId2)->getData(), 2002);
  EXPECT_EQ(slaveGroup.getNetElement(objId1)->getData(), 1002);
  EXPECT_EQ(slaveGroup.getNetElement(objId2)->getData(), 2002);

  auto objId3 = masterGroup.addNetElement(make_shared<TestElement>(3001));

  sendSlaveUpdate();

  EXPECT_EQ(slaveGroup.netElementIds().size(), 3u);
  EXPECT_EQ(slaveGroup.getNetElement(objId3)->getData(), 3001);

  // Delta must be greater than MaxChangeDataSteps
  auto objId4 = masterGroup.addNetElement(make_shared<TestElement>(4001));
  masterGroup.getNetElement(objId3)->setData(3002);
  masterGroup.getNetElement(objId4)->setData(4002);

  sendSlaveUpdate();

  EXPECT_EQ(slaveGroup.netElementIds().size(), 4u);
  EXPECT_EQ(slaveGroup.getNetElement(objId1)->getData(), 1002);
  EXPECT_EQ(slaveGroup.getNetElement(objId2)->getData(), 2002);
  EXPECT_EQ(slaveGroup.getNetElement(objId3)->getData(), 3002);
  EXPECT_EQ(slaveGroup.getNetElement(objId4)->getData(), 4002);

  NetElementTop<NetElementDynamicGroup<TestElement>> forwardedSlaveGroup;
  uint64_t lastForwardedSlaveUpdateVersion = 0;
  auto sendForwardedSlaveUpdate = [&]() {
    auto p = slaveGroup.writeNetState(lastForwardedSlaveUpdateVersion);
    lastForwardedSlaveUpdateVersion = p.second;
    forwardedSlaveGroup.readNetState(p.first);
  };

  sendForwardedSlaveUpdate();

  EXPECT_EQ(forwardedSlaveGroup.netElementIds().size(), 4u);
  EXPECT_EQ(forwardedSlaveGroup.getNetElement(objId1)->getData(), 1002);
  EXPECT_EQ(forwardedSlaveGroup.getNetElement(objId2)->getData(), 2002);
  EXPECT_EQ(forwardedSlaveGroup.getNetElement(objId3)->getData(), 3002);
  EXPECT_EQ(forwardedSlaveGroup.getNetElement(objId4)->getData(), 4002);

  masterGroup.removeNetElement(objId1);
  masterGroup.removeNetElement(objId3);
  masterGroup.getNetElement(objId2)->setData(2003);
  masterGroup.getNetElement(objId4)->setData(4003);

  sendSlaveUpdate();
  sendForwardedSlaveUpdate();

  auto obj5 = masterGroup.addNetElement(make_shared<TestElement>(5001));
  masterGroup.removeNetElement(obj5);

  sendSlaveUpdate();
  sendForwardedSlaveUpdate();

  EXPECT_EQ(slaveGroup.netElementIds().size(), 2u);
  EXPECT_EQ(slaveGroup.getNetElement(objId2)->getData(), 2003);
  EXPECT_EQ(slaveGroup.getNetElement(objId4)->getData(), 4003);

  EXPECT_EQ(forwardedSlaveGroup.netElementIds().size(), 2u);
  EXPECT_EQ(forwardedSlaveGroup.getNetElement(objId2)->getData(), 2003);
  EXPECT_EQ(forwardedSlaveGroup.getNetElement(objId4)->getData(), 4003);
}

TEST(NetElements, NetElementMap) {
  NetElementTop<NetElementMap<String, String>> masterMap;
  NetElementTop<NetElementMap<String, String>> slaveMap;

  uint64_t lastUpdateVersion = 0;

  auto sendUpdate = [&]() {
    DataStreamBuffer ds;
    auto p = masterMap.writeNetState(lastUpdateVersion);
    slaveMap.readNetState(p.first);
    lastUpdateVersion = p.second;
    return !p.first.empty();
  };

  masterMap.add("foo", "bar");
  masterMap.add("baz", "bof");

  EXPECT_EQ(masterMap.size(), 2u);

  sendUpdate();

  EXPECT_EQ(slaveMap.size(), 2u);
  EXPECT_EQ(slaveMap.get("foo"), "bar");
  EXPECT_EQ(slaveMap.get("baz"), "bof");

  masterMap.add("bif", "fob");
  masterMap.remove("foo");

  sendUpdate();

  EXPECT_EQ(slaveMap.size(), 2u);
  EXPECT_EQ(slaveMap.get("bif"), "fob");

  masterMap.clear();
  masterMap.set("fib", "fab");

  sendUpdate();

  EXPECT_EQ(slaveMap.size(), 1u);
  EXPECT_EQ(slaveMap.get("fib"), "fab");

  masterMap.set("fib", "fab");

  EXPECT_FALSE(sendUpdate());

  masterMap.reset({{"a", "b"}, {"c", "d"}, {"e", "f"}});

  sendUpdate();

  EXPECT_EQ(slaveMap.size(), 3u);
  EXPECT_EQ(slaveMap.get("a"), "b");
  EXPECT_EQ(slaveMap.get("c"), "d");
  EXPECT_EQ(slaveMap.get("e"), "f");
}

TEST(NetElements, NetElementMapInterpolated) {
  typedef NetElementTop<NetElementMap<String, String>> TestMap;

  TestMap masterMap;

  TestMap forwarderMap;
  uint64_t lastForwarderUpdateVersion = 0;
  auto sendForwarderUpdate = [&]() {
    auto p = masterMap.writeNetState(lastForwarderUpdateVersion);
    lastForwarderUpdateVersion = p.second;
    forwarderMap.readNetState(p.first);
    return !p.first.empty();
  };

  TestMap slaveMap;
  uint64_t lastSlaveUpdateVersion = 0;
  auto sendSlaveUpdate = [&](float interpolationTime) {
    auto p = forwarderMap.writeNetState(lastSlaveUpdateVersion);
    lastSlaveUpdateVersion = p.second;
    slaveMap.readNetState(p.first, interpolationTime);
    return !p.first.empty();
  };

  slaveMap.enableNetInterpolation();

  masterMap.add("foo", "bar");
  masterMap.add("baz", "bof");

  EXPECT_EQ(masterMap.size(), 2u);

  sendForwarderUpdate();
  sendSlaveUpdate(0.0f);

  EXPECT_EQ(slaveMap.size(), 2u);
  EXPECT_EQ(slaveMap.get("foo"), "bar");
  EXPECT_EQ(slaveMap.get("baz"), "bof");

  masterMap.add("bif", "fob");
  masterMap.add("qux", "qux");
  masterMap.remove("foo");

  sendForwarderUpdate();
  sendSlaveUpdate(1.0f);

  EXPECT_EQ(slaveMap.size(), 2u);
  EXPECT_EQ(slaveMap.get("foo"), "bar");
  EXPECT_EQ(slaveMap.get("baz"), "bof");

  slaveMap.tickNetInterpolation(1.0f);

  EXPECT_EQ(slaveMap.size(), 3u);
  EXPECT_EQ(slaveMap.get("baz"), "bof");
  EXPECT_EQ(slaveMap.get("bif"), "fob");
  EXPECT_EQ(slaveMap.get("qux"), "qux");

  masterMap.clear();
  masterMap.set("fib", "fab");

  sendForwarderUpdate();
  sendSlaveUpdate(1.0f);

  EXPECT_EQ(slaveMap.size(), 3u);

  slaveMap.tickNetInterpolation(1.0f);

  EXPECT_EQ(forwarderMap.size(), 1u);
  EXPECT_EQ(slaveMap.size(), 1u);
  EXPECT_EQ(slaveMap.get("fib"), "fab");

  masterMap.set("fob", "fub");

  sendForwarderUpdate();
  sendSlaveUpdate(1.0f);

  EXPECT_EQ(slaveMap.size(), 1u);
  slaveMap.disableNetInterpolation();
  EXPECT_EQ(slaveMap.size(), 2u);
  EXPECT_EQ(slaveMap.get("fib"), "fab");
  EXPECT_EQ(slaveMap.get("fob"), "fub");
}

TEST(NetElements, NetElementSignal) {
  NetElementSignal<int> masterSignal1;
  NetElementSignal<int> masterSignal2;

  NetElementTopGroup masterGroup;
  masterGroup.addNetElement(&masterSignal1);
  masterGroup.addNetElement(&masterSignal2);

  NetElementSignal<int> slaveSignal1;
  NetElementSignal<int> slaveSignal2;

  NetElementTopGroup slaveGroup;
  slaveGroup.addNetElement(&slaveSignal1);
  slaveGroup.addNetElement(&slaveSignal2);

  // No signals are supposed to be sent for the initial write
  auto masterUpdate1 = masterGroup.writeNetState();
  slaveGroup.readNetState(masterUpdate1.first);

  masterSignal1.send(101);
  masterSignal2.send(201);

  EXPECT_EQ(masterSignal1.receive(), List<int>({101}));
  EXPECT_EQ(masterSignal2.receive(), List<int>({201}));

  EXPECT_EQ(masterSignal1.receive(), List<int>({}));
  EXPECT_EQ(masterSignal2.receive(), List<int>({}));

  auto masterUpdate2 = masterGroup.writeNetState(masterUpdate1.second);
  slaveGroup.readNetState(masterUpdate2.first);

  EXPECT_EQ(slaveSignal1.receive(), List<int>({101}));
  EXPECT_EQ(slaveSignal2.receive(), List<int>({201}));

  EXPECT_EQ(slaveSignal1.receive(), List<int>({}));
  EXPECT_EQ(slaveSignal2.receive(), List<int>({}));

  masterSignal1.send(102);
  masterSignal2.send(202);

  slaveGroup.enableNetInterpolation();

  auto masterUpdate3 = masterGroup.writeNetState(masterUpdate2.second);
  slaveGroup.readNetState(masterUpdate3.first, 1.0f);

  EXPECT_EQ(slaveSignal1.receive(), List<int>({}));
  EXPECT_EQ(slaveSignal2.receive(), List<int>({}));

  slaveGroup.tickNetInterpolation(1.0f);

  EXPECT_EQ(slaveSignal1.receive(), List<int>({102}));
  EXPECT_EQ(slaveSignal2.receive(), List<int>({202}));

  EXPECT_EQ(slaveSignal1.receive(), List<int>({}));
  EXPECT_EQ(slaveSignal2.receive(), List<int>({}));
}

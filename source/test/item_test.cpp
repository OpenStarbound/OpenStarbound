#include "StarItemDatabase.hpp"

#include <list>

#include "gtest/gtest.h"

using namespace Star;

TEST(ItemTest, ItemDescriptorConstruction) {
  ItemDescriptor testItemDescriptor;

  testItemDescriptor = ItemDescriptor();

  testItemDescriptor = ItemDescriptor(Json());

  String nameOnly = "perfectlygenericitem";
  testItemDescriptor = ItemDescriptor(nameOnly);

  List<JsonArray> arrayFormats = List<JsonArray>{
      JsonArray{"perfectlygenericitem"},
      JsonArray{"perfectlygenericitem", 1},
      JsonArray{"perfectlygenericitem", 1, JsonObject()},
      JsonArray{"perfectlygenericitem", 1, JsonObject{{"testParameter", "testValue"}}}
    };

  for (auto arrayFormat : arrayFormats)
    testItemDescriptor = ItemDescriptor(arrayFormat);

  List<JsonObject> objectFormats = List<JsonObject>{
      JsonObject{{"name", "perfectlygenericitem"}},
      JsonObject{{"item", "perfectlygenericitem"}},
      JsonObject{{"name", "perfectlygenericitem"}, {"count", 1}},
      JsonObject{{"name", "perfectlygenericitem"}, {"count", 1}, {"parameters", JsonObject()}},
      JsonObject{{"name", "perfectlygenericitem"}, {"count", 1}, {"parameters", JsonObject{{"testParameter", "testValue"}}}}
    };

  for (auto objectFormat : objectFormats)
    testItemDescriptor = ItemDescriptor(objectFormat);

  testItemDescriptor = ItemDescriptor("perfectlygenericitem", 1);
  testItemDescriptor = ItemDescriptor("perfectlygenericitem", 1, JsonObject{{"testParameter", "testValue"}});
}

TEST(ItemTest, ItemComparison) {
  auto itemDatabase = Root::singleton().itemDatabase();
  ItemPtr testItem = itemDatabase->item(ItemDescriptor("perfectlygenericitem", 1));
  ItemPtr testItemParams = itemDatabase->item(ItemDescriptor("perfectlygenericitem", 1, JsonObject{{"testParameter", "testValue"}}));

  List<ItemDescriptor> testItemDescriptors = List<ItemDescriptor>{
    ItemDescriptor("perfectlygenericitem", 1),
    ItemDescriptor(JsonArray{"perfectlygenericitem"}),
    ItemDescriptor(JsonArray{"perfectlygenericitem", 1}),
    ItemDescriptor(JsonArray{"perfectlygenericitem", 1, JsonObject()}),
    ItemDescriptor(JsonObject{{"name", "perfectlygenericitem"}}),
    ItemDescriptor(JsonObject{{"item", "perfectlygenericitem"}}),
    ItemDescriptor(JsonObject{{"name", "perfectlygenericitem"}, {"count", 1}}),
    ItemDescriptor(JsonObject{{"name", "perfectlygenericitem"}, {"count", 1}, {"parameters", JsonObject()}})
  };

  List<ItemDescriptor> testItemDescriptorsParams = List<ItemDescriptor>{
    ItemDescriptor(JsonArray{"perfectlygenericitem", 1, JsonObject{{"testParameter", "testValue"}}}),
    ItemDescriptor(JsonObject{{"name", "perfectlygenericitem"}, {"count", 1}, {"parameters", JsonObject{{"testParameter", "testValue"}}}})
  };

  // comparisons WITHOUT exactMatch
  for (ItemDescriptor const& id : testItemDescriptors) {
    EXPECT_TRUE(testItem->matches(id));
    EXPECT_TRUE(testItemParams->matches(id));
    EXPECT_TRUE(id.matches(testItem));
    EXPECT_TRUE(id.matches(testItemParams));
    for (ItemDescriptor const& id2 : testItemDescriptors)
      EXPECT_TRUE(id.matches(id2));
    for (ItemDescriptor const& id2 : testItemDescriptorsParams)
      EXPECT_TRUE(id.matches(id2));
  }
  for (ItemDescriptor const& id : testItemDescriptorsParams) {
    EXPECT_TRUE(testItem->matches(id));
    EXPECT_TRUE(testItemParams->matches(id));
    EXPECT_TRUE(id.matches(testItem));
    EXPECT_TRUE(id.matches(testItemParams));
    for (ItemDescriptor const& id2 : testItemDescriptors)
      EXPECT_TRUE(id.matches(id2));
    for (ItemDescriptor const& id2 : testItemDescriptorsParams)
      EXPECT_TRUE(id.matches(id2));
  }
  EXPECT_TRUE(testItem->matches(testItemParams));
  EXPECT_TRUE(testItemParams->matches(testItem));

  // comparisons WITH exactMatch
    for (ItemDescriptor const& id : testItemDescriptors) {
    EXPECT_TRUE(testItem->matches(id, true));
    EXPECT_FALSE(testItemParams->matches(id, true));
    EXPECT_TRUE(id.matches(testItem, true));
    EXPECT_FALSE(id.matches(testItemParams, true));
    for (ItemDescriptor const& id2 : testItemDescriptors)
      EXPECT_TRUE(id.matches(id2, true));
    for (ItemDescriptor const& id2 : testItemDescriptorsParams)
      EXPECT_FALSE(id.matches(id2, true));
  }
  for (ItemDescriptor const& id : testItemDescriptorsParams) {
    EXPECT_FALSE(testItem->matches(id, true));
    EXPECT_TRUE(testItemParams->matches(id, true));
    EXPECT_FALSE(id.matches(testItem, true));
    EXPECT_TRUE(id.matches(testItemParams, true));
    for (ItemDescriptor const& id2 : testItemDescriptors)
      EXPECT_FALSE(id.matches(id2, true));
    for (ItemDescriptor const& id2 : testItemDescriptorsParams)
      EXPECT_TRUE(id.matches(id2, true));
  }
  EXPECT_FALSE(testItem->matches(testItemParams, true));
  EXPECT_FALSE(testItemParams->matches(testItem, true));
}

TEST(ItemTest, ConstructItems) {
  auto itemDatabase = Root::singleton().itemDatabase();

  for (auto itemName : itemDatabase->allItems())
    ItemPtr item = itemDatabase->item(ItemDescriptor(itemName, 1));
}

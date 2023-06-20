#include "StarJson.hpp"
#include "StarFile.hpp"
#include "StarJsonPatch.hpp"
#include "StarJsonPath.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(JsonTest, ImplicitSharing) {
  Json map1 = JsonObject{{"foo", 1}, {"bar", 10}};

  Json map2 = JsonObject{{"foo", 5}, {"bar", 50}};

  std::swap(map1, map2);
  Json map3 = map1;
  map1 = map2;
  map2 = map3;

  EXPECT_EQ(map1.get("foo"), 1);
  EXPECT_EQ(map2.get("bar"), 50);
}

TEST(JsonTest, Defaults) {
  Json obj = JsonObject{{"null", Json()}};
  Json arr = JsonArray{"array", JsonArray{Json(), Json()}};

  EXPECT_EQ(obj.getInt("null", 5), 5);
  EXPECT_EQ(arr.getInt(2, 5), 5);
  EXPECT_EQ(arr.getInt(3, 5), 5);
  EXPECT_THROW(arr.getInt(2), JsonException);
}

TEST(JsonTest, Merging) {
  JsonObject a{{"I", "feel"}, {"friendly", "now"}};
  JsonObject b{{"hello", "there"}, {"leg", "friend"}};
  JsonObject c{{"hello", "you"}, {"leg", "fiend"}};
  JsonObject d{{"goodbye", "you"}, {"friendly", "leg"}};

  Json merged = jsonMerge(a, b, c, d);

  EXPECT_EQ(merged.get("I"), "feel");
  EXPECT_EQ(merged.get("hello"), "you");
  EXPECT_EQ(merged.get("friendly"), "leg");
  EXPECT_EQ(merged.get("leg"), "fiend");

  Json e = JsonObject();
  e = e.set("1", 2);
  e = e.setAll({{"a", "b"}, {"c", "d"}});
  Json f = JsonObject{{"1", 2}, {"a", "b"}, {"c", "d"}};

  EXPECT_EQ(e, f);

  Json g = JsonObject{{"a", "a"}, {"sub", JsonObject()}};
  g = g.setPath("sub.field", 1);
  g = g.setPath("sub.field2", 2);
  g = g.erasePath("sub.field2");
  Json h = JsonObject{{"a", "a"}, {"sub", JsonObject{{"field", 1}}}};
  EXPECT_EQ(g, h);
}

TEST(JsonTest, Unicode) {
  Json v = Json::parse("{ \"first\" : \"日本語\", \"second\" : \"foobar\\u0019\" }");
  EXPECT_EQ(v.getString("first"), String("日本語"));
  EXPECT_EQ(v.get("second").repr(), String("\"foobar\\u0019\""));

  String json = v.printJson();
  Json v2 = Json::parseJson(json);
  EXPECT_EQ(v2.getString("first"), String("日本語"));

  EXPECT_EQ(v, v2);

  EXPECT_EQ(Json("😀"), Json::parse("\"\\ud83d\\ude00\""));
  EXPECT_EQ(Json::parse("\"\\ud83d\\ude00\"").toString().size(), 1u);
}

TEST(JsonTest, UnicodeFile) {
  Json v = Json::parse("{ \"first\" : \"日本語\", \"second\" : \"foobar\\u0019\" }");
  EXPECT_EQ(v.getString("first"), String("日本語"));
  EXPECT_EQ(v.get("second").repr(), String("\"foobar\\u0019\""));

  String file = File::temporaryFileName();
  auto finallyGuard = finally([&file]() { File::remove(file); });

  File::writeFile(v.printJson(), file);
  Json v2 = Json::parseJson(File::readFileString(file));
  EXPECT_EQ(v2.getString("first"), "日本語");

  EXPECT_EQ(v, v2);
}

TEST(JsonTest, JsonParsingEdge) {
  auto isValidFragment = [](String const& json) -> bool {
    try {
      Json::parse(json);
      return true;
    } catch (JsonParsingException const&) {
      return false;
    }
  };

  auto isValidJson = [](String const& json) -> bool {
    try {
      Json::parseJson(json);
      return true;
    } catch (JsonParsingException const&) {
      return false;
    }
  };

  EXPECT_TRUE(isValidFragment(" \t 0.0 "));
  EXPECT_TRUE(isValidFragment("-0.0\t "));
  EXPECT_FALSE(isValidFragment("-.0"));
  EXPECT_FALSE(isValidFragment("00.0"));

  EXPECT_FALSE(isValidJson(" 0.0"));
  EXPECT_FALSE(isValidJson("true"));
  EXPECT_TRUE(isValidJson("\t[]"));
  EXPECT_TRUE(isValidJson(" {} "));
}

TEST(JsonTest, Types) {
  Json v;
  EXPECT_EQ(v.type(), Json::Type::Null);
  v = 0;
  EXPECT_EQ(v.type(), Json::Type::Int);
  v = 0.0;
  EXPECT_EQ(v.type(), Json::Type::Float);
  v = true;
  EXPECT_EQ(v.type(), Json::Type::Bool);
  v = "";
  EXPECT_EQ(v.type(), Json::Type::String);
  v = JsonArray();
  EXPECT_EQ(v.type(), Json::Type::Array);
  v = JsonObject();
  EXPECT_EQ(v.type(), Json::Type::Object);
}

TEST(JsonTest, Query) {
  Json v = Json::parse(R"JSON(
      {
        "foo" : "bar",
        "baz" : {
          "baf" : [1, 2],
          "bal" : 2
        },

        "baf" : null
      }
    )JSON");

  EXPECT_EQ(v.query("foo"), Json("bar"));
  EXPECT_EQ(v.query("baz.baf[1]"), Json(2));
  EXPECT_EQ(v.query("baz.bal"), Json(2));
  EXPECT_EQ(v.query("blargh", Json("default")), Json("default"));
  EXPECT_EQ(v.query("blargh", Json("default")), Json("default"));
  EXPECT_EQ(v.query("baz.baf[3]", Json("default")), Json("default"));
  EXPECT_EQ(v.query("baz.bal[0]", Json("default")), Json("default"));
  EXPECT_EQ(v.query("baz[1]", Json("default")), Json("default"));
  EXPECT_EQ(v.query("baz.bal.a", Json("default")), Json("default"));
  EXPECT_EQ(v.query("baz[0]", Json("default")), Json("default"));
  EXPECT_EQ(v.query("baz.baf.a", Json("default")), Json("default"));
  EXPECT_THROW(v.query("blargh"), JsonPath::TraversalException);
  EXPECT_THROW(v.query("baz.funk"), JsonPath::TraversalException);
  EXPECT_THROW(v.query("baz.baf[3]"), JsonPath::TraversalException);
  EXPECT_THROW(v.query("baz.baf[whee]", Json()), JsonPath::ParsingException);
  EXPECT_THROW(v.query("baz.baf[[]", Json()), JsonPath::ParsingException);
  EXPECT_THROW(v.query("baz..baf", Json()), JsonPath::ParsingException);
  EXPECT_THROW(v.query("baf.nothing"), JsonException);
}

TEST(JsonTest, PatchingAdd) {
  Json before = Json::parse(R"JSON(
      {
        "foo" : "bar",
        "baz" : {
          "baf" : 1,
          "bal" : 2
        },
        "rab" : [0, 1, 2, "foo", false]
      }
    )JSON");

  Json after = Json::parse(R"JSON(
      {
        "foo" : "xyzzy",
        "bar" : "foo",
        "baz" : {
          "baf" : 1,
          "bal" : 2,
          "0" : "derp",
          "rebar" : {
            "after" : "party"
          }
        },
        "rab" : [0, 0, 1, 2, "foo", false, true, { "baz" : "bar"} ]
      }
    )JSON");

  Json patch = Json::parse(R"JSON(
      [
        {"op" : "add", "path" : "/foo", "value" : "xyzzy"},
        {"op" : "add", "path" : "/bar", "value" : "foo"},
        {"op" : "add", "path" : "/baz/rebar", "value" : {}},
        {"op" : "add", "path" : "/baz/rebar/after", "value" : "party"},
        {"op" : "add", "path" : "/baz/0", "value" : "derp"},
        {"op" : "add", "path" : "/rab/0", "value" : 0},
        {"op" : "add", "path" : "/rab/6", "value" : true},
        {"op" : "add", "path" : "/rab/-", "value" : {"baz" : "bar"} }
      ]
  )JSON");

  // Past end of list
  Json badPatch1 = Json::parse(R"JSON(
      [
        {"op" : "add", "path" : "/rab/6", "value" : {"baz" : "bar"} }
      ]
  )JSON");

  // Parent does not exist, map
  Json badPatch2 = Json::parse(R"JSON(
      [
        {"op" : "add", "path" : "/bar/baz", "value" : {"baz" : "bar"} }
      ]
  )JSON");

  // Parent does not exist, list
  Json badPatch3 = Json::parse(R"JSON(
      [
        {"op" : "add", "path" : "/bar/0", "value" : {"baz" : "bar"} }
      ]
  )JSON");

  EXPECT_EQ(jsonPatch(before, patch.toArray()), after);
  ASSERT_THROW(jsonPatch(before, badPatch1.toArray()), JsonPatchException);
  ASSERT_THROW(jsonPatch(before, badPatch2.toArray()), JsonPatchException);
  ASSERT_THROW(jsonPatch(before, badPatch3.toArray()), JsonPatchException);
}

TEST(JsonTest, PatchingRemove) {
  Json before = Json::parse(R"JSON(
      {
        "foo" : "xyzzy",
        "bar" : "foo",
        "baz" : {
          "baf" : 1,
          "bal" : 2,
          "rebar" : true
        },
        "rab" : [0, 0, 1, 2, "foo", false, {"baz" : "bar"} ]
      }
    )JSON");

  Json after = Json::parse(R"JSON(
      {
        "bar" : "foo",
        "baz" : {
          "baf" : 1,
          "bal" : 2
        },
        "rab" : [0, 1, 2, "foo", false]
      }
    )JSON");

  Json patch = Json::parse(R"JSON(
      [
        {"op" : "remove", "path" : "/foo"},
        {"op" : "remove", "path" : "/baz/rebar"},
        {"op" : "remove", "path" : "/rab/0"},
        {"op" : "remove", "path" : "/rab/5"}
      ]
  )JSON");

  // Removing end of list
  Json badPatch1 = Json::parse(R"JSON(
      [
        {"op" : "remove", "path" : "/rab/-"}
      ]
  )JSON");

  // Removing past end of list
  Json badPatch2 = Json::parse(R"JSON(
      [
        {"op" : "add", "path" : "/rab/7"}
      ]
  )JSON");

  // Path wrong type
  Json badPatch3 = Json::parse(R"JSON(
      [
        {"op" : "remove", "path" : "/bar/baz"}
      ]
  )JSON");

  EXPECT_EQ(jsonPatch(before, patch.toArray()), after);
  ASSERT_THROW(jsonPatch(before, badPatch1.toArray()), JsonPatchException);
  ASSERT_THROW(jsonPatch(before, badPatch2.toArray()), JsonPatchException);
  ASSERT_THROW(jsonPatch(before, badPatch3.toArray()), JsonPatchException);
}

TEST(JsonTest, PatchingReplace) {
  Json before = Json::parse(R"JSON(
      {
        "foo" : "bar",
        "bar" : {
          "baf" : 1,
          "bal" : 2
        },
        "baz" : {
          "baf" : 1,
          "bal" : 2
        },
        "rab" : [0, 1, 2, "foo", false],
        "rabby" : [0, 1, 2, "foo", false]
      }
    )JSON");

  Json after = Json::parse(R"JSON(
      {
        "foo" : "xyzzy",
        "bar" : [3, 2, 1, "contact"],
        "baz" : {
          "baf" : 1,
          "bal" : "touched"
        },
        "rab" : [{"omg" : "no"}, 1, 2, "foo", false],
        "rabby" : false
      }
    )JSON");

  Json patch = Json::parse(R"JSON(
      [
        {"op" : "replace", "path" : "/foo", "value" : "xyzzy"},
        {"op" : "replace", "path" : "/bar", "value" : [3, 2, 1, "contact"]},
        {"op" : "replace", "path" : "/baz/bal", "value" : "touched"},
        {"op" : "replace", "path" : "/rab/0", "value" : {"omg" : "yes"}},
        {"op" : "replace", "path" : "/rab/0/omg", "value" : "no"},
        {"op" : "replace", "path" : "/rab/2", "value" : 2},
        {"op" : "replace", "path" : "/rabby", "value" : false}
      ]
  )JSON");

  // End of list
  Json badPatch1 = Json::parse(R"JSON(
      [
        {"op" : "replace", "path" : "/rab/-", "value" : {"baz" : "bar"} }
      ]
  )JSON");

  // Past end of list
  Json badPatch2 = Json::parse(R"JSON(
      [
        {"op" : "replace", "path" : "/rab/5", "value" : {"baz" : "bar"} }
      ]
  )JSON");

  // Key does not exist
  Json badPatch3 = Json::parse(R"JSON(
      [
        {"op" : "replace", "path" : "/bar/baz", "value" : {"baz" : "bar"} }
      ]
  )JSON");

  EXPECT_EQ(jsonPatch(before, patch.toArray()), after);
  ASSERT_THROW(jsonPatch(before, badPatch1.toArray()), JsonPatchException);
  ASSERT_THROW(jsonPatch(before, badPatch2.toArray()), JsonPatchException);
  ASSERT_THROW(jsonPatch(before, badPatch3.toArray()), JsonPatchException);
}

TEST(JsonTest, PatchingMove) {
  Json before = Json::parse(R"JSON(
      {
        "foo" : "bar",
        "bar" : [1, 2, 3, "contact"],
        "baz" : {
          "baf" : 1,
          "bar" : 2
        },
        "rab" : [0, 1, 2, "foo", false],
        "rabby" : [0, 1, 2, "foo", true]
      }
    )JSON");

  Json after = Json::parse(R"JSON(
      {
        "foot" : "bar",
        "baz" : {
          "baf" : 1,
          "bar" : [3, 2, 1, "contact"]
        },
        "bar" : 2,
        "rab" : [0, 1, 2, true, "foo"]
      }
    )JSON");

  Json patch = Json::parse(R"JSON(
      [
        {"op" : "move", "from" : "/foo", "path" : "/foot"},
        {"op" : "move", "from" : "/bar", "path" : "/baz/bal"},
        {"op" : "move", "from" : "/baz/bar", "path" : "/bar"},
        {"op" : "move", "from" : "/baz/bal", "path" : "/baz/bar"},
        {"op" : "move", "from" : "/baz/bar/0", "path" : "/baz/bar/1"},
        {"op" : "move", "from" : "/baz/bar/2", "path" : "/baz/bar/0"},
        {"op" : "move", "from" : "/rabby", "path" : "/rab"},
        {"op" : "move", "from" : "/rab/3", "path" : "/rab/-"}
      ]
  )JSON");

  // From end of list
  Json badPatch1 = Json::parse(R"JSON(
      [
        {"op" : "move", "from" : "/rab/-", "path" : "/doesnotmatter"}
      ]
  )JSON");

  // From past end of list
  Json badPatch2 = Json::parse(R"JSON(
      [
        {"op" : "move", "from" : "/rab/5", "path" : "/doesnotmatter"}
      ]
  )JSON");

  // To past end of list
  Json badPatch3 = Json::parse(R"JSON(
      [
        {"op" : "move", "from" : "/rab/0", "path" : "/rab/5"}
      ]
  )JSON");

  // Source path does not exist
  Json badPatch4 = Json::parse(R"JSON(
      [
        {"op" : "move", "from" : "/omgomg", "path" : "/doesntmatter"}
      ]
  )JSON");

  // Dest path wrong type
  Json badPatch5 = Json::parse(R"JSON(
      [
        {"op" : "move", "from" : "/baz/bar", "path" : "/rabby/bar"}
      ]
  )JSON");

  EXPECT_EQ(jsonPatch(before, patch.toArray()), after);
  ASSERT_THROW(jsonPatch(before, badPatch1.toArray()), JsonPatchException);
  ASSERT_THROW(jsonPatch(before, badPatch2.toArray()), JsonPatchException);
  ASSERT_THROW(jsonPatch(before, badPatch3.toArray()), JsonPatchException);
  ASSERT_THROW(jsonPatch(before, badPatch4.toArray()), JsonPatchException);
  ASSERT_THROW(jsonPatch(before, badPatch5.toArray()), JsonPatchException);
}

TEST(JsonTest, PatchingCopy) {
  Json before = Json::parse(R"JSON(
      {
        "foo" : "bar",
        "foot" : "bar",
        "bar" : [1, 2, 3, "contact"],
        "baz" : {
          "baf" : 1,
          "bar" : 2
        },
        "rab" : [0, 1, 2, "foo", false],
        "rabby" : [0, 1, 2, "foo", true]
      }
    )JSON");

  Json after = Json::parse(R"JSON(
      {
        "foo" : "bar",
        "foot" : "bar",
        "baz" : {
          "baf" : 1,
          "bar" : [2, 1, 1, 2, 3, "contact"],
          "bal" : [1, 2, 3, "contact"]
        },
        "bar" : 2,
        "rab" : [0, 1, 2, "foo", true, "foo"],
        "rabby" : [0, 1, 2, "foo", true]
      }
    )JSON");

  Json patch = Json::parse(R"JSON(
      [
        {"op" : "copy", "from" : "/foo", "path" : "/foot"},
        {"op" : "copy", "from" : "/bar", "path" : "/baz/bal"},
        {"op" : "copy", "from" : "/baz/bar", "path" : "/bar"},
        {"op" : "copy", "from" : "/baz/bal", "path" : "/baz/bar"},
        {"op" : "copy", "from" : "/baz/bar/0", "path" : "/baz/bar/1"},
        {"op" : "copy", "from" : "/baz/bar/2", "path" : "/baz/bar/0"},
        {"op" : "copy", "from" : "/rabby", "path" : "/rab"},
        {"op" : "copy", "from" : "/rab/3", "path" : "/rab/-"}
      ]
  )JSON");

  // From end of list
  Json badPatch1 = Json::parse(R"JSON(
      [
        {"op" : "copy", "from" : "/rab/-", "path" : "/doesnotmatter"}
      ]
  )JSON");

  // From past end of list
  Json badPatch2 = Json::parse(R"JSON(
      [
        {"op" : "copy", "from" : "/rab/5", "path" : "/doesnotmatter"}
      ]
  )JSON");

  // To past end of list
  Json badPatch3 = Json::parse(R"JSON(
      [
        {"op" : "copy", "from" : "/rab/0", "path" : "/rab/6"}
      ]
  )JSON");

  // Source path does not exist
  Json badPatch4 = Json::parse(R"JSON(
      [
        {"op" : "copy", "from" : "/omgomg", "path" : "/doesntmatter"}
      ]
  )JSON");

  // Dest path wrong type
  Json badPatch5 = Json::parse(R"JSON(
      [
        {"op" : "copy", "from" : "/baz/bar", "path" : "/rabby/bar"}
      ]
  )JSON");

  EXPECT_EQ(jsonPatch(before, patch.toArray()), after);
  ASSERT_THROW(jsonPatch(before, badPatch1.toArray()), JsonPatchException);
  ASSERT_THROW(jsonPatch(before, badPatch2.toArray()), JsonPatchException);
  ASSERT_THROW(jsonPatch(before, badPatch3.toArray()), JsonPatchException);
  ASSERT_THROW(jsonPatch(before, badPatch4.toArray()), JsonPatchException);
  ASSERT_THROW(jsonPatch(before, badPatch5.toArray()), JsonPatchException);
}

TEST(JsonTest, PatchingTest) {
  Json base = Json::parse(R"JSON(
      {
        "foo" : "bar",
        "foot" : "bart",
        "bar" : [1, 2, 3, "contact"],
        "baz" : {
          "baf" : 1,
          "bar" : 2,
          "0" : 3
        }
      }
    )JSON");

  Json goodTest = Json::parse(R"JSON(
      [
        {"op" : "test", "path" : "/foo", "value" : "bar"},
        {"op" : "test", "path" : "/foo", "value" : "bark", "inverse" : true},
        {"op" : "test", "path" : "/foot", "value" : "bart"},
        {"op" : "test", "path" : "/bar", "value" : [1, 2, 3, "contact"]},
        {"op" : "test", "path" : "/bar/0", "value" : 1},
        {"op" : "test", "path" : "/bar/1", "value" : 2},
        {"op" : "test", "path" : "/bar/2", "value" : 3},
        {"op" : "test", "path" : "/bar/3", "value" : "contact"},
        {"op" : "test", "path" : "/baz", "value" : {"0" : 3, "baf" : 1, "bar" : 2}},
        {"op" : "test", "path" : "/baz/baf", "value" : 1},
        {"op" : "test", "path" : "/baz/bar", "value" : 2},
        {"op" : "test", "path" : "/baz/0", "value" : 3},
        {"op" : "test", "path" : "/nothere", "inverse" : true},
        {"op" : "test", "path" : "/foo" }
      ]
  )JSON");

  Json failTest1 = Json::parse(R"JSON(
      [
        {"op" : "test", "path" : "/bar", "value" : [1, 3, 2, "contact"]}
      ]
  )JSON");

  Json failTest2 = Json::parse(R"JSON(
      [
        {"op" : "test", "path" : "/bar/-", "value" : "contact"}
      ]
  )JSON");

  Json failTest3 = Json::parse(R"JSON(
      [
        {"op" : "test", "path" : "/xyzzy", "value" : null}
      ]
  )JSON");

  Json failTest4 = Json::parse(R"JSON(
      [
        {"op" : "test", "path" : "/xyzzy/zop", "value" : null}
      ]
  )JSON");

  Json failTest5 = Json::parse(R"JSON(
      [
        {"op" : "test", "path" : "/nothere" }
      ]
  )JSON");

  Json failTest6 = Json::parse(R"JSON(
      [
        {"op" : "test", "path" : "/bar", "inverse" : true }
      ]
  )JSON");

  Json failTest7 = Json::parse(R"JSON(
      [
        {"op" : "test", "path" : "/foo", "value" : "bar", "inverse" : true }
      ]
  )JSON");

  jsonPatch(base, goodTest.toArray());

  ASSERT_THROW(jsonPatch(base, failTest1.toArray()), JsonPatchTestFail);
  ASSERT_THROW(jsonPatch(base, failTest2.toArray()), JsonPatchTestFail);
  ASSERT_THROW(jsonPatch(base, failTest3.toArray()), JsonPatchTestFail);
  ASSERT_THROW(jsonPatch(base, failTest4.toArray()), JsonPatchTestFail);
  ASSERT_THROW(jsonPatch(base, failTest5.toArray()), JsonPatchTestFail);
  ASSERT_THROW(jsonPatch(base, failTest6.toArray()), JsonPatchTestFail);
  ASSERT_THROW(jsonPatch(base, failTest7.toArray()), JsonPatchTestFail);
}

TEST(JsonTest, PatchingEscaping) {
  Json base1 = Json::parse(R"JSON(
      {
        "~" : true,
        "/" : false,
        "~~0" : "foo",
        "~~1" : "bar",
        "~~0~1/~0~" : "ugh"
      }
  )JSON");

  Json test1 = Json::parse(R"JSON(
      [
        {"op" : "test", "path" : "/~0", "value" : true},
        {"op" : "test", "path" : "/~1", "value" : false},
        {"op" : "test", "path" : "/~0~00", "value" : "foo"},
        {"op" : "test", "path" : "/~0~01", "value" : "bar"},
        {"op" : "test", "path" : "/~0~00~01~1~00~0", "value" : "ugh"}
      ]
  )JSON");

  jsonPatch(base1, test1.toArray());
}

TEST(JsonTest, MergeQuery) {
  Json json1 = Json::parse(R"JSON(
      {
        "foo" : "foo1",
        "bar" : "bar1",
        "baz" : {
          "1" : "1"
        },
        "fob" : {},
        "fizz" : 4
      }
    )JSON");
  Json json2 = Json::parse(R"JSON(
      {
        "foo" : "foo2",
        "bar" : "bar2",
        "baz" : null,
        "baf" : {
          "2" : "2"
        },
        "fob" : 2
      }
    )JSON");
  Json json3 = Json::parse(R"JSON(
      {
        "baz" : {
          "3" : "3"
        },
        "baf" : {
          "3" : "3"
        },
        "fizz" : {
        }
      }
    )JSON");

  auto testIdentical = [&](String const& key) {
    EXPECT_EQ(jsonMergeQuery(key, json1, json2, json3), jsonMerge(json1, json2, json3).query(key, {}));
  };

  testIdentical("foo");
  testIdentical("bar");
  testIdentical("baz");
  testIdentical("baf");
  testIdentical("baz.1");
  testIdentical("baz.2");
  testIdentical("baz.3");
  testIdentical("baf.0");
  testIdentical("baf.2");
  testIdentical("baf.3");
  testIdentical("baz.blip");
  testIdentical("boo.blip");
  testIdentical("fob");
  testIdentical("fiz");
  testIdentical("nothing");
}

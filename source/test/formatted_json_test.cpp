#include "StarFormattedJson.hpp"
#include "StarJsonPath.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(FormattedJsonTest, JsonInterop) {
  Json array1 = JsonArray{1, 2, 3, 4};
  Json array2 = JsonArray{4, 3, 2, 1};
  FormattedJson farray1 = array1;
  FormattedJson farray2 = array2;

  EXPECT_EQ(farray1.toJson(), array1);
  EXPECT_EQ(farray2.toJson(), array2);
  EXPECT_NE(farray1.toJson(), array2);
  EXPECT_NE(farray2.toJson(), array1);
}

TEST(FormattedJsonTest, Parsing) {
  FormattedJson json1 = FormattedJson::parse(R"JSON(
    {
      "foo": "bar",
      "hello" : "world",
      "abc" :123
      // Comment
      ,"wat": {
        "thing": [
          49,
          27]
      }
    }
)JSON");
  FormattedJson::ElementList expectedElements = {
    WhitespaceElement{"\n      "},
    ObjectKeyElement{"foo"},
    ColonElement{},
    WhitespaceElement{" "},
    ValueElement{Json{"bar"}},
    CommaElement{},
    WhitespaceElement{"\n      "},
    ObjectKeyElement{"hello"},
    WhitespaceElement{" "},
    ColonElement{},
    WhitespaceElement{" "},
    ValueElement{Json{"world"}},
    CommaElement{},
    WhitespaceElement{"\n      "},
    ObjectKeyElement{"abc"},
    WhitespaceElement{" "},
    ColonElement{},
    ValueElement{Json{123}},
    WhitespaceElement{"\n      // Comment\n      "},
    CommaElement{},
    ObjectKeyElement{"wat"},
    ColonElement{},
    WhitespaceElement{" "},
    ValueElement{Json{JsonObject{{"thing", JsonArray{49, 27}}}}},
    WhitespaceElement{"\n    "}
  };
  EXPECT_EQ(json1.elements(), expectedElements);
  EXPECT_EQ(json1.get("foo"), FormattedJson{"bar"});
  EXPECT_EQ(json1.get("abc"), FormattedJson{123});
  EXPECT_EQ(json1.get("wat").get("thing").get(1), FormattedJson{27});
  EXPECT_NE(json1.get("wat").get("thing").get(0), FormattedJson{66});

  ASSERT_THROW(FormattedJson::parse(" "), JsonParsingException);
  ASSERT_THROW(FormattedJson::parse("/* */"), JsonParsingException);
  ASSERT_THROW(FormattedJson::parse("x"), JsonParsingException);
  ASSERT_THROW(FormattedJson::parseJson("123"), JsonParsingException);
  ASSERT_THROW(FormattedJson::parseJson("\"foo\""), JsonParsingException);
  EXPECT_TRUE(FormattedJson::parse("123").isType(Json::Type::Int));
  EXPECT_TRUE(FormattedJson::parse("\"foo\"").isType(Json::Type::String));
}

List<String> keyOrder(FormattedJson const& json) {
  List<String> keys;
  for (JsonElement const& elem : json.elements()) {
    if (elem.is<ObjectKeyElement>())
      keys.append(elem.get<ObjectKeyElement>().key);
  }
  return keys;
}

TEST(FormattedJsonTest, ObjectInsertion) {
  FormattedJson json = FormattedJson::ofType(Json::Type::Object);
  List<String> expectedKeys;

  EXPECT_EQ(keyOrder(json), expectedKeys);

  json = json.set("foo", Json{"bar"});
  expectedKeys.append("foo");
  EXPECT_EQ(keyOrder(json), expectedKeys);

  json = json.set("baz", Json{"..."});
  expectedKeys.append("baz");
  EXPECT_EQ(keyOrder(json), expectedKeys);

  json = json.prepend("hello", Json{"world"});
  expectedKeys.insertAt(0, "hello");
  EXPECT_EQ(keyOrder(json), expectedKeys);

  json = json.insertBefore("lala", Json{"alal"}, "foo");
  expectedKeys.insertAt(1, "lala");
  EXPECT_EQ(keyOrder(json), expectedKeys);

  json = json.insertAfter("lorem", Json{"ipsum"}, "foo");
  expectedKeys.insertAt(3, "lorem");
  EXPECT_EQ(keyOrder(json), expectedKeys);

  json = json.append("dolor", Json{"sit amet"});
  expectedKeys.append("dolor");
  EXPECT_EQ(keyOrder(json), expectedKeys);

  // If the key already exists, the key order doesn't change.
  json = json.set("foo", Json{123}).append("hello", Json{123}).insertAfter("dolor", Json{123}, "baz");
  EXPECT_EQ(keyOrder(json), expectedKeys);

  FormattedJson::ElementList expectedElements = {ObjectKeyElement{"hello"},
      ColonElement{},
      ValueElement{Json{123}},
      CommaElement{},
      ObjectKeyElement{"lala"},
      ColonElement{},
      ValueElement{Json{"alal"}},
      CommaElement{},
      ObjectKeyElement{"foo"},
      ColonElement{},
      ValueElement{Json{123}},
      CommaElement{},
      ObjectKeyElement{"lorem"},
      ColonElement{},
      ValueElement{Json{"ipsum"}},
      CommaElement{},
      ObjectKeyElement{"baz"},
      ColonElement{},
      ValueElement{Json{"..."}},
      CommaElement{},
      ObjectKeyElement{"dolor"},
      ColonElement{},
      ValueElement{Json{123}}};
  EXPECT_EQ(json.elements(), expectedElements);

  FormattedJson emptyObject = FormattedJson::ofType(Json::Type::Object);
  ASSERT_THROW(emptyObject.insertBefore("foo", Json{}, "bar"), JsonException);
  ASSERT_THROW(emptyObject.insertAfter("foo", Json{}, "bar"), JsonException);
  ASSERT_THROW(FormattedJson::ofType(Json::Type::Array).set("foo", Json{}), JsonException);
}

TEST(FormattedJsonTest, ObjectInsertionWithWhitespace) {
  FormattedJson json = FormattedJson::parse(" {  \"foo\": 123  } ");
  json = json.append("hello", Json{"world"});
  json = json.prepend("lorem", Json{"ipsum"});
  EXPECT_EQ(json.repr(),
      R"JSON({  "lorem": "ipsum",  "foo": 123,  "hello": "world"  })JSON");
}

TEST(FormattedJsonTest, ArrayInsertion) {
  FormattedJson json = FormattedJson::parse(R"JSON([
    12,
    34
  ])JSON");
  json = json.insert(1, Json{23});
  json = json.append(Json{45});
  json = json.set(0, Json{"01"});
  json = json.insert(0, Json{0});
  char const* expected = R"JSON([
    0,
    "01",
    23,
    34,
    45
  ])JSON";
  EXPECT_EQ(json.repr(), expected);

  FormattedJson emptyArray = FormattedJson::ofType(Json::Type::Array);
  EXPECT_EQ(emptyArray.insert(0, Json{}).size(), 1u);
  ASSERT_THROW(emptyArray.insert(1, Json{}), JsonException);
  ASSERT_THROW(FormattedJson::ofType(Json::Type::Object).insert(0, Json{}), JsonException);
}

TEST(FormattedJsonTest, ObjectErase) {
  FormattedJson json = FormattedJson::parse(R"JSON({
    "zzz": 123,
    "mmm": 456,
    "aaa": 789
  })JSON");
  json = json.eraseKey("mmm");
  char const* expected = R"JSON({
    "zzz": 123,
    "aaa": 789
  })JSON";
  EXPECT_EQ(json.repr(), expected);

  FormattedJson jsonNoZ = json.eraseKey("zzz");
  expected = R"JSON({
    "aaa": 789
  })JSON";
  EXPECT_EQ(jsonNoZ.repr(), expected);

  FormattedJson jsonNoA = json.eraseKey("aaa");
  expected = R"JSON({
    "zzz": 123
  })JSON";
  EXPECT_EQ(jsonNoA.repr(), expected);

  ASSERT_EQ(json.eraseKey("bbb"), json);
  ASSERT_THROW(FormattedJson::ofType(Json::Type::Array).eraseKey("foo"), JsonException);
}

TEST(FormattedJsonTest, ArrayErase) {
  FormattedJson json = FormattedJson::parse("[123, 456, 789]");
  EXPECT_EQ(json.eraseIndex(0).repr(), "[456, 789]");
  EXPECT_EQ(json.eraseIndex(1).repr(), "[123, 789]");
  EXPECT_EQ(json.eraseIndex(2).repr(), "[123, 456]");
  EXPECT_EQ(json.eraseIndex(0).eraseIndex(0).repr(), "[789]");
  EXPECT_EQ(json.eraseIndex(0).eraseIndex(0).eraseIndex(0).repr(), "[]");

  ASSERT_THROW(FormattedJson::ofType(Json::Type::Object).eraseIndex(0), JsonException);
}

TEST(FormattedJsonTest, CommentPreservation) {
  FormattedJson json = FormattedJson::parse(R"JSON({
    // This is a comment
    "hello": 1,
    "world": 2
  })JSON");
  json = json.insertBefore("goodbye", Json{1}, "world");
  json = json.eraseKey("hello");
  char const* expected = R"JSON({
    // This is a comment
    "goodbye": 1,
    "world": 2
  })JSON";
  EXPECT_EQ(json.repr(), expected);
}

TEST(FormattedJsonTest, StylePreservation) {
  FormattedJson json = FormattedJson::parse(R"JSON({

    "hello"        :          1234

  })JSON");
  json = json.append("world", Json{5678});
  char const* expected = R"JSON({

    "hello"        :          1234,

    "world"        :          5678

  })JSON";
  EXPECT_EQ(json.repr(), expected);
}

TEST(FormattedJsonTest, Queries) {
  FormattedJson json0 = FormattedJson::parse("[]");
  FormattedJson json1 = FormattedJson::parse("{\"a\":1}");
  FormattedJson json2 = FormattedJson::parse("[1,2]");
  FormattedJson json3 = FormattedJson::parse(R"({"a":1,"b":2,"c":3})");

  EXPECT_EQ(json0.size(), 0u);
  EXPECT_EQ(json1.size(), 1u);
  EXPECT_EQ(json2.size(), 2u);
  EXPECT_EQ(json3.size(), 3u);

  EXPECT_TRUE(json1.contains("a"));
  EXPECT_FALSE(json1.contains("b"));
  EXPECT_TRUE(json3.contains("c"));
  EXPECT_TRUE(json3.contains("b"));

  ASSERT_THROW(FormattedJson::parse("123").size(), JsonException);
  ASSERT_THROW(json2.contains("1"), JsonException);
}

TEST(FormattedJsonTest, Types) {
  FormattedJson jsonNull = Json{};
  FormattedJson jsonBool = Json{true};
  FormattedJson jsonInt = Json{1};
  FormattedJson jsonFloat = Json{2.0};
  FormattedJson jsonString = Json{"foo"};
  FormattedJson jsonArray = Json::ofType(Json::Type::Array);
  FormattedJson jsonObject = Json::ofType(Json::Type::Object);

  EXPECT_EQ(jsonNull.type(), Json::Type::Null);
  EXPECT_TRUE(jsonNull.isType(Json::Type::Null));
  EXPECT_EQ(jsonNull.typeName(), "null");

  EXPECT_EQ(jsonBool.type(), Json::Type::Bool);
  EXPECT_TRUE(jsonBool.isType(Json::Type::Bool));
  EXPECT_EQ(jsonBool.typeName(), "bool");

  EXPECT_EQ(jsonInt.type(), Json::Type::Int);
  EXPECT_TRUE(jsonInt.isType(Json::Type::Int));
  EXPECT_EQ(jsonInt.typeName(), "int");

  EXPECT_EQ(jsonFloat.type(), Json::Type::Float);
  EXPECT_TRUE(jsonFloat.isType(Json::Type::Float));
  EXPECT_EQ(jsonFloat.typeName(), "float");

  EXPECT_EQ(jsonString.type(), Json::Type::String);
  EXPECT_TRUE(jsonString.isType(Json::Type::String));
  EXPECT_EQ(jsonString.typeName(), "string");

  EXPECT_EQ(jsonArray.type(), Json::Type::Array);
  EXPECT_TRUE(jsonArray.isType(Json::Type::Array));
  EXPECT_EQ(jsonArray.typeName(), "array");

  EXPECT_EQ(jsonObject.type(), Json::Type::Object);
  EXPECT_TRUE(jsonObject.isType(Json::Type::Object));
  EXPECT_EQ(jsonObject.typeName(), "object");
}

TEST(FormattedJsonTest, JsonPath) {
  FormattedJson json = FormattedJson::parse(R"JSON({
      "foo": {
        "bar": [
          12,
          {
            "hello": "world"
          },
          45
        ]
      },
      "baz": [{"a":1}, {"a":2}, {"a":3}]
    })JSON");

  FormattedJson expectedHelloWorld = Json{JsonObject{{"hello", "world"}}};
  EXPECT_EQ(JsonPath::Pointer("/foo/bar/1").get(json), expectedHelloWorld);
  EXPECT_EQ(JsonPath::QueryPath("foo.bar[1]").get(json), expectedHelloWorld);
  EXPECT_EQ(JsonPath::Pointer("/baz/0/a").get(json), FormattedJson{Json{1}});
  EXPECT_EQ(JsonPath::QueryPath("baz[0].a").get(json), FormattedJson{Json{1}});

  json = JsonPath::Pointer("/baz/0/a").set(json, FormattedJson{Json{0}});
  json = JsonPath::QueryPath("baz[1].a").set(json, FormattedJson{Json{4}});

  json = JsonPath::Pointer("/baz/1").add(json, FormattedJson::parse("{\"b\":1}"));
  json = JsonPath::QueryPath("baz[1]").add(json, FormattedJson::parse("{\"c\":0.5}"));

  EXPECT_EQ(json.get("baz").repr(),
      R"([{"a":0},{"c":0.5},{"b":1}, {"a":4}, {"a":3}])");

  json = JsonPath::Pointer("/plz").set(json, FormattedJson{JsonArray{}});
  json = JsonPath::Pointer("/plz/-").set(json, FormattedJson{Json{"thx"}});
  json = JsonPath::Pointer("/plz/-").add(json, FormattedJson{Json{"bye"}});
  FormattedJson expectedThxBye = Json{JsonArray{"thx", "bye"}};
  EXPECT_EQ(json.get("plz"), expectedThxBye);

  // Set and add are almost the same, but:
  //    Set 0 => replaces the first array element
  json = JsonPath::Pointer("/plz/0").set(json, FormattedJson{Json{"kthx"}});
  //    Add 0 => inserts a new element at the beginning
  json = JsonPath::Pointer("/plz/0").add(json, FormattedJson{Json{"kbye"}});
  FormattedJson expectedKByeKThxBye = Json{JsonArray{"kbye", "kthx", "bye"}};
  EXPECT_EQ(json.get("plz"), expectedKByeKThxBye);

  json = JsonPath::Pointer("/foo/bar/1").remove(json);
  FormattedJson expectedBar = Json{JsonArray{12, 45}};
  EXPECT_EQ(JsonPath::Pointer("/foo/bar").get(json), expectedBar);
  json = JsonPath::QueryPath("foo.bar[1]").remove(json);
  EXPECT_EQ(JsonPath::QueryPath("foo.bar").get(json).toJson(), JsonArray{12});
}

TEST(FormattedJsonTest, NumberFormatPreservation) {
  EXPECT_EQ(FormattedJson::parse("1.0").repr(), "1.0");
  EXPECT_EQ(FormattedJson::parse("1").repr(), "1");
  EXPECT_EQ(FormattedJson::parse("-0").repr(), "-0");
  EXPECT_EQ(FormattedJson::parse("0").repr(), "0");

  EXPECT_EQ(FormattedJson::parseJson("[-0.0,1.0,-0]").repr(), "[-0.0,1.0,-0]");
  EXPECT_EQ(FormattedJson::parseJson("[1,0]").repr(), "[1,0]");
}

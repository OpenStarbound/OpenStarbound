#include "StarLua.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(LuaJsonTest, Scope) {
  auto engine = LuaEngine::create();
  auto script1 = engine->compile(
      R"SCRIPT(
        function increment()
          self.called = self.called + 1
          return self.called
        end
      )SCRIPT");

  auto script2 = engine->compile(
      R"SCRIPT(
        global = 42

        function increment()
          self.called = self.called + 1
          return self.called
        end
      )SCRIPT");

  auto context1 = engine->createContext();
  auto context2 = engine->createContext();

  context1.load(script1);
  context2.load(script2);

  context1.setPath("self", JsonObject{{"called", 0}});
  context2.setPath("self", JsonObject{{"called", 0}});

  EXPECT_TRUE(context1.contains("self"));
  EXPECT_TRUE(context1.contains("increment"));

  EXPECT_EQ(context1.invokePath<Json>("increment"), 1);
  EXPECT_EQ(context1.invokePath<Json>("increment"), 2);
  EXPECT_EQ(context1.invokePath<Json>("increment"), 3);
  EXPECT_EQ(context2.invokePath<Json>("increment"), 1);
  EXPECT_EQ(context1.invokePath<Json>("increment"), 4);
  EXPECT_EQ(context2.invokePath<Json>("increment"), 2);

  auto context3 = engine->createContext();
  context3.load(script2);
  context3.setPath("self", JsonObject{{"called", 0}});

  EXPECT_EQ(context2.invokePath<Json>("increment"), 3);
  EXPECT_EQ(context3.invokePath<Json>("increment"), 1);
  EXPECT_EQ(context1.invokePath<Json>("increment"), 5);

  EXPECT_FALSE(context1.contains("global"));
  EXPECT_TRUE(context3.contains("global"));
}

TEST(LuaJsonTest, FunkyRecursion) {
  auto engine = LuaEngine::create();

  auto context1 = engine->createContext();
  context1.load(
      R"SCRIPT(
        mine = 1

        function util()
          mine = 1
          return mine
        end

        function util3()
          mine = 1
          return callbacks.util2()
        end
      )SCRIPT");

  auto context2 = engine->createContext();
  context2.load(
      R"SCRIPT(
        mine = 2

        function util2()
          return 4
        end

        function entry()
          local other = callbacks.util()
          return {other, mine}
        end

        function entry2()
          local other = callbacks.util2()
          return {other, mine}
        end

        function entry3()
          local other = callbacks.util3()
          return {other, mine}
        end
      )SCRIPT");

  LuaCallbacks callbacks;
  callbacks.registerCallback("util", [&context1]() -> Json { return context1.invokePath<Json>("util"); });
  callbacks.registerCallback("util2", [&context2]() -> Json { return context2.invokePath<Json>("util2"); });
  callbacks.registerCallback("util3", [&context1]() -> Json { return context1.invokePath<Json>("util3"); });

  context1.setCallbacks("callbacks", callbacks);
  context2.setCallbacks("callbacks", callbacks);

  auto res = context2.invokePath<Json>("entry");
  EXPECT_EQ(res.get(0), 1);
  EXPECT_EQ(res.get(1), 2);

  auto res2 = context2.invokePath<Json>("entry2");
  EXPECT_EQ(res2.get(0), 4);
  EXPECT_EQ(res2.get(1), 2);

  auto res3 = context2.invokePath<Json>("entry3");
  EXPECT_EQ(res3.get(0), 4);
  EXPECT_EQ(res3.get(1), 2);
}

TEST(LuaJsonTest, TypeConversion) {
  auto engine = LuaEngine::create();
  auto context = engine->createContext();
  context.load(
      R"SCRIPT(
        var1 = "1"
        var2 = {}
      )SCRIPT");

  EXPECT_EQ(context.getPath<String>("var1"), "1");
  EXPECT_EQ(context.getPath<double>("var1"), 1.0);
  EXPECT_EQ(context.getPath<JsonArray>("var2"), JsonArray());
  EXPECT_EQ(context.getPath<JsonObject>("var2"), JsonObject());
}

TEST(LuaJsonTest, ChunkBoundaries) {
  auto engine = LuaEngine::create();
  auto context = engine->createContext();

  context.load(
      R"SCRIPT(
        function func()
          return env.thing()
        end
      )SCRIPT");

  context.load(
      R"SCRIPT(
        env = {}

        function env.thing()
          local temp = {
            foo = extern.var
          }

          return temp.foo
        end
      )SCRIPT");

  context.setPath("extern", JsonObject{{"var", 1}});

  EXPECT_EQ(context.invokePath<int>("func"), 1);
}

TEST(LuaJsonTest, CustomObjectType) {
  auto engine = LuaEngine::create();
  auto context = engine->createContext();

  context.load(
      R"SCRIPT(
        function createObject()
          map = jobject()
          map.foo = 'hello'
          map.bar = nil
          map.baz = nil
          return map
        end

        function handleObject(arg)
          arg.bar = 'noodles'
          arg.test1 = jarray()
          arg.test2 = jobject()
          return arg
        end

        function iteratePairs()
          local map = jobject()
          map.foo = 1
          map.foo = nil
          map.bar = 1

          local keys = {}
          for key, val in pairs(map) do
            table.insert(keys, key)
          end
          return keys
        end

        function nilsRemoved()
          local map = jobject()
          map.foo = 1
          map.foo = nil
          return rawget(map, "foo")
        end

        function removeObject(arg, key)
          jremove(arg, key)
          return arg
        end
      )SCRIPT");

  JsonObject test = context.invokePath<JsonObject>("createObject");
  JsonObject comp = {{"foo", "hello"}, {"bar", Json()}, {"baz", Json()}};
  EXPECT_EQ(test, comp);

  test = context.invokePath<JsonObject>("handleObject", JsonObject{
      {"foo", JsonArray()},
      {"bar", Json()},
      {"baz", "hunky dory"},
      {"baf", Json()}
    });
  comp = {
    {"foo", JsonArray()},
    {"bar", "noodles"},
    {"baz", "hunky dory"},
    {"baf", Json()},
    {"test1", JsonArray()},
    {"test2", JsonObject()}
  };
  EXPECT_EQ(test, comp);

  JsonArray testArray = context.invokePath<JsonArray>("iteratePairs");
  JsonArray compArray = {"bar"};
  EXPECT_EQ(testArray, compArray);

  Json testValue = context.invokePath<Json>("nilsRemoved");
  EXPECT_EQ(testValue, Json());

  Json testValue2 =
      context.invokePath<Json>("removeObject", JsonObject{{"foo", 1}, {"bar", Json()}, {"baz", Json()}}, "bar");
  Json compValue2 = JsonObject{{"foo", 1}, {"baz", Json()}};
  EXPECT_EQ(testValue2, compValue2);
}

TEST(LuaJsonTest, CustomArrayType) {
  auto engine = LuaEngine::create();
  auto context = engine->createContext();

  context.load(
      R"SCRIPT(
        function createArray()
          list = jarray()
          list[1] = 1
          list[2] = 1
          list[3] = 1
          list[7] = 1
          list[12] = nil
          return list
        end

        function handleArray(arg)
          arg[1] = 'noodles'
          arg[4] = jarray()
          arg[9] = jobject()
          arg[10] = nil
          return arg
        end

        function iteratePairs()
          local list = jarray()
          list[1] = 1
          list[5] = nil
          list[4] = 1
          list[9] = nil

          local keys = {}
          for key, val in pairs(list) do
            table.insert(keys, key)
          end
          return keys
        end

        function resizeArray(arg, size)
          jresize(arg, size)
          return arg
        end

        function listSize(list)
          return jsize(list)
        end

        function listSize2()
          return jsize({1, 1, 1, 1, 1})
        end
      )SCRIPT");

  JsonArray test = context.invokePath<JsonArray>("createArray");
  JsonArray comp = {
      1, 1, 1, {}, {}, {}, 1, {}, {}, {}, {}, {},
  };
  EXPECT_EQ(test, comp);

  test = context.invokePath<JsonArray>("handleArray",
      JsonArray{
          2, {}, 5, 6, {}, {}, "omg",
      });
  comp = {
      "noodles", {}, 5, JsonArray{}, {}, {}, "omg", {}, JsonObject{}, {},
  };
  EXPECT_EQ(test, comp);

  JsonArray testArray = context.invokePath<JsonArray>("iteratePairs");
  JsonArray compArray = {1, 4};
  EXPECT_EQ(testArray, compArray);

  JsonArray testArray2 = context.invokePath<JsonArray>("resizeArray", JsonArray{1, 2, 3, 4, 5, Json(), Json(), 8}, 4);
  JsonArray compArray2 = {1, 2, 3, 4};
  EXPECT_EQ(testArray2, compArray2);

  JsonArray testArray3 = context.invokePath<JsonArray>("resizeArray", JsonArray{1, 2, 3, 4}, 6);
  JsonArray compArray3 = {1, 2, 3, 4, Json(), Json()};
  EXPECT_EQ(testArray3, compArray3);

  Json test4 = context.invokePath<Json>("listSize", JsonArray{1, 2, 3, 4, Json(), Json(), Json()});
  EXPECT_EQ(test4, 7);

  EXPECT_EQ(context.invokePath<Json>("listSize2"), Json(5));
}

TEST(LuaJsonTest, CustomArrayType2) {
  auto engine = LuaEngine::create();
  auto context = engine->createContext();

  context.load(
      R"SCRIPT(
        function doTest()
          sampleTable = jobject()
          sampleTable[18] = 0
          sampleTable[37] = 0

          targetTable = jobject()

          for k, v in pairs(sampleTable) do
            targetTable[k] = v
          end

          return targetTable
        end

        function arrayLogic1()
          l = {}
          l[1] = "foo"
          l[2] = "bar"
          return l
        end

        function arrayLogic2()
          l = {}
          l[1] = "foo"
          l[2.1] = "bar"
          return l
        end

        function arrayLogic3()
          l = jarray()
          l[1] = "foo"
          l[2] = "bar"
          return l
        end
      )SCRIPT");

  JsonObject test = context.invokePath<JsonObject>("doTest");
  JsonObject comp = {{"18", 0}, {"37", 0}};
  EXPECT_EQ(test, comp);

  Json arrayTest1 = context.invokePath<Json>("arrayLogic1");
  Json arrayComp1 = JsonArray{"foo", "bar"};
  EXPECT_EQ(arrayTest1, arrayComp1);

  Json arrayTest2 = context.invokePath<Json>("arrayLogic2");
  Json arrayComp2 = JsonObject{{"1", "foo"}, {"2.1", "bar"}};
  EXPECT_EQ(arrayTest2, arrayComp2);

  Json arrayTest3 = context.invokePath<Json>("arrayLogic3");
  Json arrayComp3 = JsonArray{"foo", "bar"};
  EXPECT_EQ(arrayTest3, arrayComp3);
}

TEST(LuaJsonTest, IntFloat) {
  auto engine = LuaEngine::create();
  auto context = engine->createContext();

  context.load(
      R"SCRIPT(
        function returnFloat()
          return 1.0
        end

        function returnInt()
          return 1
        end

        function printNumber(n)
          return tostring(n)
        end
      )SCRIPT");

  EXPECT_TRUE(context.invokePath<Json>("returnFloat").isType(Json::Type::Float));
  EXPECT_TRUE(context.invokePath<Json>("returnInt").isType(Json::Type::Int));
  EXPECT_EQ(context.invokePath<String>("printNumber", 1.0), "1.0");
  EXPECT_EQ(context.invokePath<String>("printNumber", 1), "1");
}

// Json containers turned into lua tables remember entries set to nil, and
// whether they started out as an array or an object.  These pin the behaviour
// of the conversion in both directions, including the nil bookkeeping.
TEST(LuaJsonTest, ContainerNilEntries) {
  auto engine = LuaEngine::create();
  auto context = engine->createContext();
  context.load(R"SCRIPT(
    function nilNew(j) j.zzz = nil; return j end
    function nilThenSet(j) j.zzz = nil; j.zzz = 3; return j end
    function nilExisting(j) j.a = nil; return j end
    function twice(j) j.q = nil; j.q = nil; j.r = nil; j.r = 1; return j end
    function arrayNil(j) j[2] = nil; return j end
    function arrayAppend(j) j[#j + 1] = 9; j[7] = nil; return j end
    function resize(j) jresize(j, 5); return j end
    function remove(j) jremove(j, 1); return j end
    function size(j) return {jsize(j), #j} end
    function nested(j) j.child.extra = nil; j.list[4] = "x"; return j end
  )SCRIPT");

  // Existing nulls survive the round trip
  Json withNulls = Json::parse(R"JSON({"a":1,"n":null,"list":[1,null,3],"child":{"k":null}})JSON");
  EXPECT_EQ(context.invokePath<Json>("nilNew", withNulls), Json::parse(R"JSON({"a":1,"n":null,"list":[1,null,3],"child":{"k":null},"zzz":null})JSON"));

  // Setting an absent key to nil is remembered, setting it again forgets that
  EXPECT_EQ(context.invokePath<Json>("nilThenSet", JsonObject()), Json::parse(R"JSON({"zzz":3})JSON"));
  EXPECT_EQ(context.invokePath<Json>("nilThenSet", Json::parse("[10,20,null,40]")), Json::parse(R"JSON({"1":10,"2":20,"3":null,"4":40,"zzz":3})JSON"));
  EXPECT_EQ(context.invokePath<Json>("twice", JsonObject()), Json::parse(R"JSON({"q":null,"r":1})JSON"));

  // Setting a key that exists to nil removes it, a missing one is remembered as null
  EXPECT_EQ(context.invokePath<Json>("nilExisting", Json::parse(R"JSON({"a":1,"b":2})JSON")), Json::parse(R"JSON({"b":2})JSON"));
  EXPECT_EQ(context.invokePath<Json>("nilExisting", JsonObject()), Json::parse(R"JSON({"a":null})JSON"));

  // Arrays stay arrays, with nils in place
  EXPECT_EQ(context.invokePath<Json>("arrayNil", Json::parse("[10,20,30]")), Json::parse("[10,null,30]"));
  EXPECT_EQ(context.invokePath<Json>("arrayAppend", JsonArray()), Json::parse("[9,null,null,null,null,null,null]"));
  EXPECT_EQ(context.invokePath<Json>("arrayAppend", JsonObject()), Json::parse(R"JSON({"1":9,"7":null})JSON"));
  EXPECT_EQ(context.invokePath<Json>("resize", Json::parse("[10,20,null,40,50,60]")), Json::parse("[10,20,null,40,50]"));
  EXPECT_EQ(context.invokePath<Json>("resize", JsonArray()), Json::parse("[null,null,null,null,null]"));
  EXPECT_EQ(context.invokePath<Json>("remove", Json::parse("[10,20,null,40]")), Json::parse("[null,20,null,40]"));
  EXPECT_EQ(context.invokePath<Json>("size", Json::parse("[10,20,null,40,50,60]")), Json::parse("[6,6]"));
  EXPECT_EQ(context.invokePath<Json>("size", JsonArray()), Json::parse("[0,0]"));

  // Nested containers keep their own bookkeeping
  EXPECT_EQ(context.invokePath<Json>("nested", Json::parse(R"JSON({"child":{"x":1},"list":[1,2,3]})JSON")),
      Json::parse(R"JSON({"child":{"x":1,"extra":null},"list":[1,2,3,"x"]})JSON"));
}

TEST(LuaJsonTest, ContainerTypeHints) {
  auto engine = LuaEngine::create();
  auto context = engine->createContext();
  context.load(R"SCRIPT(
    function same(j) return j end
    function newArray() return jarray() end
    function newObject() return jobject() end
    function wrapArray() local t = {1, 2, 3}; jarray(t); t[5] = nil; return t end
    function wrapObject() local t = {a = 1}; jobject(t); t.b = nil; return t end
    function plain() return {} end
    function plainList() return {1, 2, 3} end
    function copied(j) local c = {} for k, v in pairs(j) do c[k] = v end return c end
    function hint(j) return getmetatable(j).__typehint end
  )SCRIPT");

  // Empty containers come back as the kind they went in as
  EXPECT_EQ(context.invokePath<Json>("same", JsonArray()), Json(JsonArray()));
  EXPECT_EQ(context.invokePath<Json>("same", JsonObject()), Json(JsonObject()));
  EXPECT_EQ(context.invokePath<Json>("newArray"), Json(JsonArray()));
  EXPECT_EQ(context.invokePath<Json>("newObject"), Json(JsonObject()));
  EXPECT_EQ(context.invokePath<int>("hint", JsonArray()), 1);
  EXPECT_EQ(context.invokePath<int>("hint", JsonObject()), 2);

  // Existing tables can be turned into either kind
  EXPECT_EQ(context.invokePath<Json>("wrapArray"), Json::parse("[1,2,3,null,null]"));
  EXPECT_EQ(context.invokePath<Json>("wrapObject"), Json::parse(R"JSON({"a":1,"b":null})JSON"));

  // A plain lua table is a list if its keys are 1..n, otherwise an object
  EXPECT_EQ(context.invokePath<Json>("plain"), Json(JsonObject()));
  EXPECT_EQ(context.invokePath<Json>("plainList"), Json::parse("[1,2,3]"));

  // A copy made by a script has no hint and no nil entries
  EXPECT_EQ(context.invokePath<Json>("copied", Json::parse("[1,2,3]")), Json::parse("[1,2,3]"));
  EXPECT_EQ(context.invokePath<Json>("copied", Json::parse(R"JSON({"a":1,"b":[1,2]})JSON")), Json::parse(R"JSON({"a":1,"b":[1,2]})JSON"));
}

TEST(LuaJsonTest, ContainerRoundTripValues) {
  auto engine = LuaEngine::create();

  Json deep = Json::parse(R"JSON({
    "int": 5, "negative": -7, "float": 2.5, "big": 123456789012, "bool": false, "string": "text with \"quotes\" and unicode é",
    "array": [1, 2.5, "three", true, [4, 5], {"six": 6}],
    "object": {"nested": {"deeper": [0, {"deepest": null}]}},
    "empty array": [], "empty object": {}
  })JSON");

  EXPECT_EQ(engine->luaTo<Json>(engine->luaFrom<Json>(deep)), deep);

  // Integers stay integers and floats stay floats
  auto table = engine->luaFrom<Json>(Json::parse(R"JSON({"i": 3, "f": 3.0})JSON")).get<LuaTable>();
  EXPECT_TRUE(table.get<LuaValue>("i").is<LuaInt>());
  EXPECT_TRUE(table.get<LuaValue>("f").is<LuaFloat>());
}

TEST(LuaJsonTest, UnconvertibleTables) {
  auto engine = LuaEngine::create();
  auto context = engine->createContext();
  context.load(R"SCRIPT(
    function withFunction() return {1, 2, print} end
    function withFunctionNested() return {a = {b = print}} end
    function sparse() return {[1] = "a", [2] = "b", [5] = "c"} end
    function mixed() return {1, 2, a = 3} end
    function floatKeys() return {[1.5] = "x"} end
    function boolKey() local t = {} t[true] = 1 return t end
  )SCRIPT");

  EXPECT_THROW(context.invokePath<Json>("withFunction"), LuaException);
  EXPECT_THROW(context.invokePath<Json>("withFunctionNested"), LuaException);
  EXPECT_THROW(context.invokePath<Json>("boolKey"), LuaException);

  // Sparse lists and mixed tables become objects with integer keys as strings
  EXPECT_EQ(context.invokePath<Json>("mixed"), Json::parse(R"JSON({"1":1,"2":2,"a":3})JSON"));
  EXPECT_EQ(context.invokePath<Json>("sparse"), Json::parse(R"JSON({"1":"a","2":"b","5":"c"})JSON"));
  EXPECT_EQ(context.invokePath<Json>("floatKeys"), Json::parse(R"JSON({"1.5":"x"})JSON"));
}

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
          l["1"] = "foo"
          l["2"] = "bar"
          return l
        end

        function arrayLogic3()
          l = {}
          l["1"] = "foo"
          l["2.1"] = "bar"
          return l
        end

        function arrayLogic4()
          l = jarray()
          l["1"] = "foo"
          l["2"] = "bar"
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
  Json arrayComp2 = JsonArray{"foo", "bar"};
  EXPECT_EQ(arrayTest2, arrayComp2);

  Json arrayTest3 = context.invokePath<Json>("arrayLogic3");
  Json arrayComp3 = JsonObject{{"1", "foo"}, {"2.1", "bar"}};
  EXPECT_EQ(arrayTest2, arrayComp2);

  Json arrayTest4 = context.invokePath<Json>("arrayLogic4");
  Json arrayComp4 = JsonArray{"foo", "bar"};
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

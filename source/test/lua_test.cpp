#include "StarLua.hpp"
#include "StarLuaConverters.hpp"
#include "StarLexicalCast.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(LuaTest, BasicGetSet) {
  auto luaEngine = LuaEngine::create();
  LuaContext luaContext = luaEngine->createContext();
  luaContext.load(R"SCRIPT(
      data1 = 1.0
      data2 = 3.0 > 2.0
      data3 = "hello"
    )SCRIPT");

  luaContext.set("data4", 4.0);

  EXPECT_EQ(luaContext.get<double>("data1"), 1.0);
  EXPECT_EQ(luaContext.get<bool>("data2"), true);
  EXPECT_EQ(luaContext.get<String>("data3"), "hello");
  EXPECT_EQ(luaContext.get<double>("data4"), 4.0);
}

TEST(LuaTest, TableReferences) {
  auto luaEngine = LuaEngine::create();
  LuaContext luaContext = luaEngine->createContext();
  luaContext.load(R"SCRIPT(
      table = {foo=1, bar=2}
      tableRef = table
    )SCRIPT");

  auto table = luaContext.get<LuaTable>("table");
  auto tableRef1 = luaContext.get<LuaTable>("tableRef");
  auto tableRef2 = table;

  EXPECT_EQ(table.get<double>("foo"), 1.0);
  EXPECT_EQ(table.get<double>("bar"), 2.0);

  table.set("baz", 3.0);
  EXPECT_EQ(tableRef1.get<double>("baz"), 3.0);

  tableRef1.set("baf", 4.0);
  EXPECT_EQ(table.get<double>("baf"), 4.0);
  EXPECT_EQ(tableRef2.get<double>("baf"), 4.0);
}

TEST(LuaTest, FunctionCallTest) {
  weak_ptr<int> destructionObserver;

  {
    auto luaEngine = LuaEngine::create();
    LuaContext luaContext = luaEngine->createContext();
    luaContext.load(R"SCRIPT(
        function testFunc(arg1, arg2)
          return callback(3) + arg1 + arg2
        end

        function testEmpty()
          return emptyCallback()
        end
      )SCRIPT");

    auto toDestruct = make_shared<int>();
    destructionObserver = toDestruct;
    luaContext.set("callback", luaEngine->createFunction([toDestruct](double n) { return n * 2; }));

    luaContext.set("emptyCallback", luaEngine->createFunction([]() { return "heyooo"; }));

    EXPECT_EQ(luaContext.invokePath<double>("testFunc", 5.0, 10.0), 21.0);
    EXPECT_EQ(luaContext.invokePath<String>("emptyCallback"), "heyooo");
  }

  EXPECT_TRUE(destructionObserver.expired());
}

TEST(LuaTest, CoroutineTest) {
  auto luaEngine = LuaEngine::create();
  LuaContext luaContext = luaEngine->createContext();

  luaContext.load(R"SCRIPT(
      function accumulate(sum)
        return sum + callback(coroutine.yield(sum))
      end

      function run()
          local sum = 0
          for i=1,4 do
            sum = accumulate(sum)
          end
          return sum
      end

      co = coroutine.create(run)
    )SCRIPT");

  luaContext.set("callback", luaEngine->createFunction([](double num) { return num * 2; }));

  LuaThread thread = luaEngine->createThread();
  EXPECT_EQ(thread.status(), LuaThread::Status::Dead);
  LuaFunction func = luaContext.get<LuaFunction>("run");
  thread.pushFunction(func);
  EXPECT_EQ(thread.status(), LuaThread::Status::Active);
  EXPECT_EQ(thread.resume<double>(), 0.0);
  EXPECT_EQ(thread.resume<double>(1.0), 2.0);
  EXPECT_EQ(thread.resume<double>(3.0), 8.0);
  EXPECT_EQ(thread.resume<double>(5.0), 18.0);
  EXPECT_EQ(thread.resume<double>(7.0), 32.0);
  // manually created threads are empty after execution is finished
  EXPECT_EQ(thread.status(), LuaThread::Status::Dead);

  thread.pushFunction(func);
  EXPECT_EQ(thread.resume<double>(), 0.0);
  EXPECT_EQ(thread.resume<double>(1.0), 2.0);
  EXPECT_THROW(thread.pushFunction(func), LuaException); // pushing function to suspended or errored thread

  auto coroutine = luaContext.get<LuaThread>("co");
  EXPECT_EQ(coroutine.status(), LuaThread::Status::Active);
  EXPECT_EQ(coroutine.resume<double>(), 0.0);
  EXPECT_EQ(coroutine.resume<double>(1.0), 2.0);
  EXPECT_EQ(coroutine.resume<double>(3.0), 8.0);
  EXPECT_EQ(coroutine.resume<double>(5.0), 18.0);
  EXPECT_EQ(coroutine.resume<double>(7.0), 32.0);
  EXPECT_EQ(coroutine.status(), LuaThread::Status::Dead);
  EXPECT_THROW(coroutine.resume(), LuaException);
  EXPECT_EQ(coroutine.status(), LuaThread::Status::Dead);

  LuaThread thread2 = luaEngine->createThread();
  EXPECT_EQ(thread2.status(), LuaThread::Status::Dead);
  thread2.pushFunction(func);
  EXPECT_EQ(thread2.status(), LuaThread::Status::Active);
  EXPECT_EQ(thread2.resume<double>(), 0.0);
  EXPECT_EQ(thread2.resume<double>(1.0), 2.0);
  EXPECT_THROW(thread2.resume<String>("not_a_number"), LuaException);
  EXPECT_EQ(thread2.status(), LuaThread::Status::Error);
  EXPECT_THROW(thread2.resume(), LuaException);
  EXPECT_EQ(thread2.status(), LuaThread::Status::Error);
}

template <typename T>
bool roundTripEqual(LuaContext const& context, T t) {
  return context.invokePath<T>("roundTrip", t) == t;
}

TEST(LuaTest, Converters) {
  auto luaEngine = LuaEngine::create();
  LuaContext luaContext = luaEngine->createContext();

  luaContext.load( R"SCRIPT(
      function makeVec()
        return {1, 2}
      end

      function makePoly()
        return {{1, 2}, {3, 4}, {5, 6}}
      end

      function roundTrip(ret)
        return ret
      end
    )SCRIPT");

  auto vecCompare = Vec2F(1.0f, 2.0f);
  auto polyCompare = PolyF({Vec2F(1.0f, 2.0f), Vec2F(3.0f, 4.0f), Vec2F(5.0f, 6.0f)});

  EXPECT_TRUE(luaContext.invokePath<Vec2F>("makeVec") == vecCompare);
  EXPECT_TRUE(luaContext.invokePath<PolyF>("makePoly") == polyCompare);
  EXPECT_TRUE(luaContext.invokePath<Vec2F>("roundTrip", vecCompare) == vecCompare);
  EXPECT_TRUE(luaContext.invokePath<PolyF>("roundTrip", polyCompare) == polyCompare);

  EXPECT_TRUE(roundTripEqual(luaContext, PolyF()));
  EXPECT_TRUE(roundTripEqual(luaContext, List<int>{1, 2, 3, 4}));
  EXPECT_TRUE(roundTripEqual(luaContext, List<PolyF>{PolyF(), PolyF()}));
  EXPECT_TRUE(roundTripEqual(luaContext, Maybe<int>(1)));
  EXPECT_TRUE(roundTripEqual(luaContext, Maybe<int>()));

  auto listCompare = List<int>{1, 2, 3, 4};
  EXPECT_TRUE(luaContext.invokePath<List<int>>("roundTrip", JsonArray{1, 2, 3, 4}) == listCompare);
  auto mapCompare = StringMap<String>{{"one", "two"}, {"three", "four"}};
  EXPECT_TRUE(luaContext.invokePath<StringMap<String>>("roundTrip", JsonObject{{"one", "two"}, {"three", "four"}}) == mapCompare);
}

struct TestUserData1 {
  int field;
};

struct TestUserData2 {
  int field;
};

namespace Star {
template <>
struct LuaConverter<TestUserData1> : LuaUserDataConverter<TestUserData1> {};

template <>
struct LuaConverter<TestUserData2> : LuaUserDataConverter<TestUserData2> {};
}

TEST(LuaTest, UserDataTest) {
  auto luaEngine = LuaEngine::create();

  LuaContext luaContext = luaEngine->createContext();
  luaContext.load(
      R"SCRIPT(
        function doit(ref)
          global = ref
        end
      )SCRIPT");

  auto userdata1 = luaEngine->createUserData(TestUserData1{1});
  auto userdata2 = luaEngine->createUserData(TestUserData2{2});

  luaContext.invokePath("doit", userdata1);
  auto userdata3 = luaContext.get<LuaUserData>("global");

  EXPECT_TRUE(userdata2.is<TestUserData2>());
  EXPECT_FALSE(userdata2.is<TestUserData1>());

  EXPECT_TRUE(userdata1.is<TestUserData1>());
  EXPECT_FALSE(userdata1.is<TestUserData2>());

  EXPECT_TRUE(userdata3.is<TestUserData1>());
  EXPECT_FALSE(userdata3.is<TestUserData2>());

  EXPECT_EQ(userdata1.get<TestUserData1>().field, 1);
  EXPECT_EQ(userdata2.get<TestUserData2>().field, 2);
  EXPECT_EQ(userdata3.get<TestUserData1>().field, 1);

  userdata1.get<TestUserData1>().field = 3;
  EXPECT_EQ(userdata1.get<TestUserData1>().field, 3);
  EXPECT_EQ(userdata3.get<TestUserData1>().field, 3);

  luaContext.invokePath("doit", TestUserData1());
  auto userdata4 = luaContext.get("global");
  EXPECT_TRUE(luaEngine->luaMaybeTo<TestUserData1>(userdata4).isValid());

  luaContext.invokePath("doit", "notuserdata");
  auto notuserdata = luaContext.get("global");
  EXPECT_FALSE(luaEngine->luaMaybeTo<TestUserData1>(notuserdata).isValid());
}

namespace Star {
template <>
struct LuaConverter<Vec3F> : LuaUserDataConverter<Vec3F> {};

template <>
struct LuaUserDataMethods<Vec3F> {
  static LuaMethods<Vec3F> make() {
    LuaMethods<Vec3F> methods;
    methods.registerMethodWithSignature<float, Vec3F const&>("magnitude", mem_fn(&Vec3F::magnitude));
    return methods;
  }
};
}

TEST(LuaTest, UserMethodTest) {
  auto luaEngine = LuaEngine::create();
  luaEngine->setGlobal("vec3", luaEngine->createFunctionWithSignature<Vec3F, float, float, float>(construct<Vec3F>()));

  LuaContext luaContext = luaEngine->createContext();
  luaContext.load(R"SCRIPT(
      v = vec3(3, 2, 1)
      function testMagnitude(v2)
        return v:magnitude() + v2:magnitude()
      end
    )SCRIPT");

  float magnitude = luaContext.invokePath<float>("testMagnitude", Vec3F(5, 5, 5));
  EXPECT_TRUE(fabs(magnitude - (Vec3F(3, 2, 1).magnitude() + Vec3F(5, 5, 5).magnitude())) < 0.00001f);
}

TEST(LuaTest, GlobalTest) {
  auto luaEngine = LuaEngine::create();
  luaEngine->setGlobal("globalfoo", LuaInt(42));
  EXPECT_EQ(luaEngine->getGlobal("globalfoo"), LuaInt(42));

  LuaContext luaContext = luaEngine->createContext();
  luaContext.load(R"SCRIPT(
      function test()
        return globalfoo
      end
    )SCRIPT");

  EXPECT_EQ(luaContext.invokePath("test"), LuaInt(42));
}

TEST(LuaTest, ArgTest) {
  auto luaEngine = LuaEngine::create();

  LuaContext luaContext = luaEngine->createContext();
  luaContext.load(R"SCRIPT(
      function test()
        callback("2", 3, nil)
      end
    )SCRIPT");

  luaContext.set("callback",
      luaEngine->createFunction([](LuaFloat n, LuaString s, LuaBoolean b, LuaValue o) {
        EXPECT_EQ(n, 2.0);
        EXPECT_EQ(s, String("3"));
        EXPECT_EQ(b, false);
        EXPECT_EQ(o, LuaNil);
      }));

  luaContext.invokePath("test");
}

TEST(LuaTest, ArrayTest) {
  auto luaEngine = LuaEngine::create();
  LuaContext luaContext = luaEngine->createContext();
  luaContext.load(R"SCRIPT(
      function test()
        return {2, 4, 6, 8, 10}
      end
    )SCRIPT");

  auto arrayTable = luaContext.invokePath("test").get<LuaTable>();

  EXPECT_EQ(arrayTable.length(), 5);
  EXPECT_EQ(arrayTable.get(2), LuaInt(4));
  EXPECT_EQ(arrayTable.get(5), LuaInt(10));

  List<pair<int, int>> values;
  arrayTable.iterate([&values](LuaValue const& key, LuaValue const& value) {
    auto ikey = key.get<LuaInt>();
    auto ivalue = value.get<LuaInt>();
    values.append({ikey, ivalue});
  });
  List<pair<int, int>> compare = {{1, 2}, {2, 4}, {3, 6}, {4, 8}, {5, 10}};
  EXPECT_EQ(values, compare);
}

TEST(LuaTest, PathTest) {
  auto luaEngine = LuaEngine::create();
  LuaContext luaContext = luaEngine->createContext();
  luaContext.load(R"SCRIPT(
      foo = {
          bar = {
            baz = 1
          }
        }

      function test()
        return foo.bar.baf
      end
    )SCRIPT");

  EXPECT_EQ(luaContext.containsPath("foo.bar.baz"), true);
  EXPECT_EQ(luaContext.getPath("foo.bar.baz"), LuaInt(1));
  EXPECT_EQ(luaContext.containsPath("foo.nothing.at.all"), false);

  luaContext.setPath("foo.bar.baf", LuaInt(5));
  EXPECT_EQ(luaContext.invokePath("test"), LuaInt(5));

  luaContext.setPath("new.table.value", LuaInt(5));
  EXPECT_EQ(luaContext.getPath("new.table.value"), LuaInt(5));
}

TEST(LuaTest, CallbackTest) {
  auto luaEngine = LuaEngine::create();

  LuaCallbacks callbacks;
  callbacks.registerCallback("add", [](LuaInt a, LuaInt b) { return a + b; });
  callbacks.registerCallbackWithSignature<LuaInt, LuaInt, LuaInt>("subtract", [](int a, int b) { return a - b; });
  callbacks.registerCallbackWithSignature<LuaInt, LuaInt>("multiply2", bind([](int a, int b) { return a * b; }, 2, _1));
  callbacks.registerCallbackWithSignature<void, LuaValue>("nothing", [](LuaValue v) { return v; });

  LuaContext luaContext = luaEngine->createContext();
  luaContext.setCallbacks("callbacks", callbacks);
  luaContext.load(R"SCRIPT(
      function test1()
        return callbacks.multiply2(callbacks.add(5, 10) + callbacks.subtract(3, 10))
      end

      function test2()
        return callbacks.nothing(1)
      end
    )SCRIPT");

  EXPECT_EQ(luaContext.invokePath("test1").get<LuaInt>(), 16);
  EXPECT_EQ(luaContext.invokePath("test2"), LuaNil);
}

TEST(LuaTest, VariableParameters) {
  auto luaEngine = LuaEngine::create();
  auto context1 = luaEngine->createContext();

  context1.load(R"SCRIPT(
      function variableArgsCount(...)
        local arg = {...}
        return #arg
      end
    )SCRIPT");

  auto res1 = context1.invokePath<int>("variableArgsCount");
  auto res2 = context1.invokePath<int>("variableArgsCount", 1);
  auto res3 = context1.invokePath<int>("variableArgsCount", 1, 1);
  auto res4 = context1.invokePath<int>("variableArgsCount", 1, 1, 1);

  EXPECT_EQ(res1, 0);
  EXPECT_EQ(res2, 1);
  EXPECT_EQ(res3, 2);
  EXPECT_EQ(res4, 3);
}

TEST(LuaTest, Scope) {
  auto luaEngine = LuaEngine::create();
  auto script1 = luaEngine->compile(R"SCRIPT(
      function create(param)
        local self = {}
        local foo = param

        local getValue = function()
          return foo
        end

        function self.get()
          return getValue()
        end

        return self
      end
    )SCRIPT");

  auto script2 = luaEngine->compile(R"SCRIPT(
      function init()
        obj = create(param)
      end

      function produce()
        return obj.get()
      end
    )SCRIPT");

  auto context1 = luaEngine->createContext();
  context1.load(script1);
  context1.load(script2);

  auto context2 = luaEngine->createContext();
  context2.load(script1);
  context2.load(script2);

  context1.setPath("param", 1);
  context1.invokePath("init");

  context2.setPath("param", 2);
  context2.invokePath("init");

  EXPECT_EQ(context1.invokePath<int>("produce"), 1);
  EXPECT_EQ(context2.invokePath<int>("produce"), 2);

  context1.setPath("param", 2);
  context1.invokePath("init");

  context2.setPath("param", 1);
  context2.invokePath("init");

  EXPECT_EQ(context1.invokePath<int>("produce"), 2);
  EXPECT_EQ(context2.invokePath<int>("produce"), 1);
}

TEST(LuaTest, Scope2) {
  auto luaEngine = LuaEngine::create();

  auto context1 = luaEngine->createContext();
  context1.load(R"SCRIPT(
      function init1()
        global = {}
        global.val = 10
      end
    )SCRIPT");

  auto context2 = luaEngine->createContext();
  context2.load(R"SCRIPT(
      function init2()
        global = {}
        global.val = 20
      end
    )SCRIPT");

  EXPECT_TRUE(context1.contains("init1"));
  EXPECT_TRUE(context2.contains("init2"));
  EXPECT_FALSE(context1.contains("init2"));
  EXPECT_FALSE(context2.contains("init1"));

  context1.invokePath("init1");
  EXPECT_EQ(context1.getPath<int>("global.val"), 10);

  EXPECT_TRUE(context2.getPath("global") == LuaNil);

  context2.invokePath("init2");
  EXPECT_EQ(context2.getPath<int>("global.val"), 20);

  EXPECT_EQ(context1.getPath<int>("global.val"), 10);
}

TEST(LuaTest, MetaTable) {
  auto luaEngine = LuaEngine::create();

  auto context = luaEngine->createContext();
  context.load(R"SCRIPT(
      function add(a, b)
        return a + b
      end
    )SCRIPT");

  auto mt = luaEngine->createTable();
  mt.set("__add",
      luaEngine->createFunction([](LuaEngine& engine, LuaTable const& a, LuaTable const& b) {
        return engine.createArrayTable(
            initializer_list<double>{a.get<double>(1) + b.get<double>(1), a.get<double>(2) + b.get<double>(2)});
      }));
  mt.set("test", "hello");

  auto t1 = luaEngine->createArrayTable(initializer_list<double>{1, 2});
  t1.setMetatable(mt);

  auto t2 = luaEngine->createArrayTable(initializer_list<double>{5, 6});
  t2.setMetatable(mt);

  auto tr = context.invokePath<LuaTable>("add", t1, t2);
  EXPECT_EQ(tr.get<double>(1), 6);
  EXPECT_EQ(tr.get<double>(2), 8);
  EXPECT_EQ(t1.getMetatable()->get<String>("test"), "hello");
  EXPECT_EQ(t2.getMetatable()->get<String>("test"), "hello");
}

TEST(LuaTest, Integers) {
  auto luaEngine = LuaEngine::create();
  auto context = luaEngine->createContext();
  context.load(R"SCRIPT(
      n1 = 0
      n2 = 1
      n3 = 1.0
      n4 = 1.1
      n5 = 5.0
      n6 = 5
    )SCRIPT");

  EXPECT_EQ(context.get("n1"), LuaInt(0));
  EXPECT_EQ(context.get("n2"), LuaInt(1));
  EXPECT_EQ(context.get("n3"), LuaFloat(1.0));
  EXPECT_EQ(context.get("n4"), LuaFloat(1.1));
  EXPECT_EQ(context.get("n5"), LuaFloat(5.0));
  EXPECT_EQ(context.get("n6"), LuaInt(5));
}

TEST(LuaTest, Require) {
  auto luaEngine = LuaEngine::create();
  auto context = luaEngine->createContext();
  context.setRequireFunction([](LuaContext& context, LuaString const& arg) {
      context.set(arg, context.createFunction([arg]() {
          return arg;
        }));
    });

  context.load(R"SCRIPT(
      require "a"
      require "b"
      require "c"

      function res()
        return a() .. b() .. c()
      end
    )SCRIPT");

  EXPECT_EQ(context.invokePath<LuaString>("res"), String("abc"));
}

TEST(LuaTest, Eval) {
  auto luaEngine = LuaEngine::create();
  auto context = luaEngine->createContext();

  context.eval("i = 3");
  // Make sure statements and expressions both work in eval.
  EXPECT_EQ(context.eval<int>("i + 1"), 4);
  EXPECT_EQ(context.eval<int>("return i + 1"), 4);
}

TEST(LuaTest, Multi) {
  auto luaEngine = LuaEngine::create();
  auto script = luaEngine->compile(R"SCRIPT(
      function entry()
        return callbacks.func(2, 4)
      end

      function sum(...)
        local sum = 0
        for i,v in ipairs(arg) do
          sum = sum + v
        end
        return sum
      end
    )SCRIPT");

  auto context1 = luaEngine->createContext();
  auto context2 = luaEngine->createContext();
  auto context3 = luaEngine->createContext();

  context1.load(script);
  context2.load(script);
  context3.load(script);

  LuaCallbacks addCallbacks;
  addCallbacks.registerCallback("func",
      [](LuaVariadic<int> const& args) -> int {
        int sum = 0.0;
        for (auto arg : args)
          sum += arg;
        return sum;
      });

  LuaCallbacks multCallbacks;
  multCallbacks.registerCallback("func",
      [](LuaVariadic<int> const& args) -> int {
        int mult = 1.0;
        for (auto arg : args)
          mult *= arg;
        return mult;
      });

  context1.setCallbacks("callbacks", addCallbacks);
  context2.setCallbacks("callbacks", multCallbacks);
  context3.setCallbacks("callbacks", addCallbacks);

  EXPECT_EQ(context1.invokePath<int>("entry"), 6);
  EXPECT_EQ(context2.invokePath<int>("entry"), 8);
  EXPECT_EQ(context3.invokePath<int>("entry"), 6);
  EXPECT_EQ(context1.invokePath<int>("entry"), 6);
  EXPECT_EQ(context2.invokePath<int>("entry"), 8);
  EXPECT_EQ(context3.invokePath<int>("entry"), 6);
  EXPECT_EQ(context1.invokePath<int>("entry"), 6);

  auto context4 = luaEngine->createContext();
  context4.load(R"SCRIPT(
      function sum(...)
        local args = {...}
        local sum = 0
        for i = 1, #args do
          sum = sum + args[i]
        end
        return sum
      end

      function mreturn(...)
        return ...
      end

      function callbacktest(...)
        local x, y = callback()
        return x, y
      end

      function emptycallbacktest(...)
        return emptycallback()
      end
    )SCRIPT");
  EXPECT_EQ(context4.invokePath<int>("sum", LuaVariadic<int>{1, 2, 3}), 6);
  EXPECT_EQ(context4.invokePath<int>("sum", 5, LuaVariadic<int>{1, 2, 3}, 10), 21);
  EXPECT_EQ(context4.invokePath<LuaVariadic<int>>("mreturn", 1, 2, 3), LuaVariadic<int>({1, 2, 3}));

  int a;
  float b;
  String c;
  luaTie(a, b, c) = context4.invokePath<LuaTupleReturn<int, float, String>>("mreturn", 1, 2.0f, "foo");
  EXPECT_EQ(a, 1);
  EXPECT_EQ(b, 2.0f);
  EXPECT_EQ(c, "foo");

  context4.set("callback", context4.createFunction([]() { return luaTupleReturn(5, 10); }));

  context4.set("emptycallback", context4.createFunction([]() { return luaTupleReturn(); }));

  int d;
  int e;
  luaTie(d, e) = context4.invokePath<LuaTupleReturn<int, int>>("callbacktest");
  EXPECT_EQ(d, 5);
  EXPECT_EQ(e, 10);

  EXPECT_EQ(context4.invokePath("emptycallbacktest"), LuaNil);
}

TEST(LuaTest, Limits) {
  auto luaEngine = LuaEngine::create();
  luaEngine->setInstructionLimit(500000);
  luaEngine->setRecursionLimit(64);
  auto context = luaEngine->createContext();
  context.load(R"SCRIPT(
      function toinfinityandbeyond()
        while true do
        end
      end

      function toabignumberandthenstop()
        for i = 0, 50000 do
        end
      end
    )SCRIPT");

  // Make sure infinite loops trigger the instruction limit.
  EXPECT_THROW(context.invokePath("toinfinityandbeyond"), LuaInstructionLimitReached);

  // Make sure the instruction count is reset after each call.
  context.invokePath("toabignumberandthenstop");

  auto infLoop = R"SCRIPT(
        while true do
        end
      )SCRIPT";

  // Make sure loading code into context with infinite loops in their
  // evaluation triggers instruction limit.
  EXPECT_THROW(context.load(infLoop), LuaInstructionLimitReached);

  // And the same for eval
  EXPECT_THROW(context.eval(infLoop), LuaInstructionLimitReached);

  auto call1 = [&context]() { context.invokePath("call2"); };

  auto call2 = [&context]() { context.invokePath("call1"); };

  context.set("call1", context.createFunction(call1));
  context.set("call2", context.createFunction(call2));

  EXPECT_THROW(context.invokePath("call1"), LuaRecursionLimitReached);

  // Make sure the context still functions properly after these previous
  // errors.
  EXPECT_EQ(context.eval<int>("1 + 1"), 2);
}

TEST(LuaTest, Errors) {
  auto luaEngine = LuaEngine::create();
  auto context = luaEngine->createContext();

  EXPECT_THROW(context.eval("while true do"), LuaIncompleteStatementException);
  context.setPath("val", 1.0);
  EXPECT_THROW(context.getPath<Vec2D>("val"), LuaConversionException);
  EXPECT_EQ(luaEngine->luaMaybeTo<RectF>(context.get("val")), Maybe<RectF>());

  context.set("throwException", luaEngine->createFunction([]() {
      throw StarException("lua caught the exception!");
    }));

  context.load(R"SCRIPT(
      function throwError()
        return throwException()
      end
      function catchError()
        return pcall(throwException)
      end
    )SCRIPT");

  EXPECT_THROW(context.invokePath("throwError"), LuaException);

  bool status;
  LuaValue error;
  luaTie(status, error) = context.invokePath<LuaTupleReturn<bool, LuaValue>>("catchError");
  EXPECT_EQ(status, false);
  EXPECT_TRUE(String(toString(error)).contains("lua caught the exception"));
}

TEST(LuaTest, GarbageCollection) {
  auto engine = LuaEngine::create();
  auto context = engine->createContext();

  auto ptr = make_shared<int>(5);
  engine->setAutoGarbageCollection(false);
  context.setPath("ref", context.createUserData(ptr));
  EXPECT_EQ(ptr.use_count(), 2);
  context.setPath("ref", LuaNil);
  EXPECT_EQ(ptr.use_count(), 2);
  engine->collectGarbage();
  EXPECT_EQ(ptr.use_count(), 1);
}

TEST(LuaTest, IntTest) {
  auto engine = LuaEngine::create();
  auto context = engine->createContext();
  context.setPath("test", (uint64_t)-1);
  EXPECT_EQ(context.getPath<uint64_t>("test"), (uint64_t)-1);
}

TEST(LuaTest, VariantTest) {
  auto engine = LuaEngine::create();
  auto context = engine->createContext();

  typedef Variant<int, String> IntOrString;

  EXPECT_EQ(context.eval<IntOrString>("'foo'"), IntOrString(String("foo")));
  EXPECT_EQ(context.eval<IntOrString>("'1'"), IntOrString(1));

  typedef MVariant<Maybe<int>, String> MIntOrString;

  EXPECT_EQ(context.eval<MIntOrString>("'foo'"), MIntOrString(String("foo")));
  EXPECT_EQ(context.eval<MIntOrString>("'1'"), MIntOrString(Maybe<int>(1)));
  EXPECT_EQ(context.eval<MIntOrString>("nil"), MIntOrString());
}

TEST(LuaTest, ProfilingTest) {
  auto luaEngine = LuaEngine::create();
  luaEngine->setProfilingEnabled(true);
  luaEngine->setInstructionMeasureInterval(1000);

  auto context = luaEngine->createContext();
  context.eval(R"SCRIPT(
      function function1()
        for i = 1, 1000 do
        end
      end

      function function2()
        for i = 1, 1000 do
        end
      end

      function function3()
        for i = 1, 1000 do
        end
      end

      for i = 1, 10000 do
        function1()
        function2()
        function3()
      end
    )SCRIPT");

  StringSet names;
  List<LuaProfileEntry> profile = luaEngine->getProfile();
  for (auto const& p : profile[0].calls)
    names.add(p.second->name.value());

  EXPECT_TRUE(names.contains("function1"));
  EXPECT_TRUE(names.contains("function2"));
  EXPECT_TRUE(names.contains("function3"));
}

#include "StarString.hpp"
#include "StarFormat.hpp"

#include "gtest/gtest.h"

using namespace Star;

TEST(StringTest, substr) {
  EXPECT_EQ(String("barbazbaffoo").substr(4, 4), String("azba"));
  EXPECT_EQ(String("\0asdf", 5).substr(1, 2), String("as"));
}

TEST(StringTest, find) {
  EXPECT_EQ(String("xxFooxx").find("Foo"), 2u);
  EXPECT_EQ(String("xxFooxx").find("foo"), NPos);
  EXPECT_EQ(String("xxFooxx").find("foo", 0, String::CaseInsensitive), 2u);
  EXPECT_EQ(String("xxFooxx").find("bar", 0, String::CaseInsensitive), NPos);
  EXPECT_EQ(String("BAR baz baf BAR").find("bar", 1, String::CaseInsensitive), 12u);
  EXPECT_EQ(String("\0asdf", 5).find("df"), 3u);
}

TEST(StringTest, split_join) {
  EXPECT_EQ(String("語語日語語日語語").split("日").join("_"), "語語_語語_語語");
  EXPECT_EQ(String("日語語日語語日語語日").split("日").join("_"), "_語語_語語_語語_");
  EXPECT_EQ(String("aabaabaa").split('b').join("_"), "aa_aa_aa");
  EXPECT_EQ(String("baabaabaab").split('b').join("_"), "_aa_aa_aa_");
  EXPECT_EQ(String("a").split("bcd"), StringList{"a"});
  EXPECT_EQ(String("").split("bcd"), StringList{""});
  EXPECT_EQ(String("\n\raa\n\raa\r\n\r\naa\r\n\r\n\r\n").splitAny("\r\n"), StringList(3, "aa"));
  EXPECT_EQ(String("\n\r\n\r\r\n\r\n\r\n\r\n\r\n").splitAny("\r\n"), StringList());
  EXPECT_EQ(String("").splitAny("\r\n"), StringList());
  EXPECT_EQ(String("xyxFoo").splitAny("x", 1).join("_"), "y_Foo");
  EXPECT_EQ(String("xyxFoo").rsplitAny("x", 1).join("_"), "xy_Foo");
  EXPECT_EQ(String("xyxFooxFoox").rsplitAny("x", 1).join("_"), "xyxFoo_Foo");
  EXPECT_EQ(String("x").rsplitAny("x"), StringList());
  EXPECT_EQ(String("x").splitAny("x", 1), StringList());
  EXPECT_EQ(String("").splitAny("x", 1), StringList());
  EXPECT_EQ(String("asdf\0asdf", 9).splitAny(String("\0", 1)), StringList(2, "asdf"));
  EXPECT_EQ(String("asdf\0asdf", 9).splitAny("a"), StringList({String("sdf\0", 4), String("sdf")}));
}

TEST(StringTest, replace) {
  EXPECT_EQ(String("x").replace("cdc", "foo"), "x");
  EXPECT_EQ(String("cdcdcdc").replace("cdc", "foo"), "foodfoo");
  EXPECT_EQ(String("").replace("cdc", "foo"), String());
  EXPECT_EQ(String("xxx").replace("x", "xx"), "xxxxxx");
  EXPECT_EQ(String("/bin/bash\0aaa:123123:123", 24).replace(String("\0", 1), ""), String("/bin/bashaaa:123123:123"));
  EXPECT_EQ(String("/bin/bash\0aaa:123123:123", 24).replace(String(), ""), String("/bin/bash\0aaa:123123:123", 24));
}

TEST(StringTest, endsWith) {
  EXPECT_EQ(String("something.com").endsWith(".com"), true);
  EXPECT_EQ(String("something.com").endsWith("fsomething.com"), false);
  EXPECT_EQ(String("something.com").endsWith(""), true);
  EXPECT_EQ(String("something.com").endsWith("SOMETHING.COMF", String::CaseInsensitive), false);
  EXPECT_EQ(String("something.com").endsWith('M', String::CaseInsensitive), true);
  EXPECT_EQ(String("something.com").endsWith('F', String::CaseInsensitive), false);
  EXPECT_EQ(String("").endsWith('f'), false);
  EXPECT_EQ(String("something.com\0", 14).endsWith("m"), false);
  EXPECT_EQ(String("s\0omething.com", 14).endsWith("m"), true);
  EXPECT_EQ(String("s\0omething.com", 14).endsWith("s"), false);
}

TEST(StringTest, beginsWith) {
  EXPECT_EQ(String("something.com").beginsWith("something"), true);
  EXPECT_EQ(String("something.com").beginsWith("something.comf"), false);
  EXPECT_EQ(String("something.com").beginsWith(""), true);
  EXPECT_EQ(String("something.com").beginsWith("FSOMETHING.COM", String::CaseInsensitive), false);
  EXPECT_EQ(String("something.com").beginsWith('S', String::CaseInsensitive), true);
  EXPECT_EQ(String("something.com").beginsWith('F', String::CaseInsensitive), false);
  EXPECT_EQ(String("").beginsWith('s'), false);
  EXPECT_EQ(String("\0something.com", 14).beginsWith(String("\0", 1)), true);
}

TEST(StringTest, trim) {
  EXPECT_EQ(String("").trim(), String());
  EXPECT_EQ(String("   ").trim(), String());
  EXPECT_EQ(String(" \t ").trim(), String());
  EXPECT_EQ(String("   something   ").trim(), "something");
  EXPECT_EQ(String("something").trim(), "something");
  EXPECT_EQ(String("\tsomething\t\t  \t").trim(), "something");

  EXPECT_EQ(String("thththsomethingthththt").trim("th"), "something");
  EXPECT_EQ(String("mmmmmmsomethingmmmmmmm").trim("m"), "something");
  EXPECT_EQ(String("\tsomething\t\t\t").trim("\t"), "something");
  EXPECT_EQ(String("\0something\0\0\0", 13).trim(String("\0", 1)), "something");
}

TEST(StringTest, extract) {
  String test("xxxfooxxxfooxxxfooxxxbarxxx");
  EXPECT_EQ(test.rextract("x"), "bar");
  EXPECT_EQ(test, "xxxfooxxxfooxxxfoo");
  EXPECT_EQ(test.rextract("x"), "foo");
  EXPECT_EQ(test, "xxxfooxxxfoo");
  EXPECT_EQ(test.rextract("x"), "foo");
  EXPECT_EQ(test, "xxxfoo");
  EXPECT_EQ(test.rextract("x"), "foo");
  EXPECT_EQ(test, "");
}

TEST(StringTest, reverse) {
  EXPECT_EQ(String("FooBar").reverse(), "raBooF");
  EXPECT_EQ(String("").reverse(), "");
}

TEST(StringTest, contains) {
  EXPECT_EQ(String("Foo Bar Foo").contains("foo", String::CaseInsensitive), true);
  EXPECT_EQ(String("Foo Bar Foo").contains("bar foo", String::CaseInsensitive), true);
  EXPECT_EQ(String("Foo Bar Foo").contains("foo"), false);
  EXPECT_EQ(String("Foo Bar Foo").toLower(), String("foo bar foo"));
  EXPECT_EQ(String("Foo Bar Foo").toUpper(), String("FOO BAR FOO"));
}

TEST(StringTest, format) {
  EXPECT_EQ(strf("({}, {}, {})", 1, "foo", 3.2), "(1, foo, 3.2)");
  EXPECT_EQ(strf("{} ({}, {}, {})", String("asdf\0", 5), 1, "foo", 3.2), String("asdf\0 (1, foo, 3.2)", 19));
}

TEST(StringTest, append) {
  String s = "foo";
  s.append(String("bar"));
  EXPECT_EQ(s, "foobar");

  s = "foo";
  s.append("bar");
  EXPECT_EQ(s, "foobar");

  s = "foo";
  s.append('b');
  EXPECT_EQ(s, "foob");
}

TEST(StringTest, prepend) {
  String s = "foo";
  s.prepend(String("bar"));
  EXPECT_EQ(s, "barfoo");

  s = "foo";
  s.prepend("bar");
  EXPECT_EQ(s, "barfoo");

  s = "foo";
  s.prepend('b');
  EXPECT_EQ(s, "bfoo");
}

TEST(StringTest, utf8) {
  char const* utf8String = "This is a [日本語] Unicode String. (日本語)";
  EXPECT_EQ(utf8Length(utf8String, strlen(utf8String)), 37u);

  String s1 = utf8String;
  EXPECT_EQ(s1.utf8(), std::string(utf8String));
  EXPECT_EQ(s1, utf8String);
  EXPECT_EQ(s1, "This is a [日本語] Unicode String. (日本語)");
  EXPECT_EQ("This is a [日本語] Unicode String. (日本語)", s1);
  EXPECT_EQ(s1.size(), 37u);
  EXPECT_EQ(s1.utf8().size(), 49u);
  EXPECT_EQ(String(s1.utf8Ptr()), String(utf8String));

  EXPECT_EQ(String("abcdefghijkl").slice(1, 6, 2), String("bdf"));
  EXPECT_EQ(String("aa").compare("aaaa") < 0, true);
  EXPECT_EQ(String("bb").compare("aaaa") > 0, true);
  EXPECT_EQ(String("[日本語]").compare("[日本語]") == 0, true);
  EXPECT_EQ(String("Aa").compare("aAaA", String::CaseInsensitive) < 0, true);
  EXPECT_EQ(String("bB").compare("AaAa", String::CaseInsensitive) > 0, true);
  EXPECT_EQ(String("[日本語]").compare("[日本語]", String::CaseInsensitive) == 0, true);
  EXPECT_EQ(String("[日本語]").find(']', 1), 4u);

  EXPECT_EQ(String("日本語日本語日本語日本語").substr(6, 3), String("日本語"));
  String s2 = "日本語日本語日本語日本語";
  s2.erase(6, 3);
  EXPECT_EQ(s2, String("日本語日本語日本語"));
  EXPECT_EQ(String("日本語日本語日本語").reverse(), String("語本日語本日語本日"));

  EXPECT_EQ(String("foo_bar_baz_baf").regexMatch("foo.*baf"), true);
  EXPECT_EQ(String("日本語日本語日本語").regexMatch("日.*本語"), true);
  EXPECT_EQ(String("12345678").regexMatch("[[:digit:]]{1,8}"), true);
  EXPECT_EQ(String("81234567").regexMatch("[[:digit:]]{1,7}"), false);
  EXPECT_EQ(String("12345678").regexMatch("[[:digit:]]{1,7}"), false);
  EXPECT_EQ(String("12345678").regexMatch("[[:digit:]]{1,8}", false), true);
  EXPECT_EQ(String("81234567").regexMatch("[[:digit:]]{1,7}", false), true);
  EXPECT_EQ(String("12345678").regexMatch("[[:digit:]]{1,7}", false), true);

  EXPECT_EQ(String("𠜎𠜱𠝹𠱓𠱸𠲖𠳏𠳕𠴕𠵼𠵿𠸎𠸏𠹷𠺝𠺢𠻗𠻹𠻺𠼭𠼮𠽌𠾴𠾼𠿪𡁜𡁯𡁵𡁶𡁻𡃁𡃉𡇙𢃇𢞵𢫕𢭃𢯊𢱑𢱕𢳂𢴈𢵌𢵧𢺳𣲷𤓓𤶸𤷪"
                   "𥄫𦉘𦟌𦧲𦧺𧨾𨅝𨈇𨋢𨳊𨳍𨳒𩶘")
                .size(),
      62u);
}

TEST(StringTest, tags) {
  String testString = "<foo>:<bar>";
  StringMap<String> tags = {{"foo", "hello"}, {"bar", "there"}};

  EXPECT_EQ(testString.replaceTags(tags), "hello:there");
}

TEST(StringTest, CaseInsensitive) {
  EXPECT_TRUE(CaseInsensitiveStringCompare()("foo", "FOO"));
  EXPECT_FALSE(CaseInsensitiveStringCompare()("FOO", "foo "));
  EXPECT_FALSE(CaseInsensitiveStringCompare()("foo ", "FOO"));
  EXPECT_TRUE(CaseInsensitiveStringCompare()("FOO ", "foo "));

  EXPECT_EQ(CaseInsensitiveStringHash()("foo"), CaseInsensitiveStringHash()("FOO"));
  EXPECT_NE(CaseInsensitiveStringHash()("FOO"), CaseInsensitiveStringHash()("foo "));
  EXPECT_NE(CaseInsensitiveStringHash()("foo "), CaseInsensitiveStringHash()("FOO"));
  EXPECT_EQ(CaseInsensitiveStringHash()("FOO "), CaseInsensitiveStringHash()("foo "));

  // Should only be 2 string allocations, dunno how to test it
  StringMap<int, CaseInsensitiveStringHash, CaseInsensitiveStringCompare> map;
  map["One"] = 1;
  map["Three"] = 3;
  map["OnE"] = 2;

  EXPECT_TRUE(map.contains("one"));
  EXPECT_TRUE(map.contains("three"));
  EXPECT_FALSE(map.contains("two"));

  StringSet keys;
  for (auto const& p : map)
    keys.add(p.first);

  StringSet keyCmp = {"One", "Three"};
  EXPECT_EQ(keys, keyCmp);
}

TEST(StringTest, RegexSearch) {
  EXPECT_TRUE(String("foo").regexMatch("foo", true, true));
  EXPECT_FALSE(String("foo bar").regexMatch("foo", true, true));
  EXPECT_TRUE(String("foo bar").regexMatch("foo", false, true));
  EXPECT_TRUE(String("foo bar").regexMatch("FOO", false, false));
  EXPECT_FALSE(String("foo bar").regexMatch("FOO", false, true));
  EXPECT_TRUE(String("foo bar").regexMatch("^fo*", false, true));
  EXPECT_FALSE(String("foo bar").regexMatch("^fo*", true, true));
  EXPECT_TRUE(String("0123456").regexMatch("\\d{0,9}", true, true));
}

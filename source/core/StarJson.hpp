#pragma once

#include "StarDataStream.hpp"
#include "StarVariant.hpp"
#include "StarString.hpp"
#include "StarXXHash.hpp"

namespace Star {

STAR_EXCEPTION(JsonException, StarException);
STAR_EXCEPTION(JsonParsingException, StarException);

STAR_CLASS(Json);

typedef List<Json> JsonArray;
typedef shared_ptr<JsonArray const> JsonArrayConstPtr;

typedef StringMap<Json> JsonObject;
typedef shared_ptr<JsonObject const> JsonObjectConstPtr;

// Class for holding representation of JSON data.  Immutable and implicitly
// shared.
class Json {
public:
  template <typename Container>
  struct IteratorWrapper {
    typedef typename Container::const_iterator const_iterator;
    typedef const_iterator iterator;

    const_iterator begin() const;
    const_iterator end() const;

    shared_ptr<Container const> ptr;
  };

  enum class Type : uint8_t {
    Null = 0,
    Float = 1,
    Bool = 2,
    Int = 3,
    String = 4,
    Array = 5,
    Object = 6
  };

  static String typeName(Type t);
  static Type typeFromName(String const& t);

  static Json ofType(Type t);

  // Parses JSON or JSON sub-type
  static Json parse(String const& string);

  // Parses JSON object or array only (the only top level types allowed by
  // JSON)
  static Json parseJson(String const& json);

  // Constructs type Null
  Json();

  Json(double);
  Json(bool);
  Json(int);
  Json(long);
  Json(long long);
  Json(unsigned int);
  Json(unsigned long);
  Json(unsigned long long);
  Json(char const*);
  Json(String::Char const*);
  Json(String::Char const*, size_t);
  Json(String);
  Json(std::string);
  Json(JsonArray);
  Json(JsonObject);

  // Float and Int types are convertible between each other.  toDouble,
  // toFloat, toInt, toUInt may be called on either an Int or a Float.  For a
  // Float this is simply a C style cast from double, and for an Int it is
  // simply a C style cast from int64_t.
  //
  // Bools, Strings, Arrays, Objects, and Null are not automatically
  // convertible to any other type.

  double toDouble() const;
  float toFloat() const;
  bool toBool() const;
  int64_t toInt() const;
  uint64_t toUInt() const;
  String toString() const;
  JsonArray toArray() const;
  JsonObject toObject() const;

  // Internally, String, JsonArray, and JsonObject are shared via shared_ptr
  // since this class is immutable.  Use these methods to get at this pointer
  // without causing a copy.
  StringConstPtr stringPtr() const;
  JsonArrayConstPtr arrayPtr() const;
  JsonObjectConstPtr objectPtr() const;

  // As a convenience, make it easy to safely and quickly iterate over a
  // JsonArray or JsonObject contents by holding the container pointer.
  IteratorWrapper<JsonArray> iterateArray() const;
  IteratorWrapper<JsonObject> iterateObject() const;

  // opt* methods work like this, if the json is null, it returns none.  If the
  // json is convertible, it returns the converted type, otherwise an exception
  // occurrs.
  Maybe<Json> opt() const;
  Maybe<double> optDouble() const;
  Maybe<float> optFloat() const;
  Maybe<bool> optBool() const;
  Maybe<int64_t> optInt() const;
  Maybe<uint64_t> optUInt() const;
  Maybe<String> optString() const;
  Maybe<JsonArray> optArray() const;
  Maybe<JsonObject> optObject() const;

  // Size of array / object type json
  size_t size() const;

  // If this json is array type, get the value at the given index
  Json get(size_t index) const;
  double getDouble(size_t index) const;
  float getFloat(size_t index) const;
  bool getBool(size_t index) const;
  int64_t getInt(size_t index) const;
  uint64_t getUInt(size_t index) const;
  String getString(size_t index) const;
  JsonArray getArray(size_t index) const;
  JsonObject getObject(size_t index) const;

  // These versions of get* return default value if the index is out of range,
  // or if the value pointed to is null.
  Json get(size_t index, Json def) const;
  double getDouble(size_t index, double def) const;
  float getFloat(size_t index, float def) const;
  bool getBool(size_t index, bool def) const;
  int64_t getInt(size_t index, int64_t def) const;
  uint64_t getUInt(size_t index, int64_t def) const;
  String getString(size_t index, String def) const;
  JsonArray getArray(size_t index, JsonArray def) const;
  JsonObject getObject(size_t index, JsonObject def) const;

  // If object type, whether object contains key
  bool contains(String const& key) const;

  // If this json is object type, get the value for the given key
  Json get(String const& key) const;
  double getDouble(String const& key) const;
  float getFloat(String const& key) const;
  bool getBool(String const& key) const;
  int64_t getInt(String const& key) const;
  uint64_t getUInt(String const& key) const;
  String getString(String const& key) const;
  JsonArray getArray(String const& key) const;
  JsonObject getObject(String const& key) const;

  // These versions of get* return the default if the key is missing or the
  // value is null.
  Json get(String const& key, Json def) const;
  double getDouble(String const& key, double def) const;
  float getFloat(String const& key, float def) const;
  bool getBool(String const& key, bool def) const;
  int64_t getInt(String const& key, int64_t def) const;
  uint64_t getUInt(String const& key, int64_t def) const;
  String getString(String const& key, String def) const;
  JsonArray getArray(String const& key, JsonArray def) const;
  JsonObject getObject(String const& key, JsonObject def) const;

  // Works the same way as opt methods above.  Will never return a null value,
  // if there is a null entry it will just return an empty Maybe.
  Maybe<Json> opt(String const& key) const;
  Maybe<double> optDouble(String const& key) const;
  Maybe<float> optFloat(String const& key) const;
  Maybe<bool> optBool(String const& key) const;
  Maybe<int64_t> optInt(String const& key) const;
  Maybe<uint64_t> optUInt(String const& key) const;
  Maybe<String> optString(String const& key) const;
  Maybe<JsonArray> optArray(String const& key) const;
  Maybe<JsonObject> optObject(String const& key) const;

  // Combines gets recursively in friendly expressions.  For
  // example, call like this: json.query("path.to.array[3][4]")
  Json query(String const& path) const;
  double queryDouble(String const& path) const;
  float queryFloat(String const& path) const;
  bool queryBool(String const& path) const;
  int64_t queryInt(String const& path) const;
  uint64_t queryUInt(String const& path) const;
  String queryString(String const& path) const;
  JsonArray queryArray(String const& path) const;
  JsonObject queryObject(String const& path) const;

  // These versions of get* do not throw on missing / null keys anywhere in the
  // query path.
  Json query(String const& path, Json def) const;
  double queryDouble(String const& path, double def) const;
  float queryFloat(String const& path, float def) const;
  bool queryBool(String const& path, bool def) const;
  int64_t queryInt(String const& path, int64_t def) const;
  uint64_t queryUInt(String const& path, uint64_t def) const;
  String queryString(String const& path, String const& def) const;
  JsonArray queryArray(String const& path, JsonArray def) const;
  JsonObject queryObject(String const& path, JsonObject def) const;

  // Returns none on on missing / null keys anywhere in the query path.  Will
  // never return a null value, just an empty Maybe.
  Maybe<Json> optQuery(String const& path) const;
  Maybe<double> optQueryDouble(String const& path) const;
  Maybe<float> optQueryFloat(String const& path) const;
  Maybe<bool> optQueryBool(String const& path) const;
  Maybe<int64_t> optQueryInt(String const& path) const;
  Maybe<uint64_t> optQueryUInt(String const& path) const;
  Maybe<String> optQueryString(String const& path) const;
  Maybe<JsonArray> optQueryArray(String const& path) const;
  Maybe<JsonObject> optQueryObject(String const& path) const;

  // Returns a *new* object with the given values set/erased.  Throws if not an
  // object.
  Json set(String key, Json value) const;
  Json setPath(String path, Json value) const;
  Json setAll(JsonObject values) const;
  Json eraseKey(String key) const;
  Json erasePath(String path) const;

  // Returns a *new* array with the given values set/inserted/appended/erased.
  // Throws if not an array.
  Json set(size_t index, Json value) const;
  Json insert(size_t index, Json value) const;
  Json append(Json value) const;
  Json eraseIndex(size_t index) const;

  Type type() const;
  String typeName() const;
  Json convert(Type u) const;

  bool isType(Type type) const;
  bool canConvert(Type type) const;

  // isNull returns true when the type of the Json is null.  operator bool() is
  // the opposite of isNull().
  bool isNull() const;
  explicit operator bool() const;

  // Prints JSON or JSON sub-type.  If sort is true, then any object anywhere
  // inside this value will be sorted alphanumerically before being written,
  // resulting in a known *unique* textual representation of the Json that is
  // cross-platform.
  String repr(int pretty = 0, bool sort = false) const;
  // Prints JSON object or array only (only top level types allowed by JSON)
  String printJson(int pretty = 0, bool sort = false) const;

  // operator== and operator!= compare for exact equality with all types, and
  // additionally equality with numeric conversion with Int <-> Float
  bool operator==(Json const& v) const;
  bool operator!=(Json const& v) const;

  // Does this Json not share its storage with any other Json?
  bool unique() const;

  void getHash(XXHash3& hasher) const;

private:
  Json const* ptr(size_t index) const;
  Json const* ptr(String const& key) const;

  Variant<Empty, double, bool, int64_t, StringConstPtr, JsonArrayConstPtr, JsonObjectConstPtr> m_data;
};

std::ostream& operator<<(std::ostream& os, Json const& v);

// Fixes ambiguity with OrderedHashMap operator<<
std::ostream& operator<<(std::ostream& os, JsonObject const& v);

// Serialize json to DataStream.  Strings are stored as UTF-8, ints are stored
// as VLQ, doubles as 64 bit.
DataStream& operator<<(DataStream& ds, Json const& v);
DataStream& operator>>(DataStream& ds, Json& v);

// Convenience methods for Json containers
DataStream& operator<<(DataStream& ds, JsonArray const& l);
DataStream& operator>>(DataStream& ds, JsonArray& l);
DataStream& operator<<(DataStream& ds, JsonObject const& m);
DataStream& operator>>(DataStream& ds, JsonObject& m);

// Merges the two given json values and returns the result, by the following
// rules (applied in order):  If the base value is null, returns the merger.
// If the merger value is null, returns base.  For any two non-objects types,
// returns the merger.  If both values are objects, then the resulting object
// is the combination of both objects, but for each repeated key jsonMerge is
// called recursively on both values to determine the result.
Json jsonMerge(Json const& base, Json const& merger);

template <typename... T>
Json jsonMerge(Json const& base, Json const& merger, T const&... rest);

// Similar to jsonMerge, but query only for a single key.  Gets a value equal
// to jsonMerge(jsons...).query(key, Json()), but much faster than doing an
// entire merge operation.
template <typename... T>
Json jsonMergeQuery(String const& key, Json const& first, T const&... rest);

// jsonMergeQuery with a default.
template <typename... T>
Json jsonMergeQueryDef(String const& key, Json def, Json const& first, T const&... rest);

template <>
struct hash<Json> {
  size_t operator()(Json const& v) const;
};

template <typename Container>
auto Json::IteratorWrapper<Container>::begin() const -> const_iterator {
  return ptr->begin();
}

template <typename Container>
auto Json::IteratorWrapper<Container>::end() const -> const_iterator {
  return ptr->end();
}

template <typename... T>
Json jsonMerge(Json const& base, Json const& merger, T const&... rest) {
  return jsonMerge(jsonMerge(base, merger), rest...);
}

template <typename... T>
Json jsonMergeQuery(String const&, Json def) {
  return def;
}

template <typename... T>
Json jsonMergeQueryImpl(String const& key, Json const& json) {
  return json.query(key, {});
}

template <typename... T>
Json jsonMergeQueryImpl(String const& key, Json const& base, Json const& first, T const&... rest) {
  Json value = jsonMergeQueryImpl(key, first, rest...);
  if (value && !value.isType(Json::Type::Object))
    return value;
  return jsonMerge(base.query(key, {}), value);
}

template <typename... T>
Json jsonMergeQuery(String const& key, Json const& first, T const&... rest) {
  return jsonMergeQueryImpl(key, first, rest...);
}

template <typename... T>
Json jsonMergeQueryDef(String const& key, Json def, Json const& first, T const&... rest) {
  if (auto v = jsonMergeQueryImpl(key, first, rest...))
    return v;
  return def;
}

// Compares the two given json values and returns a boolean, by the following
// rules (applied in order): If both values are identical, return true. If both
// values are not equal, check if they are objects. If they are objects,
// iterate through every pair in the comparing object and check if the key is
// in the base object. If the key is in the base object, then jsonCompare is
// called recursively on both values. If the base object does not contain the
// key, or the recursion fails, return false. Otherwise, return true. If they
// are not objects, check if they are arrays. If they are arrays, iterate
// through every value in the comparing object and then recursively call
// jsonCompare on every value in the base object until a match is found. If a
// match is found, break and move on to the next value in the comparing array.
// If a match is found for every value in the comparing array, return true.
// Otherwise, return false. If both values are not identical, and are not
// objects or arrays, return false.
bool jsonCompare(Json const& base, Json const& compare);

}

template <> struct fmt::formatter<Star::Json> : ostream_formatter {};
template <> struct fmt::formatter<Star::JsonObject> : ostream_formatter {};

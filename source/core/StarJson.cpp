#include "StarJson.hpp"
#include "StarJsonBuilder.hpp"
#include "StarJsonPath.hpp"
#include "StarFormat.hpp"
#include "StarLexicalCast.hpp"
#include "StarIterator.hpp"
#include "StarFile.hpp"

namespace Star {

Json::Type Json::typeFromName(String const& t) {
  if (t == "float")
    return Type::Float;
  else if (t == "bool")
    return Type::Bool;
  else if (t == "int")
    return Type::Int;
  else if (t == "string")
    return Type::String;
  else if (t == "array")
    return Type::Array;
  else if (t == "object")
    return Type::Object;
  else if (t == "null")
    return Type::Null;
  else
    throw JsonException(strf("String '{}' is not a valid json type", t));
}

String Json::typeName(Type t) {
  switch (t) {
    case Type::Float:
      return "float";
    case Type::Bool:
      return "bool";
    case Type::Int:
      return "int";
    case Type::String:
      return "string";
    case Type::Array:
      return "array";
    case Type::Object:
      return "object";
    default:
      return "null";
  }
}

bool Json::operator==(const Json& v) const {
  if (type() == Type::Null && v.type() == Type::Null) {
    return true;
  } else if (type() != v.type()) {
    if ((type() == Type::Float || type() == Type::Int) && (v.type() == Type::Float || v.type() == Type::Int))
      return toDouble() == v.toDouble() && toInt() == v.toInt();
    return false;
  } else {
    if (type() == Type::Float)
      return m_data.get<double>() == v.m_data.get<double>();
    else if (type() == Type::Bool)
      return m_data.get<bool>() == v.m_data.get<bool>();
    else if (type() == Type::Int)
      return m_data.get<int64_t>() == v.m_data.get<int64_t>();
    else if (type() == Type::String)
      return *m_data.get<StringConstPtr>() == *v.m_data.get<StringConstPtr>();
    else if (type() == Type::Array)
      return *m_data.get<JsonArrayConstPtr>() == *v.m_data.get<JsonArrayConstPtr>();
    else if (type() == Type::Object)
      return *m_data.get<JsonObjectConstPtr>() == *v.m_data.get<JsonObjectConstPtr>();
  }
  return false;
}

bool Json::operator!=(const Json& v) const {
  return !(*this == v);
}

bool Json::unique() const {
  if (m_data.is<StringConstPtr>())
    return m_data.get<StringConstPtr>().unique();
  else if (m_data.is<JsonArrayConstPtr>())
    return m_data.get<JsonArrayConstPtr>().unique();
  else if (m_data.is<JsonObjectConstPtr>())
    return m_data.get<JsonObjectConstPtr>().unique();
  else
    return true;
}

Json Json::ofType(Type t) {
  switch (t) {
    case Type::Float:
      return Json(0.0);
    case Type::Bool:
      return Json(false);
    case Type::Int:
      return Json(0);
    case Type::String:
      return Json("");
    case Type::Array:
      return Json(JsonArray());
    case Type::Object:
      return Json(JsonObject());
    default:
      return Json();
  }
}

Json Json::parse(String const& string) {
  return inputUtf32Json<String::const_iterator>(string.begin(), string.end(), JsonParseType::Value);
}

Json Json::parseSequence(String const& sequence) {
  return inputUtf32Json<String::const_iterator>(sequence.begin(), sequence.end(), JsonParseType::Sequence);
}

Json Json::parseJson(String const& json) {
  return inputUtf32Json<String::const_iterator>(json.begin(), json.end(), JsonParseType::Top);
}

Json::Json() {}

Json::Json(double d) {
  m_data = d;
}

Json::Json(bool b) {
  m_data = b;
}

Json::Json(int i) {
  m_data = (int64_t)i;
}

Json::Json(long i) {
  m_data = (int64_t)i;
}

Json::Json(long long i) {
  m_data = (int64_t)i;
}

Json::Json(unsigned int i) {
  m_data = (int64_t)i;
}

Json::Json(unsigned long i) {
  m_data = (int64_t)i;
}

Json::Json(unsigned long long i) {
  m_data = (int64_t)i;
}

Json::Json(char const* s) {
  m_data = make_shared<String const>(s);
}

Json::Json(String::Char const* s) {
  m_data = make_shared<String const>(s);
}

Json::Json(String::Char const* s, size_t len) {
  m_data = make_shared<String const>(s, len);
}

Json::Json(String s) {
  m_data = make_shared<String const>(std::move(s));
}

Json::Json(std::string s) {
  m_data = make_shared<String const>((std::move(s)));
}

Json::Json(JsonArray l) {
  m_data = make_shared<JsonArray const>(std::move(l));
}

Json::Json(JsonObject m) {
  m_data = make_shared<JsonObject const>(std::move(m));
}

double Json::toDouble() const {
  if (type() == Type::Float)
    return m_data.get<double>();
  if (type() == Type::Int)
    return (double)m_data.get<int64_t>();

  throw JsonException::format("Improper conversion to double from {}", typeName());
}

float Json::toFloat() const {
  return (float)toDouble();
}

bool Json::toBool() const {
  if (type() != Type::Bool)
    throw JsonException::format("Improper conversion to bool from {}", typeName());
  return m_data.get<bool>();
}

int64_t Json::toInt() const {
  if (type() == Type::Float) {
    return (int64_t)m_data.get<double>();
  } else if (type() == Type::Int) {
    return m_data.get<int64_t>();
  } else {
    throw JsonException::format("Improper conversion to int from {}", typeName());
  }
}

uint64_t Json::toUInt() const {
  if (type() == Type::Float) {
    return (uint64_t)m_data.get<double>();
  } else if (type() == Type::Int) {
    return (uint64_t)m_data.get<int64_t>();
  } else {
    throw JsonException::format("Improper conversion to unsigned int from {}", typeName());
  }
}

String Json::toString() const {
  if (type() != Type::String)
    throw JsonException(strf("Cannot convert from {} to string", typeName()));
  return *m_data.get<StringConstPtr>();
}

JsonArray Json::toArray() const {
  if (type() != Type::Array)
    throw JsonException::format("Improper conversion to JsonArray from {}", typeName());
  return *m_data.get<JsonArrayConstPtr>();
}

JsonObject Json::toObject() const {
  if (type() != Type::Object)
    throw JsonException::format("Improper conversion to JsonObject from {}", typeName());
  return *m_data.get<JsonObjectConstPtr>();
}

StringConstPtr Json::stringPtr() const {
  if (type() != Type::String)
    throw JsonException(strf("Cannot convert from {} to string", typeName()));
  return m_data.get<StringConstPtr>();
}

JsonArrayConstPtr Json::arrayPtr() const {
  if (type() != Type::Array)
    throw JsonException::format("Improper conversion to JsonArray from {}", typeName());
  return m_data.get<JsonArrayConstPtr>();
}

JsonObjectConstPtr Json::objectPtr() const {
  if (type() != Type::Object)
    throw JsonException::format("Improper conversion to JsonObject from {}", typeName());
  return m_data.get<JsonObjectConstPtr>();
}

Json::IteratorWrapper<JsonArray> Json::iterateArray() const {
  return IteratorWrapper<JsonArray>{arrayPtr()};
}

Json::IteratorWrapper<JsonObject> Json::iterateObject() const {
  return IteratorWrapper<JsonObject>{objectPtr()};
}

Maybe<Json> Json::opt() const {
  if (isNull())
    return {};
  return *this;
}

Maybe<double> Json::optDouble() const {
  if (isNull())
    return {};
  return toDouble();
}

Maybe<float> Json::optFloat() const {
  if (isNull())
    return {};
  return toFloat();
}

Maybe<bool> Json::optBool() const {
  if (isNull())
    return {};
  return toBool();
}

Maybe<int64_t> Json::optInt() const {
  if (isNull())
    return {};
  return toInt();
}

Maybe<uint64_t> Json::optUInt() const {
  if (isNull())
    return {};
  return toUInt();
}

Maybe<String> Json::optString() const {
  if (isNull())
    return {};
  return toString();
}

Maybe<JsonArray> Json::optArray() const {
  if (isNull())
    return {};
  return toArray();
}

Maybe<JsonObject> Json::optObject() const {
  if (isNull())
    return {};
  return toObject();
}

size_t Json::size() const {
  if (type() == Type::Array)
    return m_data.get<JsonArrayConstPtr>()->size();
  else if (type() == Type::Object)
    return m_data.get<JsonObjectConstPtr>()->size();
  else
    throw JsonException("size() called on improper json type");
}

bool Json::contains(String const& key) const {
  if (type() == Type::Object)
    return m_data.get<JsonObjectConstPtr>()->contains(key);
  else
    throw JsonException("contains() called on improper json type");
}

Json Json::get(size_t index) const {
  if (auto p = ptr(index))
    return *p;
  throw JsonException(strf("Json::get({}) out of range", index));
}

double Json::getDouble(size_t index) const {
  return get(index).toDouble();
}

float Json::getFloat(size_t index) const {
  return get(index).toFloat();
}

bool Json::getBool(size_t index) const {
  return get(index).toBool();
}

int64_t Json::getInt(size_t index) const {
  return get(index).toInt();
}

uint64_t Json::getUInt(size_t index) const {
  return get(index).toUInt();
}

String Json::getString(size_t index) const {
  return get(index).toString();
}

JsonArray Json::getArray(size_t index) const {
  return get(index).toArray();
}

JsonObject Json::getObject(size_t index) const {
  return get(index).toObject();
}

Json Json::get(size_t index, Json def) const {
  if (auto p = ptr(index))
    return *p;
  return def;
}

double Json::getDouble(size_t index, double def) const {
  if (auto p = ptr(index))
    return p->toDouble();
  return def;
}

float Json::getFloat(size_t index, float def) const {
  if (auto p = ptr(index))
    return p->toFloat();
  return def;
}

bool Json::getBool(size_t index, bool def) const {
  if (auto p = ptr(index))
    return p->toBool();
  return def;
}

int64_t Json::getInt(size_t index, int64_t def) const {
  if (auto p = ptr(index))
    return p->toInt();
  return def;
}

uint64_t Json::getUInt(size_t index, int64_t def) const {
  if (auto p = ptr(index))
    return p->toUInt();
  return def;
}

String Json::getString(size_t index, String def) const {
  if (auto p = ptr(index))
    return p->toString();
  return def;
}

JsonArray Json::getArray(size_t index, JsonArray def) const {
  if (auto p = ptr(index))
    return p->toArray();
  return def;
}

JsonObject Json::getObject(size_t index, JsonObject def) const {
  if (auto p = ptr(index))
    return p->toObject();
  return def;
}

Json Json::get(String const& key) const {
  if (auto p = ptr(key))
    return *p;
  throw JsonException(strf("No such key in Json::get(\"{}\")", key));
}

double Json::getDouble(String const& key) const {
  return get(key).toDouble();
}

float Json::getFloat(String const& key) const {
  return get(key).toFloat();
}

bool Json::getBool(String const& key) const {
  return get(key).toBool();
}

int64_t Json::getInt(String const& key) const {
  return get(key).toInt();
}

uint64_t Json::getUInt(String const& key) const {
  return get(key).toUInt();
}

String Json::getString(String const& key) const {
  return get(key).toString();
}

JsonArray Json::getArray(String const& key) const {
  return get(key).toArray();
}

JsonObject Json::getObject(String const& key) const {
  return get(key).toObject();
}

Json Json::get(String const& key, Json def) const {
  if (auto p = ptr(key))
    return *p;
  return def;
}

double Json::getDouble(String const& key, double def) const {
  auto p = ptr(key);
  if (p && *p)
    return p->toDouble();
  return def;
}

float Json::getFloat(String const& key, float def) const {
  auto p = ptr(key);
  if (p && *p)
    return p->toFloat();
  return def;
}

bool Json::getBool(String const& key, bool def) const {
  auto p = ptr(key);
  if (p && *p)
    return p->toBool();
  return def;
}

int64_t Json::getInt(String const& key, int64_t def) const {
  auto p = ptr(key);
  if (p && *p)
    return p->toInt();
  return def;
}

uint64_t Json::getUInt(String const& key, int64_t def) const {
  auto p = ptr(key);
  if (p && *p)
    return p->toUInt();
  return def;
}

String Json::getString(String const& key, String def) const {
  auto p = ptr(key);
  if (p && *p)
    return p->toString();
  return def;
}

JsonArray Json::getArray(String const& key, JsonArray def) const {
  auto p = ptr(key);
  if (p && *p)
    return p->toArray();
  return def;
}

JsonObject Json::getObject(String const& key, JsonObject def) const {
  auto p = ptr(key);
  if (p && *p)
    return p->toObject();
  return def;
}

Maybe<Json> Json::opt(String const& key) const {
  auto p = ptr(key);
  if (p && *p)
    return *p;
  return {};
}

Maybe<double> Json::optDouble(String const& key) const {
  auto p = ptr(key);
  if (p && *p)
    return p->toDouble();
  return {};
}

Maybe<float> Json::optFloat(String const& key) const {
  auto p = ptr(key);
  if (p && *p)
    return p->toFloat();
  return {};
}

Maybe<bool> Json::optBool(String const& key) const {
  auto p = ptr(key);
  if (p && *p)
    return p->toBool();
  return {};
}

Maybe<int64_t> Json::optInt(String const& key) const {
  auto p = ptr(key);
  if (p && *p)
    return p->toInt();
  return {};
}

Maybe<uint64_t> Json::optUInt(String const& key) const {
  auto p = ptr(key);
  if (p && *p)
    return p->toUInt();
  return {};
}

Maybe<String> Json::optString(String const& key) const {
  auto p = ptr(key);
  if (p && *p)
    return p->toString();
  return {};
}

Maybe<JsonArray> Json::optArray(String const& key) const {
  auto p = ptr(key);
  if (p && *p)
    return p->toArray();
  return {};
}

Maybe<JsonObject> Json::optObject(String const& key) const {
  auto p = ptr(key);
  if (p && *p)
    return p->toObject();
  return {};
}

Json Json::query(String const& q) const {
  return JsonPath::pathGet(*this, JsonPath::parseQueryPath, q);
}

double Json::queryDouble(String const& q) const {
  return JsonPath::pathGet(*this, JsonPath::parseQueryPath, q).toDouble();
}

float Json::queryFloat(String const& q) const {
  return JsonPath::pathGet(*this, JsonPath::parseQueryPath, q).toFloat();
}

bool Json::queryBool(String const& q) const {
  return JsonPath::pathGet(*this, JsonPath::parseQueryPath, q).toBool();
}

int64_t Json::queryInt(String const& q) const {
  return JsonPath::pathGet(*this, JsonPath::parseQueryPath, q).toInt();
}

uint64_t Json::queryUInt(String const& q) const {
  return JsonPath::pathGet(*this, JsonPath::parseQueryPath, q).toUInt();
}

String Json::queryString(String const& q) const {
  return JsonPath::pathGet(*this, JsonPath::parseQueryPath, q).toString();
}

JsonArray Json::queryArray(String const& q) const {
  return JsonPath::pathGet(*this, JsonPath::parseQueryPath, q).toArray();
}

JsonObject Json::queryObject(String const& q) const {
  return JsonPath::pathGet(*this, JsonPath::parseQueryPath, q).toObject();
}

Json Json::query(String const& query, Json def) const {
  if (auto json = JsonPath::pathFind(*this, JsonPath::parseQueryPath, query))
    return *json;
  return def;
}

double Json::queryDouble(String const& query, double def) const {
  auto json = JsonPath::pathFind(*this, JsonPath::parseQueryPath, query);
  if (json && *json)
    return json->toDouble();
  return def;
}

float Json::queryFloat(String const& query, float def) const {
  auto json = JsonPath::pathFind(*this, JsonPath::parseQueryPath, query);
  if (json && *json)
    return json->toFloat();
  return def;
}

bool Json::queryBool(String const& query, bool def) const {
  auto json = JsonPath::pathFind(*this, JsonPath::parseQueryPath, query);
  if (json && *json)
    return json->toBool();
  return def;
}

int64_t Json::queryInt(String const& query, int64_t def) const {
  auto json = JsonPath::pathFind(*this, JsonPath::parseQueryPath, query);
  if (json && *json)
    return json->toInt();
  return def;
}

uint64_t Json::queryUInt(String const& query, uint64_t def) const {
  auto json = JsonPath::pathFind(*this, JsonPath::parseQueryPath, query);
  if (json && *json)
    return json->toUInt();
  return def;
}

String Json::queryString(String const& query, String const& def) const {
  auto json = JsonPath::pathFind(*this, JsonPath::parseQueryPath, query);
  if (json && *json)
    return json->toString();
  return def;
}

JsonArray Json::queryArray(String const& query, JsonArray def) const {
  auto json = JsonPath::pathFind(*this, JsonPath::parseQueryPath, query);
  if (json && *json)
    return json->toArray();
  return def;
}

JsonObject Json::queryObject(String const& query, JsonObject def) const {
  auto json = JsonPath::pathFind(*this, JsonPath::parseQueryPath, query);
  if (json && *json)
    return json->toObject();
  return def;
}

Maybe<Json> Json::optQuery(String const& path) const {
  auto json = JsonPath::pathFind(*this, JsonPath::parseQueryPath, path);
  if (json && *json)
    return *json;
  return {};
}

Maybe<double> Json::optQueryDouble(String const& path) const {
  auto json = JsonPath::pathFind(*this, JsonPath::parseQueryPath, path);
  if (json && *json)
    return json->toDouble();
  return {};
}

Maybe<float> Json::optQueryFloat(String const& path) const {
  auto json = JsonPath::pathFind(*this, JsonPath::parseQueryPath, path);
  if (json && *json)
    return json->toFloat();
  return {};
}

Maybe<bool> Json::optQueryBool(String const& path) const {
  auto json = JsonPath::pathFind(*this, JsonPath::parseQueryPath, path);
  if (json && *json)
    return json->toBool();
  return {};
}

Maybe<int64_t> Json::optQueryInt(String const& path) const {
  auto json = JsonPath::pathFind(*this, JsonPath::parseQueryPath, path);
  if (json && *json)
    return json->toInt();
  return {};
}

Maybe<uint64_t> Json::optQueryUInt(String const& path) const {
  auto json = JsonPath::pathFind(*this, JsonPath::parseQueryPath, path);
  if (json && *json)
    return json->toUInt();
  return {};
}

Maybe<String> Json::optQueryString(String const& path) const {
  auto json = JsonPath::pathFind(*this, JsonPath::parseQueryPath, path);
  if (json && *json)
    return json->toString();
  return {};
}

Maybe<JsonArray> Json::optQueryArray(String const& path) const {
  auto json = JsonPath::pathFind(*this, JsonPath::parseQueryPath, path);
  if (json && *json)
    return json->toArray();
  return {};
}

Maybe<JsonObject> Json::optQueryObject(String const& path) const {
  auto json = JsonPath::pathFind(*this, JsonPath::parseQueryPath, path);
  if (json && *json)
    return json->toObject();
  return {};
}

Json Json::set(String key, Json value) const {
  auto map = toObject();
  map[std::move(key)] = std::move(value);
  return map;
}

Json Json::setPath(String path, Json value) const {
  return JsonPath::pathSet(*this, JsonPath::parseQueryPath, path, value);
}

Json Json::erasePath(String path) const {
  return JsonPath::pathRemove(*this, JsonPath::parseQueryPath, path);
}

Json Json::setAll(JsonObject values) const {
  auto map = toObject();
  for (auto& p : values)
    map[std::move(p.first)] = std::move(p.second);
  return map;
}

Json Json::eraseKey(String key) const {
  auto map = toObject();
  map.erase(std::move(key));
  return map;
}

Json Json::set(size_t index, Json value) const {
  auto array = toArray();
  array[index] = std::move(value);
  return array;
}

Json Json::insert(size_t index, Json value) const {
  auto array = toArray();
  array.insertAt(index, std::move(value));
  return array;
}

Json Json::append(Json value) const {
  auto array = toArray();
  array.append(std::move(value));
  return array;
}

Json Json::eraseIndex(size_t index) const {
  auto array = toArray();
  array.eraseAt(index);
  return array;
}

Json::Type Json::type() const {
  return (Type)m_data.typeIndex();
}

String Json::typeName() const {
  return typeName(type());
}

Json Json::convert(Type u) const {
  if (type() == u)
    return *this;

  switch (u) {
    case Type::Null:
      return Json();
    case Type::Float:
      return toDouble();
    case Type::Bool:
      return toBool();
    case Type::Int:
      return toInt();
    case Type::String:
      return toString();
    case Type::Array:
      return toArray();
    case Type::Object:
      return toObject();
    default:
      throw JsonException::format("Improper conversion to type {}", typeName(u));
  }
}

bool Json::isType(Type t) const {
  return type() == t;
}

bool Json::canConvert(Type t) const {
  if (type() == t)
    return true;

  if (t == Type::Null)
    return true;

  if ((type() == Type::Float || type() == Type::Int) && (t == Type::Float || t == Type::Int))
    return true;

  return false;
}

bool Json::isNull() const {
  return type() == Type::Null;
}

Json::operator bool() const {
  return !isNull();
}

String Json::repr(int pretty, bool sort) const {
  String result;
  outputUtf32Json(*this, std::back_inserter(result), pretty, sort);
  return result;
}

String Json::printJson(int pretty, bool sort) const {
  if (type() != Type::Object && type() != Type::Array)
    throw JsonException("printJson called on non-top-level JSON type");

  return repr(pretty, sort);
}

std::ostream& operator<<(std::ostream& os, Json const& v) {
  outputUtf8Json(v, std::ostream_iterator<char>(os), 0, false);
  return os;
}

std::ostream& operator<<(std::ostream& os, JsonObject const& v) {
  // Blargh copy
  os << Json(v);
  return os;
}

DataStream& operator<<(DataStream& os, const Json& v) {
  // Compatibility with old serialization, 0 was INVALID but INVALID is no
  // longer used.
  os.write<uint8_t>((uint8_t)v.type() + 1);

  if (v.type() == Json::Type::Float) {
    os.write<double>(v.toDouble());
  } else if (v.type() == Json::Type::Bool) {
    os.write<bool>(v.toBool());
  } else if (v.type() == Json::Type::Int) {
    os.writeVlqI(v.toInt());
  } else if (v.type() == Json::Type::String) {
    os.write<String>(v.toString());
  } else if (v.type() == Json::Type::Array) {
    auto const& l = v.toArray();
    os.writeVlqU(l.size());
    for (auto const& v : l)
      os.write<Json>(v);
  } else if (v.type() == Json::Type::Object) {
    auto const& m = v.toObject();
    os.writeVlqU(m.size());
    for (auto const& v : m) {
      os.write<String>(v.first);
      os.write<Json>(v.second);
    }
  }
  return os;
}

DataStream& operator>>(DataStream& os, Json& v) {
  // Compatibility with old serialization, 0 was INVALID but INVALID is no
  // longer used.
  uint8_t typeIndex = os.read<uint8_t>();
  if (typeIndex > 0)
    typeIndex -= 1;

  Json::Type type = (Json::Type)typeIndex;

  if (type == Json::Type::Float) {
    v = Json(os.read<double>());
  } else if (type == Json::Type::Bool) {
    v = Json(os.read<bool>());
  } else if (type == Json::Type::Int) {
    v = Json(os.readVlqI());
  } else if (type == Json::Type::String) {
    v = Json(os.read<String>());
  } else if (type == Json::Type::Array) {
    JsonArray l;
    size_t s = os.readVlqU();
    for (size_t i = 0; i < s; ++i)
      l.append(os.read<Json>());

    v = std::move(l);
  } else if (type == Json::Type::Object) {
    JsonObject m;
    size_t s = os.readVlqU();
    for (size_t i = 0; i < s; ++i) {
      String k = os.read<String>();
      m[k] = os.read<Json>();
    }

    v = std::move(m);
  }

  return os;
}

DataStream& operator<<(DataStream& ds, JsonArray const& l) {
  ds.writeContainer(l);
  return ds;
}

DataStream& operator>>(DataStream& ds, JsonArray& l) {
  ds.readContainer(l);
  return ds;
}

DataStream& operator<<(DataStream& ds, JsonObject const& m) {
  ds.writeMapContainer(m);
  return ds;
}

DataStream& operator>>(DataStream& ds, JsonObject& m) {
  ds.readMapContainer(m);
  return ds;
}

void Json::getHash(XXHash3& hasher) const {
  Json::Type type = (Json::Type)m_data.typeIndex();
  if (type == Json::Type::Bool)
    hasher.push(m_data.get<bool>() ? "\2\1" : "\2\0", 2);
  else {
    hasher.push((const char*)&type, sizeof(type));
    if (type == Json::Type::Float)
      hasher.push((const char*)m_data.ptr<double>(), sizeof(double));
    else if (type == Json::Type::Int)
      hasher.push((const char*)m_data.ptr<int64_t>(), sizeof(int64_t));
    else if (type == Json::Type::String) {
      const String& str = *m_data.get<StringConstPtr>();
      hasher.push(str.utf8Ptr(), str.utf8Size());
    }
    else if (type == Json::Type::Array) {
      for (Json const& json : *m_data.get<JsonArrayConstPtr>())
        json.getHash(hasher);
    }
    else if (type == Json::Type::Object) {
      auto& object = *m_data.get<JsonObjectConstPtr>();
      List<JsonObject::const_iterator> iterators;
      iterators.reserve(object.size());
      for (auto i = object.begin(); i != object.end(); ++i)
        iterators.append(i);
      iterators.sort([](JsonObject::const_iterator a, JsonObject::const_iterator b) {
        return a->first < b->first;
      });
      for (auto& iter : iterators) {
        hasher.push(iter->first.utf8Ptr(), iter->first.utf8Size());
        iter->second.getHash(hasher);
      }
    }
  }
}

size_t hash<Json>::operator()(Json const& v) const {
  XXHash3 hasher;
  v.getHash(hasher);
  return hasher.digest();
}

Json const* Json::ptr(size_t index) const {
  if (type() != Type::Array)
    throw JsonException::format("Cannot call get with index on Json type {}, must be Array type", typeName());

  auto const& list = *m_data.get<JsonArrayConstPtr>();
  if (index >= list.size())
    return nullptr;
  return &list[index];
}

Json const* Json::ptr(String const& key) const {
  if (type() != Type::Object)
    throw JsonException::format("Cannot call get with key on Json type {}, must be Object type", typeName());
  auto const& map = m_data.get<JsonObjectConstPtr>();

  auto i = map->find(key);
  if (i == map->end())
    return nullptr;

  return &i->second;
}

Json jsonMerge(Json const& base, Json const& merger) {
  if (base.type() == Json::Type::Object && merger.type() == Json::Type::Object) {
    JsonObject merged = base.toObject();
    for (auto const& p : merger.toObject()) {
      auto res = merged.insert(p);
      if (!res.second)
        res.first->second = jsonMerge(res.first->second, p.second);
    }
    return merged;
  }
  return merger.type() == Json::Type::Null ? base : merger;
}

Json jsonMergeNulling(Json const& base, Json const& merger) {
  if (base.type() == Json::Type::Object && merger.type() == Json::Type::Object) {
    JsonObject merged = base.toObject();
    for (auto const& p : merger.toObject()) {
      if (p.second.isNull())
        merged.erase(p.first);
      else {
        auto res = merged.insert(p);
        if (!res.second)
          res.first->second = jsonMergeNulling(res.first->second, p.second);
      }
    }
    return merged;
  }
  return merger;
}

bool jsonPartialMatch(Json const& base, Json const& compare) {
  if (base == compare) {
    return true;
  } else {
    if (base.type() == Json::Type::Object && compare.type() == Json::Type::Object) {
      for (auto const& c : compare.toObject()) {
        if (!base.contains(c.first) || !jsonPartialMatch(base.get(c.first), c.second))
          return false;
      }
      return true;
    }
    if (base.type() == Json::Type::Array && compare.type() == Json::Type::Array) {
      for (auto const& c : compare.toArray()) {
        bool similar = false;
        for (auto const& b : base.toArray()) {
          if (jsonPartialMatch(c, b)) {
            similar = true;
            break;
          }
        }
        if (!similar)
          return false;
      }
      return true;
    }

    return false;
  }
}

}
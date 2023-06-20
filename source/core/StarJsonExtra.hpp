#ifndef STAR_JSON_EXTRA_HPP
#define STAR_JSON_EXTRA_HPP

#include "StarJson.hpp"
#include "StarPoly.hpp"
#include "StarColor.hpp"
#include "StarSet.hpp"
#include "StarWeightedPool.hpp"

namespace Star {

// Extra methods to parse a variety of types out of pure JSON.  Throws
// JsonException if json is not of correct type or size.

size_t jsonToSize(Json const& v);
Json jsonFromSize(size_t s);

// Must be array of appropriate size.

Vec2D jsonToVec2D(Json const& v);
Vec2F jsonToVec2F(Json const& v);
Json jsonFromVec2F(Vec2F const& v);
Vec2I jsonToVec2I(Json const& v);
Json jsonFromVec2I(Vec2I const& v);
Vec2U jsonToVec2U(Json const& v);
Json jsonFromVec2U(Vec2U const& v);
Vec2B jsonToVec2B(Json const& v);
Json jsonFromVec2B(Vec2B const& v);

Vec3D jsonToVec3D(Json const& v);
Vec3F jsonToVec3F(Json const& v);
Json jsonFromVec3F(Vec3F const& v);
Vec3I jsonToVec3I(Json const& v);
Json jsonFromVec3I(Vec3I const& v);
Vec3B jsonToVec3B(Json const& v);

Vec4B jsonToVec4B(Json const& v);
Vec4I jsonToVec4I(Json const& v);
Vec4F jsonToVec4F(Json const& v);

// Must be array of size 4 or 2 arrays of size 2 in an array.
RectD jsonToRectD(Json const& v);
Json jsonFromRectD(RectD const& rect);
RectF jsonToRectF(Json const& v);
Json jsonFromRectF(RectF const& rect);
RectI jsonToRectI(Json const& v);
Json jsonFromRectI(RectI const& rect);
RectU jsonToRectU(Json const& v);
Json jsonFromRectU(RectU const& rect);

// Can be a string, array of size 3 or 4 of doubles or ints.  If double, range
// is 0.0 to 1.0, if int range is 0-255
Color jsonToColor(Json const& v);
Json jsonFromColor(Color const& color);

// HACK: Fix clockwise specified polygons in coming from JSON
template <typename Float>
Polygon<Float> fixInsideOutPoly(Polygon<Float> p);

// Array of size 2 arrays
PolyD jsonToPolyD(Json const& v);
PolyF jsonToPolyF(Json const& v);
PolyI jsonToPolyI(Json const& v);
Json jsonFromPolyF(PolyF const& poly);

// Expects a size 2 array of size 2 arrays
Line2F jsonToLine2F(Json const& v);
Json jsonFromLine2F(Line2F const& line);

Mat3F jsonToMat3F(Json const& v);
Json jsonFromMat3F(Mat3F const& v);

StringList jsonToStringList(Json const& v);
Json jsonFromStringList(List<String> const& v);
StringSet jsonToStringSet(Json const& v);
Json jsonFromStringSet(StringSet const& v);
List<float> jsonToFloatList(Json const& v);
List<int> jsonToIntList(Json const& v);
List<Vec2I> jsonToVec2IList(Json const& v);
List<Vec2U> jsonToVec2UList(Json const& v);
List<Vec2F> jsonToVec2FList(Json const& v);
List<Vec4B> jsonToVec4BList(Json const& v);
List<Color> jsonToColorList(Json const& v);

Json weightedChoiceFromJson(Json const& source, Json const& default_);

// Assumes that the bins parameter is an array of pairs (arrays), where the
// first element is a minimum value and the second element is the actual
// important value.  Finds the pair with the highest value that is less than or
// equal to the given target, and returns the second element.
Json binnedChoiceFromJson(Json const& bins, float target, Json const& def = Json());

template <typename T>
WeightedPool<T> jsonToWeightedPool(Json const& source);
template <typename T, typename Converter>
WeightedPool<T> jsonToWeightedPool(Json const& source, Converter&& converter);

template <typename T>
Json jsonFromWeightedPool(WeightedPool<T> const& pool);
template <typename T, typename Converter>
Json jsonFromWeightedPool(WeightedPool<T> const& pool, Converter&& converter);

template <size_t Size>
Array<unsigned, Size> jsonToArrayU(Json const& v) {
  if (v.size() != Size)
    throw JsonException(strf("Json array not of size %d in jsonToArrayU", Size).c_str());

  Array<unsigned, Size> res;
  for (size_t i = 0; i < Size; i++) {
    res[i] = v.getUInt(i);
  }

  return res;
}

template <size_t Size>
Array<size_t, Size> jsonToArrayS(Json const& v) {
  if (v.size() != Size)
    throw JsonException(strf("Json array not of size %d in jsonToArrayS", Size).c_str());

  Array<size_t, Size> res;
  for (size_t i = 0; i < Size; i++) {
    res[i] = v.getUInt(i);
  }

  return res;
}

template <size_t Size>
Array<int, Size> jsonToArrayI(Json const& v) {
  if (v.size() != Size)
    throw JsonException(strf("Json array not of size %d in jsonToArrayI", Size).c_str());

  Array<int, Size> res;
  for (size_t i = 0; i < Size; i++) {
    res[i] = v.getInt(i);
  }

  return res;
}

template <size_t Size>
Array<float, Size> jsonToArrayF(Json const& v) {
  if (v.size() != Size)
    throw JsonException(strf("Json array not of size %d in jsonToArrayF", Size).c_str());

  Array<float, Size> res;
  for (size_t i = 0; i < Size; i++) {
    res[i] = v.getFloat(i);
  }

  return res;
}

template <size_t Size>
Array<double, Size> jsonToArrayD(Json const& v) {
  if (v.size() != Size)
    throw JsonException(strf("Json array not of size %d in jsonToArrayD", Size).c_str());

  Array<double, Size> res;
  for (size_t i = 0; i < Size; i++) {
    res[i] = v.getDouble(i);
  }

  return res;
}

template <size_t Size>
Array<String, Size> jsonToStringArray(Json const& v) {
  if (v.size() != Size)
    throw JsonException(strf("Json array not of size %d in jsonToStringArray", Size).c_str());

  Array<String, Size> res;
  for (size_t i = 0; i < Size; i++) {
    res[i] = v.getString(i);
  }

  return res;
}

template <typename Value>
List<Value> jsonToList(Json const& v) {
  return jsonToList<Value>(v, construct<Value>());
}

template <typename Value, typename Converter>
List<Value> jsonToList(Json const& v, Converter&& valueConvert) {
  if (v.type() != Json::Type::Array)
    throw JsonException("Json type is not a array in jsonToList");

  List<Value> res;
  for (auto const& entry : v.iterateArray())
    res.push_back(valueConvert(entry));

  return res;
}

template <typename Value>
Json jsonFromList(List<Value> const& list) {
  return jsonFromList<Value>(list, construct<Json>());
}

template <typename Value, typename Converter>
Json jsonFromList(List<Value> const& list, Converter&& valueConvert) {
  JsonArray res;
  for (auto const& entry : list)
    res.push_back(valueConvert(entry));

  return res;
}

template <typename Value>
Set<Value> jsonToSet(Json const& v) {
  return jsonToSet<Value>(v, construct<Value>());
}

template <typename Value, typename Converter>
Set<Value> jsonToSet(Json const& v, Converter&& valueConvert) {
  if (v.type() != Json::Type::Array)
    throw JsonException("Json type is not an array in jsonToSet");

  Set<Value> res;
  for (auto const& entry : v.iterateArray())
    res.add(valueConvert(entry));

  return res;
}

template <typename Value>
Json jsonFromSet(Set<Value> const& Set) {
  return jsonFromSet<Value>(Set, construct<Json>());
}

template <typename Value, typename Converter>
Json jsonFromSet(Set<Value> const& Set, Converter&& valueConvert) {
  JsonArray res;
  for (auto entry : Set)
    res.push_back(valueConvert(entry));

  return res;
}

template <typename MapType, typename KeyConverter, typename ValueConverter>
MapType jsonToMapKV(Json const& v, KeyConverter&& keyConvert, ValueConverter&& valueConvert) {
  if (v.type() != Json::Type::Object)
    throw JsonException("Json type is not an object in jsonToMap");

  MapType res;
  for (auto const& pair : v.iterateObject())
    res.add(keyConvert(pair.first), valueConvert(pair.second));

  return res;
}

template <typename MapType, typename KeyConverter>
MapType jsonToMapK(Json const& v, KeyConverter&& keyConvert) {
  return jsonToMapKV<MapType>(v, forward<KeyConverter>(keyConvert), construct<typename MapType::mapped_type>());
}

template <typename MapType, typename ValueConverter>
MapType jsonToMapV(Json const& v, ValueConverter&& valueConvert) {
  return jsonToMapKV<MapType>(v, construct<typename MapType::key_type>(), forward<ValueConverter>(valueConvert));
}

template <typename MapType>
MapType jsonToMap(Json const& v) {
  return jsonToMapKV<MapType>(v, construct<typename MapType::key_type>(), construct<typename MapType::mapped_type>());
}

template <typename MapType, typename KeyConverter, typename ValueConverter>
Json jsonFromMapKV(MapType const& map, KeyConverter&& keyConvert, ValueConverter&& valueConvert) {
  JsonObject res;
  for (auto pair : map)
    res[keyConvert(pair.first)] = valueConvert(pair.second);

  return res;
}

template <typename MapType, typename KeyConverter>
Json jsonFromMapK(MapType const& map, KeyConverter&& keyConvert) {
  return jsonFromMapKV<MapType>(map, forward<KeyConverter>(keyConvert), construct<Json>());
}

template <typename MapType, typename ValueConverter>
Json jsonFromMapV(MapType const& map, ValueConverter&& valueConvert) {
  return jsonFromMapKV<MapType>(map, construct<String>(), forward<ValueConverter>(valueConvert));
}

template <typename MapType>
Json jsonFromMap(MapType const& map) {
  return jsonFromMapKV<MapType>(map, construct<String>(), construct<Json>());
}

template <typename T, typename Converter>
Json jsonFromMaybe(Maybe<T> const& m, Converter&& converter) {
  return m.apply(converter).value();
}

template <typename T>
Json jsonFromMaybe(Maybe<T> const& m) {
  return jsonFromMaybe(m, construct<Json>());
}

template <typename T, typename Converter>
Maybe<T> jsonToMaybe(Json v, Converter&& converter) {
  if (v.isNull())
    return {};
  return converter(v);
}

template <typename T>
Maybe<T> jsonToMaybe(Json const& v) {
  return jsonToMaybe<T>(v, construct<T>());
}

template <typename T>
WeightedPool<T> jsonToWeightedPool(Json const& source) {
  return jsonToWeightedPool<T>(source, construct<T>());
}

template <typename T, typename Converter>
WeightedPool<T> jsonToWeightedPool(Json const& source, Converter&& converter) {
  WeightedPool<T> res;
  if (source.isNull())
    return res;
  for (auto entry : source.iterateArray()) {
    if (entry.isType(Json::Type::Array))
      res.add(entry.get(0).toDouble(), converter(entry.get(1)));
    else
      res.add(entry.getDouble("weight"), converter(entry.get("item")));
  }

  return res;
}

template <typename T>
Json jsonFromWeightedPool(WeightedPool<T> const& pool) {
  return jsonFromWeightedPool<T>(pool, construct<Json>());
}

template <typename T, typename Converter>
Json jsonFromWeightedPool(WeightedPool<T> const& pool, Converter&& converter) {
  JsonArray res;
  for (auto const& pair : pool.items()) {
    res.append(JsonObject{
        {"weight", pair.first}, {"item", converter(pair.second)},
    });
  }
  return res;
}

template <>
WeightedPool<int> jsonToWeightedPool(Json const& source);

template <>
WeightedPool<unsigned> jsonToWeightedPool(Json const& source);

template <>
WeightedPool<float> jsonToWeightedPool(Json const& source);

template <>
WeightedPool<double> jsonToWeightedPool(Json const& source);

template <>
WeightedPool<String> jsonToWeightedPool(Json const& source);

template <>
WeightedPool<JsonArray> jsonToWeightedPool(Json const& source);

template <>
WeightedPool<JsonObject> jsonToWeightedPool(Json const& source);

template <typename Float>
Polygon<Float> fixInsideOutPoly(Polygon<Float> p) {
  if (p.sides() > 2) {
    if ((p.side(1).diff() ^ p.side(0).diff()) > 0)
      reverse(p.vertexes());
  }
  return p;
}

}

#endif

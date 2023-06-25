#include "StarJsonExtra.hpp"
#include "StarLogging.hpp"
#include "StarRandom.hpp"

namespace Star {

size_t jsonToSize(Json const& v) {
  if (v.isNull())
    return NPos;

  if (!v.canConvert(Json::Type::Int))
    throw JsonException("Json not an int in jsonToSize");

  return v.toUInt();
}

Json jsonFromSize(size_t s) {
  if (s == NPos)
    return Json();
  return Json(s);
}

Vec2D jsonToVec2D(Json const& v) {
  if (v.type() != Json::Type::Array || v.size() != 2)
    throw JsonException("Json not an array of size 2 in jsonToVec2D");

  return Vec2D(v.getDouble(0), v.getDouble(1));
}

Vec2F jsonToVec2F(Json const& v) {
  if (v.type() != Json::Type::Array || v.size() != 2)
    throw JsonException("Json not an array of size 2 in jsonToVec2F");

  return Vec2F(v.getFloat(0), v.getFloat(1));
}

Json jsonFromVec2F(Vec2F const& v) {
  return JsonArray{v[0], v[1]};
}

Vec2I jsonToVec2I(Json const& v) {
  if (v.type() != Json::Type::Array || v.size() != 2)
    throw JsonException("Json not an array of size 2 in jsonToVec2I");

  return Vec2I(v.getInt(0), v.getInt(1));
}

Json jsonFromVec2I(Vec2I const& v) {
  return JsonArray{v[0], v[1]};
}

Vec2U jsonToVec2U(Json const& v) {
  if (v.type() != Json::Type::Array || v.size() != 2)
    throw JsonException("Json not an array of size 2 in jsonToVec2I");

  return Vec2U(v.getInt(0), v.getInt(1));
}

Json jsonFromVec2U(Vec2U const& v) {
  return JsonArray{v[0], v[1]};
}

Vec2B jsonToVec2B(Json const& v) {
  if (v.type() != Json::Type::Array || v.size() != 2)
    throw JsonException("Json not an array of size 2 in jsonToVec2B");

  return Vec2B(v.getInt(0), v.getInt(1));
}

Json jsonFromVec2B(Vec2B const& v) {
  return JsonArray{v[0], v[1]};
}

Vec3D jsonToVec3D(Json const& v) {
  if (v.type() != Json::Type::Array || v.size() != 3)
    throw JsonException("Json not an array of size size 3 in jsonToVec3D");

  return Vec3D(v.getDouble(0), v.getDouble(1), v.getDouble(2));
}

Vec3F jsonToVec3F(Json const& v) {
  if (v.type() != Json::Type::Array || v.size() != 3)
    throw JsonException("Json not an array of size 3 in jsonToVec3D");

  return Vec3F(v.getFloat(0), v.getFloat(1), v.getFloat(2));
}

Json jsonFromVec3F(Vec3F const& v) {
  return JsonArray{v[0], v[1], v[2]};
}

Vec3I jsonToVec3I(Json const& v) {
  if (v.type() != Json::Type::Array || v.size() != 3)
    throw JsonException("Json not an array of size 3 in jsonToVec3I");

  return Vec3I(v.getInt(0), v.getInt(1), v.getInt(2));
}

Json jsonFromVec3I(Vec3I const& v) {
  JsonArray result;
  result.append(v[0]);
  result.append(v[1]);
  result.append(v[2]);
  return result;
}

Vec3B jsonToVec3B(Json const& v) {
  if (v.type() != Json::Type::Array || v.size() != 3)
    throw JsonException("Json not an array of size 3 in jsonToVec3B");

  return Vec3B(v.getInt(0), v.getInt(1), v.getInt(2));
}

Vec4B jsonToVec4B(Json const& v) {
  if (v.type() != Json::Type::Array || v.size() != 4)
    throw JsonException("Json not an array of size 4 in jsonToVec4B");

  return Vec4B(v.getInt(0), v.getInt(1), v.getInt(2), v.getInt(3));
}

Vec4I jsonToVec4I(Json const& v) {
  if (v.type() != Json::Type::Array || v.size() != 4)
    throw JsonException("Json not an array of size 4 in jsonToVec4B");

  return Vec4I(v.getInt(0), v.getInt(1), v.getInt(2), v.getInt(3));
}

Vec4F jsonToVec4F(Json const& v) {
  if (v.type() != Json::Type::Array || v.size() != 4)
    throw JsonException("Json not an array of size 4 in jsonToVec4B");

  return Vec4F(v.getFloat(0), v.getFloat(1), v.getFloat(2), v.getFloat(3));
}

RectD jsonToRectD(Json const& v) {
  if (v.type() != Json::Type::Array)
    throw JsonException("Json not an array in jsonToRectD");

  if (v.size() != 4 && v.size() != 2)
    throw JsonException("Json not an array of proper size in jsonToRectD");

  if (v.size() == 4)
    return RectD(v.getDouble(0), v.getDouble(1), v.getDouble(2), v.getDouble(3));

  try {
    auto lowerLeft = jsonToVec2D(v.get(0));
    auto upperRight = jsonToVec2D(v.get(1));
    return RectD(lowerLeft, upperRight);
  } catch (JsonException const& e) {
    throw JsonException(strf("Inner position not well formed in jsonToRectD: %s", outputException(e, true)));
  }
}

Json jsonFromRectD(RectD const& rect) {
  return JsonArray{rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax()};
}

RectF jsonToRectF(Json const& v) {
  return RectF(jsonToRectD(v));
}

Json jsonFromRectF(RectF const& rect) {
  return JsonArray{rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax()};
}

RectI jsonToRectI(Json const& v) {
  if (v.type() != Json::Type::Array)
    throw JsonException("Json not an array in jsonToRectI");

  if (v.size() != 4 && v.size() != 2)
    throw JsonException("Json not an array of proper size in jsonToRectI");

  if (v.size() == 4)
    return RectI(v.getInt(0), v.getInt(1), v.getInt(2), v.getInt(3));

  try {
    auto lowerLeft = jsonToVec2I(v.get(0));
    auto upperRight = jsonToVec2I(v.get(1));
    return RectI(lowerLeft, upperRight);
  } catch (JsonException const& e) {
    throw JsonException(strf("Inner position not well formed in jsonToRectI: %s", outputException(e, true)));
  }
}

Json jsonFromRectI(RectI const& rect) {
  return JsonArray{rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax()};
}

RectU jsonToRectU(Json const& v) {
  if (v.type() != Json::Type::Array)
    throw JsonException("Json not an array in jsonToRectU");

  if (v.size() != 4 && v.size() != 2)
    throw JsonException("Json not an array of proper size in jsonToRectU");

  if (v.size() == 4)
    return RectU(v.getInt(0), v.getUInt(1), v.getUInt(2), v.getUInt(3));

  try {
    auto lowerLeft = jsonToVec2U(v.get(0));
    auto upperRight = jsonToVec2U(v.get(1));
    return RectU(lowerLeft, upperRight);
  } catch (JsonException const& e) {
    throw JsonException(strf("Inner position not well formed in jsonToRectU: %s", outputException(e, true)));
  }
}

Json jsonFromRectU(RectU const& rect) {
  return JsonArray{rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax()};
}

Color jsonToColor(Json const& v) {
  if (v.type() == Json::Type::Array) {
    if (v.type() != Json::Type::Array || (v.size() != 3 && v.size() != 4))
      throw JsonException("Json not an array of size 3 or 4 in jsonToColor");
    Color c = Color::rgba(0, 0, 0, 255);

    c.setRed(v.getInt(0));
    c.setGreen(v.getInt(1));
    c.setBlue(v.getInt(2));

    if (v.size() == 4)
      c.setAlpha(v.getInt(3));

    return c;
  } else if (v.type() == Json::Type::String) {
    return Color(v.toString());
  } else {
    throw JsonException(strf("Json of type %s cannot be converted to color", v.typeName()));
  }
}

Json jsonFromColor(Color const& color) {
  JsonArray result;
  result.push_back(color.red());
  result.push_back(color.green());
  result.push_back(color.blue());
  if (color.alpha() != 255) {
    result.push_back(color.alpha());
  }
  return result;
}

PolyD jsonToPolyD(Json const& v) {
  PolyD poly;

  for (Json const& vertex : v.iterateArray())
    poly.add(jsonToVec2D(vertex));

  return fixInsideOutPoly(poly);
}

PolyF jsonToPolyF(Json const& v) {
  PolyF poly;

  for (Json const& vertex : v.iterateArray())
    poly.add(jsonToVec2F(vertex));

  return fixInsideOutPoly(poly);
}

PolyI jsonToPolyI(Json const& v) {
  PolyI poly;

  for (Json const& vertex : v.iterateArray())
    poly.add(jsonToVec2I(vertex));

  return fixInsideOutPoly(poly);
}

Json jsonFromPolyF(PolyF const& poly) {
  JsonArray vertexList;
  for (auto const& vertex : poly.vertexes())
    vertexList.append(JsonArray{vertex[0], vertex[1]});

  return vertexList;
}

Line2F jsonToLine2F(Json const& v) {
  return Line2F(jsonToVec2F(v.get(0)), jsonToVec2F(v.get(1)));
}

Json jsonFromLine2F(Line2F const& line) {
  return JsonArray{jsonFromVec2F(line.min()), jsonFromVec2F(line.max())};
}

Mat3F jsonToMat3F(Json const& v) {
  return Mat3F(jsonToVec3F(v.get(0)), jsonToVec3F(v.get(1)), jsonToVec3F(v.get(2)));
}

Json jsonFromMat3F(Mat3F const& v) {
  return JsonArray{jsonFromVec3F(v[0]), jsonFromVec3F(v[1]), jsonFromVec3F(v[2])};
}

StringList jsonToStringList(Json const& v) {
  StringList result;
  for (auto const& entry : v.iterateArray())
    result.push_back(entry.toString());
  return result;
}

Json jsonFromStringList(List<String> const& v) {
  JsonArray result;
  for (auto& e : v)
    result.push_back(e);
  return result;
}

List<float> jsonToFloatList(Json const& v) {
  List<float> result;
  for (auto const& entry : v.iterateArray())
    result.push_back(entry.toFloat());
  return result;
}

StringSet jsonToStringSet(Json const& v) {
  StringSet result;
  for (auto const& entry : v.iterateArray())
    result.add(entry.toString());
  return result;
}

Json jsonFromStringSet(StringSet const& v) {
  JsonArray result;
  for (auto& e : v)
    result.push_back(e);
  return result;
}

List<int> jsonToIntList(Json const& v) {
  List<int> result;
  for (auto const& entry : v.iterateArray())
    result.push_back(entry.toInt());
  return result;
}

List<Vec2I> jsonToVec2IList(Json const& v) {
  List<Vec2I> result;
  for (auto const& entry : v.iterateArray())
    result.append(jsonToVec2I(entry));
  return result;
}

List<Vec2U> jsonToVec2UList(Json const& v) {
  List<Vec2U> result;
  for (auto const& entry : v.iterateArray())
    result.append(jsonToVec2U(entry));
  return result;
}

List<Vec2F> jsonToVec2FList(Json const& v) {
  List<Vec2F> result;
  for (auto const& entry : v.iterateArray())
    result.append(jsonToVec2F(entry));
  return result;
}

List<Vec4B> jsonToVec4BList(Json const& v) {
  List<Vec4B> result;
  for (auto const& entry : v.iterateArray())
    result.append(jsonToVec4B(entry));
  return result;
}

List<Color> jsonToColorList(Json const& v) {
  List<Color> result;
  for (auto const& entry : v.iterateArray())
    result.append(jsonToColor(entry));
  return result;
}

List<Directives> jsonToDirectivesList(Json const& v) {
  List<Directives> result;
  for (auto const& entry : v.iterateArray())
    result.append(move(entry.toString()));
  return result;
}

Json jsonFromDirectivesList(List<Directives> const& v) {
  JsonArray result;
  for (auto& e : v)
    result.push_back(e.toString());
  return result;
}

Json weightedChoiceFromJson(Json const& source, Json const& default_) {
  if (source.isNull())
    return default_;
  if (source.type() != Json::Type::Array)
    throw StarException("Json of array type expected.");
  List<pair<float, Json>> options;
  float sum = 0;
  size_t idx = 0;
  while (idx < source.size()) {
    float weight = 1;
    Json entry = source.get(idx);
    if (entry.type() == Json::Type::Int || entry.type() == Json::Type::Float) {
      weight = entry.toDouble();
      idx++;
      if (idx >= source.size())
        throw StarException("Weighted companion cube cannot cry.");
      sum += weight;
      options.append(pair<float, Json>{weight, source.get(idx)});
    } else {
      sum += weight;
      options.append(pair<float, Json>{weight, entry});
    }
    idx++;
  }
  if (!options.size())
    return default_;
  float choice = Random::randf() * sum;
  idx = 0;
  while (idx < options.size()) {
    auto const& entry = options[idx];
    if (entry.first >= choice)
      return entry.second;
    choice -= entry.first;
    idx++;
  }
  return options[options.size() - 1].second;
}

Json binnedChoiceFromJson(Json const& bins, float target, Json const& def) {
  JsonArray binList = bins.toArray();
  sortByComputedValue(binList, [](Json const& pair) { return -pair.getFloat(0); });
  Json result = def;
  for (auto const& pair : binList) {
    if (pair.getFloat(0) <= target) {
      result = pair.get(1);
      break;
    }
  }
  return result;
}

template <>
WeightedPool<int> jsonToWeightedPool(Json const& source) {
  return jsonToWeightedPool<int>(source, [](Json const& v) { return v.toInt(); });
}

template <>
WeightedPool<unsigned> jsonToWeightedPool(Json const& source) {
  return jsonToWeightedPool<unsigned>(source, [](Json const& v) { return v.toUInt(); });
}

template <>
WeightedPool<float> jsonToWeightedPool(Json const& source) {
  return jsonToWeightedPool<float>(source, [](Json const& v) { return v.toFloat(); });
}

template <>
WeightedPool<double> jsonToWeightedPool(Json const& source) {
  return jsonToWeightedPool<double>(source, [](Json const& v) { return v.toDouble(); });
}

template <>
WeightedPool<String> jsonToWeightedPool(Json const& source) {
  return jsonToWeightedPool<String>(source, [](Json const& v) { return v.toString(); });
}

template <>
WeightedPool<JsonArray> jsonToWeightedPool(Json const& source) {
  return jsonToWeightedPool<JsonArray>(source, [](Json const& v) { return v.toArray(); });
}

template <>
WeightedPool<JsonObject> jsonToWeightedPool(Json const& source) {
  return jsonToWeightedPool<JsonObject>(source, [](Json const& v) { return v.toObject(); });
}

}

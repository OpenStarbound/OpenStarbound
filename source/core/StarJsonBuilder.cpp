#include "StarJsonBuilder.hpp"
#include "StarLexicalCast.hpp"

namespace Star {

void JsonBuilderStream::beginObject() {
  pushSentry();
}

void JsonBuilderStream::objectKey(char32_t const* s, size_t len) {
  push(Json(s, len));
}

void JsonBuilderStream::endObject() {
  JsonObject object;
  while (true) {
    if (isSentry()) {
      set(Json(move(object)));
      return;
    } else {
      Json v = pop();
      String k = pop().toString();
      if (!object.insert(k, move(v)).second)
        throw JsonParsingException(strf("Json object contains a duplicate entry for key '%s'", k));
    }
  }
}

void JsonBuilderStream::beginArray() {
  pushSentry();
}

void JsonBuilderStream::endArray() {
  JsonArray array;
  while (true) {
    if (isSentry()) {
      array.reverse();
      set(Json(move(array)));
      return;
    } else {
      array.append(pop());
    }
  }
}

void JsonBuilderStream::putString(char32_t const* s, size_t len) {
  push(Json(s, len));
}

void JsonBuilderStream::putDouble(char32_t const* s, size_t len) {
  push(Json(lexicalCast<double>(String(s, len))));
}

void JsonBuilderStream::putInteger(char32_t const* s, size_t len) {
  push(Json(lexicalCast<long long>(String(s, len))));
}

void JsonBuilderStream::putBoolean(bool b) {
  push(Json(b));
}

void JsonBuilderStream::putNull() {
  push(Json());
}

void JsonBuilderStream::putWhitespace(char32_t const*, size_t) {}

void JsonBuilderStream::putColon() {}

void JsonBuilderStream::putComma() {}

size_t JsonBuilderStream::stackSize() {
  return m_stack.size();
}

Json JsonBuilderStream::takeTop() {
  if (m_stack.size())
    return m_stack.takeLast().take();
  else
    return Json();
}

void JsonBuilderStream::push(Json v) {
  m_stack.append(move(v));
}

Json JsonBuilderStream::pop() {
  return m_stack.takeLast().take();
}

void JsonBuilderStream::set(Json v) {
  m_stack.last() = move(v);
}

void JsonBuilderStream::pushSentry() {
  m_stack.append({});
}

bool JsonBuilderStream::isSentry() {
  return !m_stack.empty() && !m_stack.last();
}

void JsonStreamer<Json>::toJsonStream(Json const& val, JsonStream& stream, bool sort) {
  Json::Type type = val.type();
  if (type == Json::Type::Null) {
    stream.putNull();
  } else if (type == Json::Type::Float) {
    auto d = String(toString(val.toDouble())).wideString();
    stream.putDouble(d.c_str(), d.length());
  } else if (type == Json::Type::Bool) {
    stream.putBoolean(val.toBool());
  } else if (type == Json::Type::Int) {
    auto i = String(toString(val.toInt())).wideString();
    stream.putInteger(i.c_str(), i.length());
  } else if (type == Json::Type::String) {
    auto ws = val.toString().wideString();
    stream.putString(ws.c_str(), ws.length());
  } else if (type == Json::Type::Array) {
    stream.beginArray();
    bool first = true;
    for (auto const& elem : val.iterateArray()) {
      if (!first)
        stream.putComma();
      first = false;
      toJsonStream(elem, stream, sort);
    }
    stream.endArray();
  } else if (type == Json::Type::Object) {
    stream.beginObject();
    List<String::Char> chars;
    if (sort) {
      auto objectPtr = val.objectPtr();
      List<JsonObject::const_iterator> iterators;
      iterators.reserve(objectPtr->size());
      for (auto i = objectPtr->begin(); i != objectPtr->end(); ++i)
        iterators.append(i);
      iterators.sort([](JsonObject::const_iterator a, JsonObject::const_iterator b) {
          return a->first < b->first;
        });
      bool first = true;
      for (auto const& i : iterators) {
        if (!first)
          stream.putComma();
        first = false;
        chars.clear();
        for (auto const& c : i->first)
          chars.push_back(c);
        stream.objectKey(chars.ptr(), chars.size());
        stream.putColon();
        toJsonStream(i->second, stream, sort);
      }
    } else {
      bool first = true;
      for (auto const& pair : val.iterateObject()) {
        if (!first)
          stream.putComma();
        first = false;
        auto ws = pair.first.wideString();
        stream.objectKey(ws.c_str(), ws.length());
        stream.putColon();
        toJsonStream(pair.second, stream, sort);
      }
    }
    stream.endObject();
  }
}

}

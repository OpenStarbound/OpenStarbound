#ifndef STAR_JSON_BUILDER_HPP
#define STAR_JSON_BUILDER_HPP

#include "StarJsonParser.hpp"
#include "StarJson.hpp"

namespace Star {

class JsonBuilderStream : public JsonStream {
public:
  virtual void beginObject();
  virtual void objectKey(char32_t const* s, size_t len);
  virtual void endObject();

  virtual void beginArray();
  virtual void endArray();

  virtual void putString(char32_t const* s, size_t len);
  virtual void putDouble(char32_t const* s, size_t len);
  virtual void putInteger(char32_t const* s, size_t len);
  virtual void putBoolean(bool b);
  virtual void putNull();

  virtual void putWhitespace(char32_t const* s, size_t len);
  virtual void putColon();
  virtual void putComma();

  size_t stackSize();
  Json takeTop();

private:
  void push(Json v);
  Json pop();
  void set(Json v);
  void pushSentry();
  bool isSentry();

  List<Maybe<Json>> m_stack;
};

template <typename Jsonlike>
class JsonStreamer {
public:
  static void toJsonStream(Jsonlike const& val, JsonStream& stream, bool sort);
};

template <>
class JsonStreamer<Json> {
public:
  static void toJsonStream(Json const& val, JsonStream& stream, bool sort);
};

template <typename InputIterator>
Json inputUtf8Json(InputIterator begin, InputIterator end, bool fragment) {
  typedef U8ToU32Iterator<InputIterator> Utf32Input;
  typedef JsonParser<Utf32Input> Parser;

  JsonBuilderStream stream;
  Parser parser(stream);
  Utf32Input wbegin(begin);
  Utf32Input wend(end);
  Utf32Input pend = parser.parse(wbegin, wend, fragment);

  if (parser.error())
    throw JsonParsingException(strf("Error parsing json: %s at %s:%s", parser.error(), parser.line(), parser.column()));
  else if (pend != wend)
    throw JsonParsingException(strf("Error extra data at end of input at %s:%s", parser.line(), parser.column()));

  return stream.takeTop();
}

template <typename OutputIterator>
void outputUtf8Json(Json const& val, OutputIterator out, int pretty, bool sort) {
  typedef Utf8OutputIterator<OutputIterator> Utf8Output;
  typedef JsonWriter<Utf8Output> Writer;
  Writer writer(Utf8Output(out), pretty);
  JsonStreamer<Json>::toJsonStream(val, writer, sort);
}

template <typename InputIterator, typename Stream = JsonBuilderStream, typename Jsonlike = Json>
Jsonlike inputUtf32Json(InputIterator begin, InputIterator end, bool fragment) {
  Stream stream;
  JsonParser<InputIterator> parser(stream);

  InputIterator pend = parser.parse(begin, end, fragment);

  if (parser.error()) {
    throw JsonParsingException(strf("Error parsing json: %s at %s:%s", parser.error(), parser.line(), parser.column()));
  } else if (pend != end) {
    throw JsonParsingException(strf("Error extra data at end of input at %s:%s", parser.line(), parser.column()));
  }

  return stream.takeTop();
}

template <typename OutputIterator, typename Jsonlike = Json>
void outputUtf32Json(Jsonlike const& val, OutputIterator out, int pretty, bool sort) {
  JsonWriter<OutputIterator> writer(out, pretty);
  JsonStreamer<Jsonlike>::toJsonStream(val, writer, sort);
}

}

#endif

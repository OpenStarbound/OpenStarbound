#include "StarFormattedJson.hpp"
#include "StarJsonBuilder.hpp"
#include "StarLexicalCast.hpp"

namespace Star {

class FormattedJsonBuilderStream : public JsonStream {
public:
  virtual void beginObject();
  virtual void objectKey(String::Char const* s, size_t len);
  virtual void endObject();

  virtual void beginArray();
  virtual void endArray();

  virtual void putString(String::Char const* s, size_t len);
  virtual void putDouble(String::Char const* s, size_t len);
  virtual void putInteger(String::Char const* s, size_t len);
  virtual void putBoolean(bool b);
  virtual void putNull();

  virtual void putWhitespace(String::Char const* s, size_t len);
  virtual void putComma();
  virtual void putColon();

  FormattedJson takeTop();

private:
  void push(FormattedJson const& v);
  FormattedJson pop();
  FormattedJson& current();
  void putValue(Json const& value, Maybe<String> formatting = {});

  Maybe<FormattedJson> m_root;
  List<FormattedJson> m_stack;
};

template <>
class JsonStreamer<FormattedJson> {
public:
  static void toJsonStream(FormattedJson const& val, JsonStream& stream, bool sort);
};

ValueElement::ValueElement(FormattedJson const& json) : value(make_shared<FormattedJson>(json)) {}

bool ValueElement::operator==(ValueElement const& v) const {
  return *value == *v.value;
}

bool ObjectKeyElement::operator==(ObjectKeyElement const& v) const {
  return key == v.key;
}

bool WhitespaceElement::operator==(WhitespaceElement const& v) const {
  return whitespace == v.whitespace;
}

bool ColonElement::operator==(ColonElement const&) const {
  return true;
}

bool CommaElement::operator==(CommaElement const&) const {
  return true;
}

FormattedJson FormattedJson::parse(String const& string) {
  return inputUtf32Json<String::const_iterator, FormattedJsonBuilderStream, FormattedJson>(
      string.begin(), string.end(), true);
}

FormattedJson FormattedJson::parseJson(String const& string) {
  return inputUtf32Json<String::const_iterator, FormattedJsonBuilderStream, FormattedJson>(
      string.begin(), string.end(), false);
}

FormattedJson FormattedJson::ofType(Json::Type type) {
  FormattedJson json;
  json.m_jsonValue = Json::ofType(type);
  return json;
}

FormattedJson::FormattedJson() : FormattedJson(Json()) {}

FormattedJson::FormattedJson(Json const& json)
  : m_jsonValue(Json::ofType(json.type())),
    m_elements(),
    m_formatting(),
    m_lastKey(),
    m_objectEntryLocations(),
    m_arrayElementLocations() {
  if (json.type() == Json::Type::Object || json.type() == Json::Type::Array) {
    FormattedJsonBuilderStream stream;
    JsonStreamer<Json>::toJsonStream(json, stream, false);
    FormattedJson parsed = stream.takeTop();
    for (JsonElement const& elem : parsed.elements()) {
      appendElement(elem);
    }
  }
  m_jsonValue = json;
}

Json const& FormattedJson::toJson() const {
  return m_jsonValue;
}

FormattedJson FormattedJson::get(String const& key) const {
  if (type() != Json::Type::Object)
    throw JsonException::format("Cannot call get with key on FormattedJson type {}, must be Object type", typeName());

  Maybe<pair<ElementLocation, ElementLocation>> entry = m_objectEntryLocations.maybe(key);
  if (entry.isNothing())
    throw JsonException::format("No such key in FormattedJson::get(\"{}\")", key);

  return getFormattedJson(entry->second);
}

FormattedJson FormattedJson::get(size_t index) const {
  if (type() != Json::Type::Array)
    throw JsonException::format("Cannot call get with index on FormattedJson type {}, must be Array type", typeName());

  if (index >= m_arrayElementLocations.size())
    throw JsonException::format("FormattedJson::get({}) out of range", index);

  ElementLocation loc = m_arrayElementLocations.at(index);
  return getFormattedJson(loc);
}

struct WhitespaceStyle {
  String beforeKey;
  String beforeColon;
  String beforeValue;
  String beforeComma;
};

template <class ElementType>
FormattedJson::ElementLocation indexOf(FormattedJson::ElementList const& elements, FormattedJson::ElementLocation pos) {
  for (; pos < elements.size(); ++pos) {
    if (elements[pos].is<ElementType>())
      return pos;
  }
  return NPos;
}

template <class ElementType>
FormattedJson::ElementLocation lastIndexOf(
    FormattedJson::ElementList const& elements, FormattedJson::ElementLocation pos) {
  while (pos > 0) {
    --pos;
    if (elements[pos].is<ElementType>())
      return pos;
  }
  return NPos;
}

String concatWhitespace(FormattedJson::ElementList const& elements, FormattedJson::ElementLocation from,
    FormattedJson::ElementLocation to) {
  String whitespace;
  for (JsonElement const& elem : elements.slice(from, to)) {
    if (elem.is<WhitespaceElement>())
      whitespace += elem.get<WhitespaceElement>().whitespace;
  }
  return whitespace;
}

WhitespaceStyle detectWhitespace(FormattedJson::ElementList const& elements,
    FormattedJson::ElementLocation insertLoc, bool array) {
  WhitespaceStyle style;

  // Find a nearby value as a reference location to learn whitespace from.
  FormattedJson::ElementLocation valueLoc = lastIndexOf<ValueElement>(elements, insertLoc);
  if (valueLoc == NPos)
    valueLoc = indexOf<ValueElement>(elements, insertLoc);

  if (valueLoc == NPos) {
    // This object/array is empty. Pre-key/value whitespace will be the total of
    // the whitespace already present, plus some guessed indentation if it
    // contained a newline.
    String beforeValue = concatWhitespace(elements, 0, elements.size());
    if (beforeValue.find('\n') != NPos)
      beforeValue += "  ";
    if (array)
      return WhitespaceStyle{"", "", beforeValue, ""};
    return WhitespaceStyle{beforeValue, "", "", ""};
  }

  FormattedJson::ElementLocation commaLoc = indexOf<CommaElement>(elements, valueLoc);
  if (commaLoc != NPos) {
    style.beforeComma = concatWhitespace(elements, valueLoc + 1, commaLoc);
  }

  FormattedJson::ElementLocation colonLoc = lastIndexOf<ColonElement>(elements, valueLoc);
  starAssert((colonLoc == NPos) == array);
  if (colonLoc != NPos) {
    style.beforeValue = concatWhitespace(elements, colonLoc + 1, valueLoc);

    FormattedJson::ElementLocation keyLoc = lastIndexOf<ObjectKeyElement>(elements, colonLoc);
    starAssert(keyLoc != NPos);
    style.beforeColon = concatWhitespace(elements, keyLoc + 1, colonLoc);

    FormattedJson::ElementLocation prevValueLoc = lastIndexOf<ValueElement>(elements, keyLoc);
    if (prevValueLoc == NPos)
      prevValueLoc = 0;
    style.beforeKey = concatWhitespace(elements, prevValueLoc, keyLoc);

  } else {
    FormattedJson::ElementLocation prevValueLoc = lastIndexOf<ValueElement>(elements, valueLoc);
    if (prevValueLoc == NPos)
      prevValueLoc = 0;
    style.beforeValue = concatWhitespace(elements, prevValueLoc, valueLoc);
  }

  return style;
}

void insertWhitespace(FormattedJson::ElementList& destination, FormattedJson::ElementLocation& at, String const& whitespace) {
  if (whitespace == "")
    return;
  destination.insertAt(at++, WhitespaceElement{whitespace});
}

void insertWithWhitespace(FormattedJson::ElementList& destination, WhitespaceStyle const& style,
    FormattedJson::ElementLocation& at, JsonElement const& element) {
  if (element.is<ValueElement>())
    insertWhitespace(destination, at, style.beforeValue);
  if (element.is<ObjectKeyElement>())
    insertWhitespace(destination, at, style.beforeKey);
  if (element.is<ColonElement>())
    insertWhitespace(destination, at, style.beforeColon);
  if (element.is<CommaElement>())
    insertWhitespace(destination, at, style.beforeComma);
  destination.insertAt(at++, element);
}

void insertWithCommaAndFormatting(FormattedJson::ElementList& destination, FormattedJson::ElementLocation at,
    bool array, FormattedJson::ElementList const& elements) {
  // Find the previous value we're inserting after, if any.
  at = lastIndexOf<ValueElement>(destination, at);
  if (at == NPos)
    at = 0;
  else
    at += 1;
  bool empty = lastIndexOf<ValueElement>(destination, destination.size()) == NPos;
  bool appendComma = at == 0 && !empty;
  bool prependComma = !appendComma && !empty;

  WhitespaceStyle style = detectWhitespace(destination, at, array);

  if (prependComma) {
    // Inserting into a non-empty object/array. Prepend a comma
    insertWithWhitespace(destination, style, at, CommaElement{});
  }
  for (JsonElement const& elem : elements) {
    insertWithWhitespace(destination, style, at, elem);
  }
  if (appendComma) {
    insertWithWhitespace(destination, style, at, CommaElement{});
  }
}

FormattedJson FormattedJson::prepend(String const& key, FormattedJson const& value) const {
  return objectInsert(key, value, 0);
}

FormattedJson FormattedJson::insertBefore(String const& key, FormattedJson const& value, String const& beforeKey) const {
  if (!m_objectEntryLocations.contains(beforeKey))
    throw JsonException::format("Cannot insert before key \"{}\", which does not exist", beforeKey);
  ElementLocation loc = m_objectEntryLocations.get(beforeKey).first;
  return objectInsert(key, value, loc);
}

FormattedJson FormattedJson::insertAfter(String const& key, FormattedJson const& value, String const& afterKey) const {
  if (!m_objectEntryLocations.contains(afterKey))
    throw JsonException::format("Cannot insert after key \"{}\", which does not exist", afterKey);
  ElementLocation loc = m_objectEntryLocations.get(afterKey).second;
  return objectInsert(key, value, loc + 1);
}

FormattedJson FormattedJson::append(String const& key, FormattedJson const& value) const {
  return objectInsert(key, value, m_elements.size());
}

FormattedJson FormattedJson::set(String const& key, FormattedJson const& value) const {
  return objectInsert(key, value, m_elements.size());
}

void removeValueFromArray(List<JsonElement>& elements, size_t loc) {
  // Remove the value itself, the comma following and the whitespace up to the
  // next value.
  // If it's the last value, it removes the value, and the preceding whitespace
  // and comma.
  size_t commaLoc = elements.indexOf(CommaElement{}, loc);
  if (commaLoc != NPos) {
    elements.eraseAt(loc, commaLoc + 1);
    while (loc < elements.size() && elements.at(loc).is<WhitespaceElement>())
      elements.eraseAt(loc);
  } else {
    commaLoc = elements.lastIndexOf(CommaElement{}, loc);
    if (commaLoc == NPos)
      commaLoc = 0;
    elements.eraseAt(commaLoc, loc + 1);
  }
}

FormattedJson FormattedJson::eraseKey(String const& key) const {
  if (type() != Json::Type::Object)
    throw JsonException::format("Cannot call erase with key on FormattedJson type {}, must be Object type", typeName());

  Maybe<pair<ElementLocation, ElementLocation>> maybeEntry = m_objectEntryLocations.maybe(key);
  if (maybeEntry.isNothing())
    return *this;

  ElementLocation loc = maybeEntry->first;
  ElementList elements = m_elements;
  elements.eraseAt(loc, maybeEntry->second); // Remove key, colon and whitespace up to the value
  removeValueFromArray(elements, loc);
  return object(elements);
}

FormattedJson FormattedJson::insert(size_t index, FormattedJson const& value) const {
  if (type() != Json::Type::Array)
    throw JsonException::format(
        "Cannot call insert with index on FormattedJson type {}, must be Array type", typeName());

  if (index > m_arrayElementLocations.size())
    throw JsonException::format("FormattedJson::insert({}) out of range", index);

  ElementList elements = m_elements;
  ElementLocation insertPosition = elements.size();
  if (index < m_arrayElementLocations.size())
    insertPosition = m_arrayElementLocations.at(index);

  insertWithCommaAndFormatting(elements, insertPosition, true, {ValueElement{value}});
  return array(elements);
}

FormattedJson FormattedJson::append(FormattedJson const& value) const {
  if (type() != Json::Type::Array)
    throw JsonException::format("Cannot call append on FormattedJson type {}, must be Array type", typeName());

  ElementList elements = m_elements;
  insertWithCommaAndFormatting(elements, elements.size(), true, {ValueElement{value}});
  return array(elements);
}

FormattedJson FormattedJson::set(size_t index, FormattedJson const& value) const {
  if (type() != Json::Type::Array)
    throw JsonException::format("Cannot call set with index on FormattedJson type {}, must be Array type", typeName());

  if (index >= m_arrayElementLocations.size())
    throw JsonException::format("FormattedJson::set({}) out of range", index);

  ElementLocation loc = m_arrayElementLocations.at(index);
  ElementList elements = m_elements;
  elements.at(loc) = ValueElement{value};
  return array(elements);
}

FormattedJson FormattedJson::eraseIndex(size_t index) const {
  if (type() != Json::Type::Array)
    throw JsonException::format("Cannot call set with index on FormattedJson type {}, must be Array type", typeName());

  if (index >= m_arrayElementLocations.size())
    throw JsonException::format("FormattedJson::eraseIndex({}) out of range", index);

  ElementLocation loc = m_arrayElementLocations.at(index);
  ElementList elements = m_elements;
  removeValueFromArray(elements, loc);
  return array(elements);
}

size_t FormattedJson::size() const {
  return m_jsonValue.size();
}

bool FormattedJson::contains(String const& key) const {
  return m_jsonValue.contains(key);
}

Json::Type FormattedJson::type() const {
  return m_jsonValue.type();
}

bool FormattedJson::isType(Json::Type type) const {
  return m_jsonValue.isType(type);
}

String FormattedJson::typeName() const {
  return m_jsonValue.typeName();
}

String FormattedJson::toFormattedDouble() const {
  if (!isType(Json::Type::Float))
    throw JsonException::format("Cannot call toFormattedDouble on Json type {}, must be Float", typeName());
  if (m_formatting.isValid())
    return *m_formatting;
  return toJson().repr();
}

String FormattedJson::toFormattedInt() const {
  if (!isType(Json::Type::Int))
    throw JsonException::format("Cannot call toFormattedInt on Json type {}, must be Int", typeName());
  if (m_formatting.isValid())
    return *m_formatting;
  return toJson().repr();
}

String FormattedJson::repr() const {
  if (m_formatting.isValid())
    return *m_formatting;
  String result;
  outputUtf32Json<std::back_insert_iterator<String>, FormattedJson>(*this, std::back_inserter(result), 0, false);
  return result;
}

String FormattedJson::printJson() const {
  if (type() != Json::Type::Object && type() != Json::Type::Array)
    throw JsonException("printJson called on non-top-level JSON type");
  return repr();
}

Json elemToJson(JsonElement const& elem) {
  return elem.get<ValueElement>().value->toJson();
}

FormattedJson::ElementList const& FormattedJson::elements() const {
  return m_elements;
}

bool FormattedJson::operator==(FormattedJson const& v) const {
  return m_jsonValue == v.m_jsonValue;
}

bool FormattedJson::operator!=(FormattedJson const& v) const {
  return !(*this == v);
}

FormattedJson FormattedJson::object(ElementList const& elements) {
  FormattedJson json = ofType(Json::Type::Object);
  for (JsonElement const& elem : elements) {
    json.appendElement(elem);
  }
  return json;
}

FormattedJson FormattedJson::array(ElementList const& elements) {
  FormattedJson json = ofType(Json::Type::Array);
  for (JsonElement const& elem : elements) {
    if (elem.is<ColonElement>() || elem.is<ObjectKeyElement>())
      throw JsonException("Invalid FormattedJson element in Json array");
    json.appendElement(elem);
  }
  return json;
}

FormattedJson FormattedJson::objectInsert(String const& key, FormattedJson const& value, ElementLocation loc) const {
  if (type() != Json::Type::Object)
    throw JsonException::format("Cannot call set with key on FormattedJson type {}, must be Object type", typeName());

  Maybe<pair<ElementLocation, ElementLocation>> maybeEntry = m_objectEntryLocations.maybe(key);
  if (maybeEntry.isValid()) {
    ElementList elements = m_elements;
    elements.at(maybeEntry->second) = ValueElement{value};
    return object(elements);
  }

  ElementList elements = m_elements;
  insertWithCommaAndFormatting(elements, loc, false, {ObjectKeyElement{key}, ColonElement{}, ValueElement{value}});
  return object(elements);
}

void FormattedJson::appendElement(JsonElement const& elem) {
  ElementLocation loc = m_elements.size();
  m_elements.append(elem);

  if (elem.is<ObjectKeyElement>()) {
    starAssert(isType(Json::Type::Object));
    m_lastKey = loc;

  } else if (elem.is<ValueElement>()) {
    m_lastValue = loc;

    if (m_lastKey.isValid()) {
      starAssert(isType(Json::Type::Object));
      String key = m_elements[*m_lastKey].get<ObjectKeyElement>().key;

      m_objectEntryLocations[key] = make_pair(*m_lastKey, loc);
      m_jsonValue = m_jsonValue.set(key, elemToJson(elem));

      m_lastKey = {};
    } else {
      starAssert(isType(Json::Type::Array));
      m_arrayElementLocations.append(loc);

      m_jsonValue = m_jsonValue.append(elemToJson(elem));
    }
  }
}

FormattedJson const& FormattedJson::getFormattedJson(ElementLocation loc) const {
  return *m_elements[loc].get<ValueElement>().value;
}

FormattedJson FormattedJson::formattedAs(String const& formatting) const {
  starAssert(Json::parse(formatting) == toJson());
  FormattedJson json = *this;
  json.m_formatting = formatting;
  return json;
}

void FormattedJsonBuilderStream::beginObject() {
  FormattedJson value = FormattedJson::ofType(Json::Type::Object);
  push(value);
}

void FormattedJsonBuilderStream::objectKey(String::Char const* s, size_t len) {
  current().appendElement(ObjectKeyElement{String(s, len)});
}

void FormattedJsonBuilderStream::endObject() {
  FormattedJson value = pop();
  if (m_stack.size() > 0)
    current().appendElement(ValueElement{value});
  else
    m_root = value;
}

void FormattedJsonBuilderStream::beginArray() {
  FormattedJson value = FormattedJson::ofType(Json::Type::Array);
  push(value);
}

void FormattedJsonBuilderStream::endArray() {
  FormattedJson value = pop();
  if (m_stack.size() > 0)
    current().appendElement(ValueElement{value});
  else
    m_root = value;
}

void FormattedJsonBuilderStream::putString(String::Char const* s, size_t len) {
  putValue(String(s, len));
}

void FormattedJsonBuilderStream::putDouble(String::Char const* s, size_t len) {
  String formatted(s, len);
  double d = lexicalCast<double>(formatted);
  putValue(d, formatted);
}

void FormattedJsonBuilderStream::putInteger(String::Char const* s, size_t len) {
  String formatted(s, len);
  long long d = lexicalCast<long long>(formatted);
  putValue(d, formatted);
}

void FormattedJsonBuilderStream::putBoolean(bool b) {
  putValue(b);
}

void FormattedJsonBuilderStream::putNull() {
  putValue(Json::ofType(Json::Type::Null));
}

void FormattedJsonBuilderStream::putWhitespace(String::Char const* s, size_t len) {
  if (m_stack.size() > 0)
    current().appendElement(WhitespaceElement{String(s, len)});
}

void FormattedJsonBuilderStream::putColon() {
  current().appendElement(ColonElement{});
}

void FormattedJsonBuilderStream::putComma() {
  current().appendElement(CommaElement{});
}

FormattedJson FormattedJsonBuilderStream::takeTop() {
  return m_root.take();
}

void FormattedJsonBuilderStream::push(FormattedJson const& v) {
  m_stack.push_back(v);
}

FormattedJson FormattedJsonBuilderStream::pop() {
  FormattedJson result = m_stack.back();
  m_stack.pop_back();
  return result;
}

FormattedJson& FormattedJsonBuilderStream::current() {
  return m_stack.back();
}

void FormattedJsonBuilderStream::putValue(Json const& value, Maybe<String> formatting) {
  FormattedJson formattedValue = value;
  if (formatting.isValid())
    formattedValue = formattedValue.formattedAs(*formatting);

  if (m_stack.size() > 0)
    current().appendElement(ValueElement{formattedValue});
  else {
    m_root = formattedValue;
  }
}

void JsonStreamer<FormattedJson>::toJsonStream(FormattedJson const& val, JsonStream& stream, bool sort) {
  if (val.isType(Json::Type::Object))
    stream.beginObject();

  else if (val.isType(Json::Type::Array))
    stream.beginArray();

  else if (val.isType(Json::Type::Float)) {
    // Float and Int are to be formatted the same way they were parsed to
    // preserve, e.g. negative zeroes and trailing 0 digits on decimals.
    auto ws = val.toFormattedDouble().wideString();
    stream.putDouble(ws.c_str(), ws.length());
    return;

  } else if (val.isType(Json::Type::Int)) {
    auto ws = val.toFormattedInt().wideString();
    stream.putInteger(ws.c_str(), ws.length());
    return;

  } else {
    // If val is not an object, array or number, it has no formatting and no
    // elements. Stream the wrapped Json value the usual way.
    JsonStreamer<Json>::toJsonStream(val.toJson(), stream, sort);
    return;
  }

  for (JsonElement elem : val.elements()) {
    if (elem.is<ObjectKeyElement>()) {
      String::WideString key = elem.get<ObjectKeyElement>().key.wideString();
      stream.objectKey(key.c_str(), key.length());
    } else if (elem.is<WhitespaceElement>()) {
      String::WideString white = elem.get<WhitespaceElement>().whitespace.wideString();
      stream.putWhitespace(white.c_str(), white.length());
    } else if (elem.is<ColonElement>()) {
      stream.putColon();
    } else if (elem.is<CommaElement>()) {
      stream.putComma();
    } else {
      toJsonStream(*elem.get<ValueElement>().value, stream, sort);
    }
  }

  if (val.isType(Json::Type::Object))
    stream.endObject();
  if (val.isType(Json::Type::Array))
    stream.endArray();
}

std::ostream& operator<<(std::ostream& os, JsonElement const& elem) {
  if (elem.is<ValueElement>())
    return os << "ValueElement{" << elem.get<ValueElement>().value << "}";
  if (elem.is<ObjectKeyElement>())
    return os << "ObjectKeyElement{" << elem.get<ObjectKeyElement>().key << "}";
  if (elem.is<WhitespaceElement>())
    return os << "WhitespaceElement{" << elem.get<WhitespaceElement>().whitespace << "}";
  if (elem.is<ColonElement>())
    return os << "ColonElement{}";
  if (elem.is<CommaElement>())
    return os << "CommaElement{}";
  starAssert(false);
  return os;
}

std::ostream& operator<<(std::ostream& os, FormattedJson const& json) {
  return os << json.repr();
}

}

#pragma once

#include <list>

#include "StarJson.hpp"

namespace Star {

STAR_CLASS(FormattedJson);

struct ObjectElement;
struct ObjectKeyElement;
struct ValueElement;
struct WhitespaceElement;
struct ColonElement;
struct CommaElement;

typedef Variant<ValueElement, ObjectKeyElement, WhitespaceElement, ColonElement, CommaElement> JsonElement;

struct ValueElement {
  ValueElement(FormattedJson const& json);

  FormattedJsonPtr value;

  bool operator==(ValueElement const& v) const;
};

struct ObjectKeyElement {
  String key;

  bool operator==(ObjectKeyElement const& v) const;
};

struct WhitespaceElement {
  String whitespace;

  bool operator==(WhitespaceElement const& v) const;
};

struct ColonElement {
  bool operator==(ColonElement const&) const;
};

struct CommaElement {
  bool operator==(CommaElement const&) const;
};

std::ostream& operator<<(std::ostream& os, JsonElement const& elem);

// Class representing formatted JSON data. Preserves whitespace and comments.
class FormattedJson {
public:
  typedef List<JsonElement> ElementList;
  typedef size_t ElementLocation;

  static FormattedJson parse(String const& string);
  static FormattedJson parseJson(String const& string);

  static FormattedJson ofType(Json::Type type);

  FormattedJson();
  FormattedJson(Json const&);

  Json const& toJson() const;

  FormattedJson get(String const& key) const;
  FormattedJson get(size_t index) const;

  // Returns a new FormattedJson with the given values added or erased.
  // Prepend, insert and append update the value in-place if the key already
  // exists.
  FormattedJson prepend(String const& key, FormattedJson const& value) const;
  FormattedJson insertBefore(String const& key, FormattedJson const& value, String const& beforeKey) const;
  FormattedJson insertAfter(String const& key, FormattedJson const& value, String const& afterKey) const;
  FormattedJson append(String const& key, FormattedJson const& value) const;
  FormattedJson set(String const& key, FormattedJson const& value) const;
  FormattedJson eraseKey(String const& key) const;

  FormattedJson insert(size_t index, FormattedJson const& value) const;
  FormattedJson append(FormattedJson const& value) const;
  FormattedJson set(size_t index, FormattedJson const& value) const;
  FormattedJson eraseIndex(size_t index) const;

  // Returns the number of elements in a Json array, or entries in an object.
  size_t size() const;

  bool contains(String const& key) const;

  Json::Type type() const;
  bool isType(Json::Type type) const;
  String typeName() const;

  String toFormattedDouble() const;
  String toFormattedInt() const;

  String repr() const;
  String printJson() const;

  ElementList const& elements() const;

  // Equality ignores whitespace and formatting. It just compares the Json
  // values.
  bool operator==(FormattedJson const& v) const;
  bool operator!=(FormattedJson const& v) const;

private:
  friend class FormattedJsonBuilderStream;

  static FormattedJson object(ElementList const& elements);
  static FormattedJson array(ElementList const& elements);

  FormattedJson objectInsert(String const& key, FormattedJson const& value, ElementLocation loc) const;
  void appendElement(JsonElement const& elem);

  FormattedJson const& getFormattedJson(ElementLocation loc) const;
  FormattedJson formattedAs(String const& formatting) const;

  Json m_jsonValue;
  ElementList m_elements;
  // Used to preserve the formatting of numbers, i.e. -0 vs 0, 1.0 vs 1:
  Maybe<String> m_formatting;

  Maybe<ElementLocation> m_lastKey, m_lastValue;
  Map<String, pair<ElementLocation, ElementLocation>> m_objectEntryLocations;
  List<ElementLocation> m_arrayElementLocations;
};

std::ostream& operator<<(std::ostream& os, FormattedJson const& json);

}

template <> struct fmt::formatter<Star::FormattedJson> : ostream_formatter {};

#include "StarImage.hpp"
#include "StarImageProcessing.hpp"
#include "StarDirectives.hpp"
#include "StarXXHash.hpp"

namespace Star {

Directives::Entry::Entry(ImageOperation&& newOperation, String&& newString) {
  operation = move(newOperation);
  string = move(newString);
}

Directives::Entry::Entry(ImageOperation const& newOperation, String const& newString) {
  operation = newOperation;
  string = newString;
}

Directives::Entry::Entry(Entry const& other) {
  operation = other.operation;
  string = other.string;
}

Directives::Directives() {}
Directives::Directives(String const& directives) {
  parse(directives);
}

Directives::Directives(String&& directives) {
  String mine = move(directives);
  parse(mine);
}

Directives::Directives(const char* directives) {
  String string(directives);
  parse(string);
}

Directives::Directives(List<Entry>&& newEntries) {
  entries = std::make_shared<List<Entry> const>(move(newEntries));
  String directives = toString(); // This needs to be better
  hash = XXH3_64bits(directives.utf8Ptr(), directives.utf8Size());
}

void Directives::parse(String const& directives) {
  List<Entry> newList;
  StringList split = directives.split('?');
  newList.reserve(split.size());
  for (String& str : split) {
    if (!str.empty()) {
      ImageOperation operation = imageOperationFromString(str);
      newList.emplace_back(move(operation), move(str));
    }
  }
  entries = std::make_shared<List<Entry> const>(move(newList));
  hash = XXH3_64bits(directives.utf8Ptr(), directives.utf8Size());
}

void Directives::buildString(String& out) const {
  for (auto& entry : *entries) {
    out += "?";
    out += entry.string;
  }
}

String Directives::toString() const {
  String result;
  buildString(result);
  return result;
}

inline bool Directives::empty() const {
  return entries->empty();
}


DataStream& operator>>(DataStream& ds, Directives& directives) {
  String string;
  ds.read(string);

  directives.parse(string);

  return ds;
}

DataStream& operator<<(DataStream& ds, Directives const& directives) {
  ds.write(directives.toString());

  return ds;
}

DirectivesGroup::DirectivesGroup() : m_count(0) {}
DirectivesGroup::DirectivesGroup(String const& directives) {
  m_directives.emplace_back(directives);
  m_count = m_directives.back().entries->size();
}
DirectivesGroup::DirectivesGroup(String&& directives) {
  m_directives.emplace_back(move(directives));
  m_count = m_directives.back().entries->size();
}

inline bool DirectivesGroup::empty() const {
  return m_count == 0;
}

inline bool DirectivesGroup::compare(DirectivesGroup const& other) const {
  if (m_count != other.m_count)
    return false;

  if (empty())
    return true;

  return hash() == other.hash();
}

void DirectivesGroup::append(Directives const& directives) {
  m_directives.emplace_back(directives);
  m_count += m_directives.back().entries->size();
}

void DirectivesGroup::append(List<Directives::Entry>&& entries) {
  size_t size = entries.size();
  m_directives.emplace_back(move(entries));
  m_count += size;
}

void DirectivesGroup::clear() {
  m_directives.clear();
  m_count = 0;
}

DirectivesGroup& DirectivesGroup::operator+=(Directives const& other) {
  append(other);
  return *this;
}

inline String DirectivesGroup::toString() const {
  String string;
  addToString(string);
  return string;
}

void DirectivesGroup::addToString(String& string) const {
  for (auto& directives : m_directives)
    directives.buildString(string);
}

void DirectivesGroup::forEach(DirectivesCallback callback) const {
  for (auto& directives : m_directives) {
    for (auto& entry : *directives.entries)
      callback(entry);
  }
}

bool DirectivesGroup::forEachAbortable(AbortableDirectivesCallback callback) const {
  for (auto& directives : m_directives) {
    for (auto& entry : *directives.entries) {
      if (!callback(entry))
        return false;
    }
  }

  return true;
}

inline Image DirectivesGroup::applyNewImage(Image const& image) const {
  Image result = image;
  applyExistingImage(result);
  return result;
}

void DirectivesGroup::applyExistingImage(Image& image) const {
  forEach([&](auto const& entry) {
    processImageOperation(entry.operation, image);
  });
}

inline size_t DirectivesGroup::hash() const {
  XXHash3 hasher;
  for (auto& directives : m_directives)
    hasher.push((const char*)&directives.hash, sizeof(directives.hash));

  return hasher.digest();
}

bool operator==(DirectivesGroup const& a, DirectivesGroup const& b) {
  return a.compare(b);
}

bool operator!=(DirectivesGroup const& a, DirectivesGroup const& b) {
  return !a.compare(b);
}

size_t hash<DirectivesGroup>::operator()(DirectivesGroup const& s) const {
  return s.hash();
}

}
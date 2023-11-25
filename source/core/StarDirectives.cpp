#include "StarDirectives.hpp"
#include "StarImage.hpp"
#include "StarImageProcessing.hpp"
#include "StarXXHash.hpp"
#include "StarLogging.hpp"

namespace Star {

Directives::Entry::Entry(ImageOperation&& newOperation, size_t strBegin, size_t strLength) {
  operation = move(newOperation);
  begin = strBegin;
  length = strLength;
}

Directives::Entry::Entry(ImageOperation const& newOperation, size_t strBegin, size_t strLength) {
  operation = newOperation;
  begin = strBegin;
  length = strLength;
}

Directives::Entry::Entry(Entry const& other) {
  operation = other.operation;
  begin = other.begin;
  length = other.length;
}

ImageOperation const& Directives::Entry::loadOperation(Shared const& parent) const {
  if (operation.is<NullImageOperation>()) {
    try { operation = imageOperationFromString(string(parent)); }
    catch (StarException const& e) { operation = ErrorImageOperation{ std::current_exception() }; }
  }
  return operation;
}

StringView Directives::Entry::string(Shared const& parent) const {
  StringView result = parent.string;
  result = result.utf8().substr(begin, length);
  return result;
}

bool Directives::Shared::empty() const {
  return entries.empty();
}

Directives::Shared::Shared(List<Entry>&& givenEntries, String&& givenString, StringView givenPrefix) {
  entries = move(givenEntries);
  string = move(givenString);
  prefix = givenPrefix;
  hash = XXH3_64bits(string.utf8Ptr(), string.utf8Size());
}

Directives::Directives() {}
Directives::Directives(String const& directives) {
  parse(String(directives));
}

Directives::Directives(String&& directives) {
  parse(move(directives));
}

Directives::Directives(const char* directives) {
  parse(directives);
}

Directives& Directives::operator=(String const& directives) {
  if (shared && shared->string == directives)
    return *this;

  parse(String(directives));
  return *this;
}

Directives& Directives::operator=(String&& directives) {
  if (shared && shared->string == directives) {
    directives.clear();
    return *this;
  }

  parse(move(directives));
  return *this;
}

Directives& Directives::operator=(const char* directives) {
  if (shared && shared->string.utf8().compare(directives) == 0)
    return *this;

  parse(directives);
  return *this;
}

void Directives::loadOperations() const {
  if (!shared)
    return;

  MutexLocker lock(shared->mutex, true);
  for (auto& entry : shared->entries)
    entry.loadOperation(*shared);
}

void Directives::parse(String&& directives) {
  if (directives.empty()) {
    shared.reset();
    return;
  }

  List<Entry> newList;
  StringView view(directives);
  StringView prefix = "";
  view.forEachSplitView("?", [&](StringView split, size_t beg, size_t end) {
    if (!split.empty()) {
      if (beg == 0) {
        try {
          ImageOperation operation = imageOperationFromString(split);
          newList.emplace_back(move(operation), beg, end);
        }
        catch (StarException const& e) {
          prefix = split;
          return;
        }
      }
      else {
        ImageOperation operation = NullImageOperation();
        newList.emplace_back(move(operation), beg, end);
      }
    }
  });

  if (newList.empty() && !prefix.empty()) {
    shared.reset();
    return;
  }

  shared = std::make_shared<Shared const>(move(newList), move(directives), prefix);
  if (view.size() < 1000) { // Pre-load short enough directives
    for (auto& entry : shared->entries)
      entry.loadOperation(*shared);
  }
}

String Directives::string() const {
  if (!shared)
    return "";
  else
    return shared->string;
}

StringView Directives::prefix() const {
  if (!shared)
    return "";
  else
    return shared->prefix;
}

String const* Directives::stringPtr() const {
  if (!shared)
    return nullptr;
  else
    return &shared->string;
}


String Directives::buildString() const {
  if (shared) {
    String built = shared->prefix;

    for (auto& entry : shared->entries) {
      built += "?";
      built += entry.string(*shared);
    }

    return built;
  }

  return String();
}

String& Directives::addToString(String& out) const {
  if (!empty())
    out += shared->string;
  return out;
}

size_t Directives::hash() const {
  return shared ? shared->hash : 0;
}

size_t Directives::size() const {
  return shared ? shared->entries.size() : 0;
}

bool Directives::empty() const {
  return !shared || shared->empty();
}

Directives::operator bool() const { return !empty(); }

DataStream& operator>>(DataStream& ds, Directives& directives) {
  String string;
  ds.read(string);

  directives.parse(move(string));

  return ds;
}

DataStream& operator<<(DataStream & ds, Directives const& directives) {
  if (directives)
    ds.write(directives.shared->string);
  else
    ds.write(String());

  return ds;
}

DirectivesGroup::DirectivesGroup() : m_count(0) {}
DirectivesGroup::DirectivesGroup(String const& directives) : m_count(0) {
  if (directives.empty())
    return;

  Directives parsed(directives);
  if (parsed) {
    m_directives.emplace_back(move(parsed));
    m_count = m_directives.back().size();
  }
}
DirectivesGroup::DirectivesGroup(String&& directives) : m_count(0) {
  if (directives.empty()) {
    directives.clear();
    return;
  }

  Directives parsed(move(directives));
  if (parsed) {
    m_directives.emplace_back(move(parsed));
    m_count = m_directives.back().size();
  }
}

bool DirectivesGroup::empty() const {
  return m_count == 0;
}

DirectivesGroup::operator bool() const {
  return empty();
}

bool DirectivesGroup::compare(DirectivesGroup const& other) const {
  if (m_count != other.m_count)
    return false;

  if (empty())
    return true;

  return hash() == other.hash();
}

void DirectivesGroup::append(Directives const& directives) {
  m_directives.emplace_back(directives);
  m_count += m_directives.back().size();
}

void DirectivesGroup::clear() {
  m_directives.clear();
  m_count = 0;
}

DirectivesGroup& DirectivesGroup::operator+=(Directives const& other) {
  append(other);
  return *this;
}

String DirectivesGroup::toString() const {
  String string;
  addToString(string);
  return string;
}

void DirectivesGroup::addToString(String& string) const {
  for (auto& directives : m_directives)
    if (directives.shared) {
      auto& dirString = directives.shared->string;
      if (!dirString.empty()) {
        if (dirString.utf8().front() != '?')
          string += "?";
        string += dirString;
      }
    }
}

void DirectivesGroup::forEach(DirectivesCallback callback) const {
  for (auto& directives : m_directives) {
    if (directives.shared) {
      for (auto& entry : directives.shared->entries)
        callback(entry, directives);
    }
  }
}

bool DirectivesGroup::forEachAbortable(AbortableDirectivesCallback callback) const {
  for (auto& directives : m_directives) {
    if (directives.shared) {
      for (auto& entry : directives.shared->entries) {
        if (!callback(entry, directives))
          return false;
      }
    }
  }

  return true;
}

Image DirectivesGroup::applyNewImage(Image const& image) const {
  Image result = image;
  applyExistingImage(result);
  return result;
}

void DirectivesGroup::applyExistingImage(Image& image) const {
  forEach([&](auto const& entry, Directives const& directives) {
    ImageOperation const& operation = entry.loadOperation(*directives.shared);
    if (auto error = operation.ptr<ErrorImageOperation>())
      std::rethrow_exception(error->exception);
    else
      processImageOperation(operation, image);
  });
}

size_t DirectivesGroup::hash() const {
  XXHash3 hasher;
  for (auto& directives : m_directives) {
    size_t hash = directives.hash();
    hasher.push((const char*)&hash, sizeof(hash));
  }

  return hasher.digest();
}

const List<Directives>& DirectivesGroup::list() const {
  return m_directives;
}

bool operator==(DirectivesGroup const& a, DirectivesGroup const& b) {
  return a.compare(b);
}

bool operator!=(DirectivesGroup const& a, DirectivesGroup const& b) {
  return !a.compare(b);
}

DataStream& operator>>(DataStream& ds, DirectivesGroup& directivesGroup) {
  String string;
  ds.read(string);

  directivesGroup = DirectivesGroup(move(string));

  return ds;
}

DataStream& operator<<(DataStream& ds, DirectivesGroup const& directivesGroup) {
  ds.write(directivesGroup.toString());

  return ds;
}

size_t hash<DirectivesGroup>::operator()(DirectivesGroup const& s) const {
  return s.hash();
}

}
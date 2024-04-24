#include "StarDirectives.hpp"
#include "StarImage.hpp"
#include "StarImageProcessing.hpp"
#include "StarXXHash.hpp"
#include "StarLogging.hpp"

namespace Star {

Directives::Entry::Entry(ImageOperation&& newOperation, size_t strBegin, size_t strLength) {
  operation = std::move(newOperation);
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
    try
      { operation = imageOperationFromString(string(parent)); }
    catch (StarException const& e)
      { operation = ErrorImageOperation{std::exception_ptr()}; }
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

Directives::Shared::Shared() {}

Directives::Shared::Shared(List<Entry>&& givenEntries, String&& givenString) {
  entries = std::move(givenEntries);
  string = std::move(givenString);
  hash = string.empty() ? 0 : XXH3_64bits(string.utf8Ptr(), string.utf8Size());
}

Directives::Directives() {}

Directives::Directives(String const& directives) {
  parse(String(directives));
}

Directives::Directives(String&& directives) {
  parse(std::move(directives));
}

Directives::Directives(const char* directives) {
  parse(directives);
}

Directives::Directives(Directives&& directives) noexcept {
  *this = std::move(directives);
}

Directives::Directives(Directives const& directives) {
  *this = directives;
}

Directives::~Directives() {}

Directives& Directives::operator=(String const& s) {
  if (m_shared && m_shared->string == s)
    return *this;

  parse(String(s));
  return *this;
}

Directives& Directives::operator=(String&& s) {
  if (m_shared && m_shared->string == s) {
    s.clear();
    return *this;
  }

  parse(std::move(s));
  return *this;
}

Directives& Directives::operator=(const char* s) {
  if (m_shared && m_shared->string.utf8().compare(s) == 0)
    return *this;

  parse(s);
  return *this;
}

Directives& Directives::operator=(Directives&& other) noexcept {
  m_shared = std::move(other.m_shared);
  return *this;
}

Directives& Directives::operator=(Directives const& other) {
  m_shared = other.m_shared;
  return *this;
}

void Directives::loadOperations() const {
  if (!m_shared)
    return;

  MutexLocker locker(m_shared->mutex, false);
  if (!m_shared.unique())
    locker.lock();
  for (auto& entry : m_shared->entries)
    entry.loadOperation(*m_shared);
}

void Directives::parse(String&& directives) {
  if (directives.empty()) {
    m_shared.reset();
    return;
  }

  List<Entry> entries;
  StringView view(directives);
  view.forEachSplitView("?", [&](StringView split, size_t beg, size_t end) {
    if (!split.empty()) {
      ImageOperation operation = NullImageOperation();
      if (beg == 0) {
        try
          { operation = imageOperationFromString(split); }
        catch (StarException const& e)
          { operation = ErrorImageOperation{std::exception_ptr()}; }
      }
      entries.emplace_back(std::move(operation), beg, end);
    }
  });

  if (entries.empty()) {
    m_shared.reset();
    return;
  }

  m_shared = std::make_shared<Shared const>(std::move(entries), std::move(directives));
  if (view.utf8().size() < 1000) { // Pre-load short enough directives
    for (auto& entry : m_shared->entries)
      entry.loadOperation(*m_shared);
  }
}

String Directives::string() const {
  if (!m_shared)
    return "";
  else
    return m_shared->string;
}

String const* Directives::stringPtr() const {
  if (!m_shared)
    return nullptr;
  else
    return &m_shared->string;
}


String Directives::buildString() const {
  if (m_shared) {
    String built;
    for (auto& entry : m_shared->entries) {
      if (entry.begin > 0)
        built += "?";
      built += entry.string(*m_shared);
    }

    return built;
  }

  return String();
}

String& Directives::addToString(String& out) const {
  if (!empty())
    out += m_shared->string;
  return out;
}

size_t Directives::hash() const {
  return m_shared ? m_shared->hash : 0;
}

size_t Directives::size() const {
  return m_shared ? m_shared->entries.size() : 0;
}

bool Directives::empty() const {
  return !m_shared || m_shared->empty();
}

Directives::operator bool() const {
  return !empty();
}

bool Directives::equals(Directives const& other) const {
  return hash() == other.hash();
}

bool Directives::equals(String const& string) const {
  auto directiveString = stringPtr();
  return directiveString ? string == *directiveString : string.empty();
}

bool Directives::operator==(Directives const& other) const {
  return equals(other);
}

bool Directives::operator==(String const& string) const {
  return equals(string);
}

DataStream& operator>>(DataStream& ds, Directives& directives) {
  String string;
  ds.read(string);

  directives.parse(std::move(string));

  return ds;
}

DataStream& operator<<(DataStream & ds, Directives const& directives) {
  if (directives)
    ds.write(directives->string);
  else
    ds.write(String());

  return ds;
}

bool operator==(Directives const& d1, Directives const& d2) {
  return d1.equals(d2);
}

bool operator==(String const& string, Directives const& directives) {
  return directives.equals(string);
}

bool operator==(Directives const& directives, String const& string) {
  return directives.equals(string);
}

DirectivesGroup::DirectivesGroup() : m_count(0) {}
DirectivesGroup::DirectivesGroup(String const& directives) : m_count(0) {
  if (directives.empty())
    return;

  Directives parsed(directives);
  if (parsed) {
    m_directives.emplace_back(std::move(parsed));
    m_count = m_directives.back().size();
  }
}
DirectivesGroup::DirectivesGroup(String&& directives) : m_count(0) {
  if (directives.empty()) {
    directives.clear();
    return;
  }

  Directives parsed(std::move(directives));
  if (parsed) {
    m_directives.emplace_back(std::move(parsed));
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
  for (auto& entry : m_directives) {
    if (entry && !entry->string.empty()) {
      if (!string.empty() && string.utf8().back() != '?' && entry->string.utf8()[0] != '?')
        string.append('?');
      string += entry->string;
    }
  }
}

void DirectivesGroup::forEach(DirectivesCallback callback) const {
  for (auto& directives : m_directives) {
    if (directives) {
      for (auto& entry : directives->entries)
        callback(entry, directives);
    }
  }
}

bool DirectivesGroup::forEachAbortable(AbortableDirectivesCallback callback) const {
  for (auto& directives : m_directives) {
    if (directives) {
      for (auto& entry : directives->entries) {
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
    ImageOperation const& operation = entry.loadOperation(*directives);
    if (auto error = operation.ptr<ErrorImageOperation>())
      if (auto string = error->cause.ptr<std::string>())
        throw DirectivesException::format("ImageOperation parse error: {}", *string);
      else
        std::rethrow_exception(error->cause.get<std::exception_ptr>());
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

  directivesGroup = DirectivesGroup(std::move(string));

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
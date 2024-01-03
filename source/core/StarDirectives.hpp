#ifndef STAR_DIRECTIVES_HPP
#define STAR_DIRECTIVES_HPP

#include "StarImageProcessing.hpp"
#include "StarHash.hpp"
#include "StarDataStream.hpp"
#include "StarStringView.hpp"
#include "StarThread.hpp"

namespace Star {

STAR_CLASS(Directives);
STAR_CLASS(DirectivesGroup);
STAR_EXCEPTION(DirectivesException, StarException);

// Kae: My attempt at reducing memory allocation and per-frame string parsing for extremely long directives
class Directives {
public:
  struct Shared;
  struct Entry {
    mutable ImageOperation operation;
    size_t begin;
    size_t length;

    ImageOperation const& loadOperation(Shared const& parent) const;
    StringView string(Shared const& parent) const;
    Entry(ImageOperation&& newOperation, size_t begin, size_t end);
    Entry(ImageOperation const& newOperation, size_t begin, size_t end);
    Entry(Entry const& other);
  };

  struct Shared {
    List<Entry> entries;
    String string;
    StringView prefix;
    size_t hash = 0;
    mutable Mutex mutex;

    bool empty() const;
    Shared(List<Entry>&& givenEntries, String&& givenString, StringView givenPrefix);
  };

  Directives();
  Directives(String const& directives);
  Directives(String&& directives);
  Directives(const char* directives);
  Directives(Directives const& directives);
  Directives(Directives&& directives);
  ~Directives();

  Directives& operator=(String const& s);
  Directives& operator=(String&& s);
  Directives& operator=(const char* s);
  Directives& operator=(Directives&& other);
  Directives& operator=(Directives const& other);

  void loadOperations() const;
  void parse(String&& directives);
  String string() const;
  StringView prefix() const;
  String const* stringPtr() const;
  String buildString() const;
  String& addToString(String& out) const;
  size_t hash() const;
  size_t size() const;
  bool empty() const;
  operator bool() const;

  friend DataStream& operator>>(DataStream& ds, Directives& directives);
  friend DataStream& operator<<(DataStream& ds, Directives const& directives);

  std::shared_ptr<Shared const> shared;
};

class DirectivesGroup {
public:
  DirectivesGroup();
  DirectivesGroup(String const& directives);
  DirectivesGroup(String&& directives);

  bool empty() const;
  operator bool() const;
  bool compare(DirectivesGroup const& other) const;
  void append(Directives const& other);
  void clear();

  DirectivesGroup& operator+=(Directives const& other);

  String toString() const;
  void addToString(String& string) const;

  typedef function<void(Directives::Entry const&, Directives const&)> DirectivesCallback;
  typedef function<bool(Directives::Entry const&, Directives const&)> AbortableDirectivesCallback;

  void forEach(DirectivesCallback callback) const;
  bool forEachAbortable(AbortableDirectivesCallback callback) const;

  Image applyNewImage(const Image& image) const;
  void applyExistingImage(Image& image) const;
  
  size_t hash() const;
  const List<Directives>& list() const;

  friend bool operator==(DirectivesGroup const& a, DirectivesGroup const& b);
  friend bool operator!=(DirectivesGroup const& a, DirectivesGroup const& b);

  friend DataStream& operator>>(DataStream& ds, DirectivesGroup& directives);
  friend DataStream& operator<<(DataStream& ds, DirectivesGroup const& directives);
private:
  void buildString(String& string, const DirectivesGroup& directives) const;

  List<Directives> m_directives;
  size_t m_count;
};


template <>
struct hash<DirectivesGroup> {
  size_t operator()(DirectivesGroup const& s) const;
};

typedef DirectivesGroup ImageDirectives;

}

#endif

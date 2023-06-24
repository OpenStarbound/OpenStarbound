#ifndef STAR_DIRECTIVES_HPP
#define STAR_DIRECTIVES_HPP

#include "StarImageProcessing.hpp"
#include "StarHash.hpp"
#include "StarDataStream.hpp"

namespace Star {

STAR_CLASS(Directives);
STAR_CLASS(DirectivesGroup);
STAR_EXCEPTION(DirectivesException, StarException);

// Kae: My attempt at reducing memory allocation and per-frame string parsing for extremely long directives
class Directives {
public:
  struct Entry {
    ImageOperation operation;
    String string; // One day, we can make this a string_view pointing to Entry::string.

    Entry(ImageOperation&& newOperation, String&& newString);
    Entry(ImageOperation const& newOperation, String const& newString);
    Entry(Entry const& other);
  };

  Directives();
  Directives(String const& directives);
  Directives(String&& directives);
  Directives(const char* directives);
  Directives(List<Entry>&& entries);

  void parse(String const& directives);
  String& buildString(String& out) const;
  String toString() const;
  inline bool empty() const;
  inline operator bool() const;

  friend DataStream& operator>>(DataStream& ds, Directives& directives);
  friend DataStream& operator<<(DataStream& ds, Directives const& directives);

  std::shared_ptr<List<Entry> const> entries;
  size_t hash = 0;
  String string;
};

class DirectivesGroup {
public:
  DirectivesGroup();
  DirectivesGroup(String const& directives);
  DirectivesGroup(String&& directives);

  void parseDirectivesIntoLeaf(String const& directives);

  inline bool empty() const;
  inline operator bool() const;
  bool compare(DirectivesGroup const& other) const;
  void append(Directives const& other);
  void append(List<Directives::Entry>&& entries);
  void clear();

  DirectivesGroup& operator+=(Directives const& other);

  inline String toString() const;
  void addToString(String& string) const;

  typedef function<void(Directives::Entry const&)> DirectivesCallback;
  typedef function<bool(Directives::Entry const&)> AbortableDirectivesCallback;

  void forEach(DirectivesCallback callback) const;
  bool forEachAbortable(AbortableDirectivesCallback callback) const;

  inline Image applyNewImage(const Image& image) const;
  void applyExistingImage(Image& image) const;
  
  inline size_t hash() const;

  friend bool operator==(DirectivesGroup const& a, DirectivesGroup const& b);
  friend bool operator!=(DirectivesGroup const& a, DirectivesGroup const& b);
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

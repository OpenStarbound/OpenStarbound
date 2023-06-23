#ifndef STAR_DIRECTIVES_HPP
#define STAR_DIRECTIVES_HPP

#include "StarImageProcessing.hpp"

namespace Star {

STAR_CLASS(NestedDirectives);
STAR_EXCEPTION(DirectivesException, StarException);

// Kae: My attempt at reducing memory allocation and per-frame string parsing for extremely long directives
class NestedDirectives {
public:
  struct Leaf {
    List<ImageOperation> operations;
    List<String> strings;

    size_t length() const;
  };

  typedef function<void(Leaf const&)> LeafCallback;
  typedef function<void(ImageOperation const&, String const&)> LeafPairCallback;
  typedef function<bool(Leaf const&)> AbortableLeafCallback;
  typedef function<bool(ImageOperation const&, String const&)> AbortableLeafPairCallback;

  struct Cell;
  typedef std::shared_ptr<Cell> Branch;
  typedef std::shared_ptr<Cell const> ConstBranch;
  typedef List<ConstBranch> Branches;


  struct Cell {
    Variant<Leaf, Branches> value;

    Cell();
    Cell(Leaf&& leaf);
    Cell(Branches&& branches);
    Cell(const Leaf& leaf);
    Cell(const Branches& branches);

    void buildString(String& string) const;
    void forEach(LeafCallback& callback) const;
    bool forEachAbortable(AbortableLeafCallback& callback) const;
  };


  NestedDirectives();
  NestedDirectives(String const& directives);
  NestedDirectives(String&& directives);

  void parseDirectivesIntoLeaf(String const& directives);

  bool empty() const;
  void append(const NestedDirectives& other);

  const ConstBranch& branch() const;

  String toString() const;
  void addToString(String& string) const;

  void forEach(LeafCallback callback) const;
  void forEachPair(LeafPairCallback callback) const;
  bool forEachAbortable(AbortableLeafCallback callback) const;
  bool forEachPairAbortable(AbortableLeafPairCallback callback) const;

  Image apply(Image& image) const;
private:
  void buildString(String& string, const Cell& cell) const;
  Branches& convertToBranches();

  Branch m_root;
};

typedef NestedDirectives ImageDirectives;

}

#endif

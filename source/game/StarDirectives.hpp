#ifndef STAR_DIRECTIVES_HPP
#define STAR_DIRECTIVES_HPP

#include "StarImageProcessing.hpp"

namespace Star {

STAR_CLASS(NestedDirectives);

// Attempt at reducing memory allocation and per-frame string parsing for extremely long directives
class NestedDirectives {
public:
  struct Leaf {
    List<ImageOperation> operations;
    String string;
  };

  struct Cell;
  typedef std::shared_ptr<Cell const> Branch;
  typedef List<Branch> Branches;

  struct Cell {
    Variant<Leaf, Branches> value;

    Cell() : value(Leaf()) {};
    Cell(Leaf&& leaf) : value(move(leaf)) {};
    Cell(Branches&& branches) : value(move(branches)) {};
    Cell(const Leaf& leaf) : value(leaf) {};
    Cell(const Branches& branches) : value(branches) {};
  };


  NestedDirectives();
  NestedDirectives(String const& string);

  void addBranch(const Branch& newBranch);
  const Branch& branch() const;
  String toString() const;
  void forEach() const;
  Image apply(Image& image) const;
private:
  void buildString(String& string, const Cell& cell) const;
  void convertToBranches();

  Cell m_root;
};

}

#endif

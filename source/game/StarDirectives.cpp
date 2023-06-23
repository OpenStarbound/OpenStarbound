#include "StarImage.hpp"
#include "StarImageProcessing.hpp"
#include "StarDirectives.hpp"

namespace Star {

NestedDirectives::NestedDirectives() {}
NestedDirectives::NestedDirectives(String const& string) : m_root{ Leaf{ parseImageOperations(string), string} } {}

void NestedDirectives::addBranch(const Branch& newBranch) {
  convertToBranches();

  m_root.value.get<Branches>().emplace_back(newBranch);
}

String NestedDirectives::toString() const {
  String string;
  buildString(string, m_root);
  return string;
}

void NestedDirectives::forEach() const {

}

Image NestedDirectives::apply(Image& image) const {
  Image current = image;

  return current;
}

void NestedDirectives::buildString(String& string, const Cell& cell) const {
  if (auto leaf = cell.value.ptr<Leaf>())
    string += leaf->string;
  else {
    for (auto& branch : cell.value.get<Branches>())
      buildString(string, *branch);
  }
}

void NestedDirectives::convertToBranches() {
  if (m_root.value.is<Branches>())
    return;

  Leaf& leaf = m_root.value.get<Leaf>();
  Branches newBranches;
  newBranches.emplace_back(std::make_shared<Cell const>(move(leaf)));
  m_root.value = move(newBranches);
}

}
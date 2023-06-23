#include "StarImage.hpp"
#include "StarImageProcessing.hpp"
#include "StarDirectives.hpp"

namespace Star {

NestedDirectives::NestedDirectives() : m_root(nullptr) {}
NestedDirectives::NestedDirectives(String const& directives) {
  parseDirectivesIntoLeaf(directives);
}
NestedDirectives::NestedDirectives(String&& directives) {
  String mine = move(directives); // most useless move constructor in the world
  parseDirectivesIntoLeaf(mine);
}

void NestedDirectives::parseDirectivesIntoLeaf(String const& directives) {
  Leaf leaf;
  for (String& op : directives.split('?')) {
    if (!op.empty()) {
      leaf.operations.append(imageOperationFromString(op));
      leaf.strings.append(move(op));
    }
  }
  m_root = std::make_shared<Cell>(move(leaf));
}

bool NestedDirectives::empty() const {
  return (bool)m_root;
}

void NestedDirectives::append(const NestedDirectives& other) {
  convertToBranches().emplace_back(other.branch());
}

String NestedDirectives::toString() const {
  String string;
  addToString(string);
  return string;
}

void NestedDirectives::addToString(String& string) const {
  if (m_root)
    m_root->buildString(string);
}

void NestedDirectives::forEach(LeafCallback callback) const {
  if (m_root)
    m_root->forEach(callback);
}

void NestedDirectives::forEachPair(LeafPairCallback callback) const {
  if (m_root) {
    LeafCallback pairCallback = [&](Leaf const& leaf) {
      size_t length = leaf.length();
      for (size_t i = 0; i != length; ++i)
        callback(leaf.operations.at(i), leaf.strings.at(i));
    };
    m_root->forEach(pairCallback);
  }
}

bool NestedDirectives::forEachAbortable(AbortableLeafCallback callback) const {
  if (!m_root)
    return false;
  else
    return m_root->forEachAbortable(callback);
}

bool NestedDirectives::forEachPairAbortable(AbortableLeafPairCallback callback) const {
  if (!m_root)
    return false;
  else {
    AbortableLeafCallback pairCallback = [&](Leaf const& leaf) -> bool {
      size_t length = leaf.length();
      for (size_t i = 0; i != length; ++i) {
        if (!callback(leaf.operations.at(i), leaf.strings.at(i)))
          return false;
      }

      return true;
    };
    return m_root->forEachAbortable(pairCallback);
  }
}

Image NestedDirectives::apply(Image& image) const {
  Image current = image;
  forEach([&](Leaf const& leaf) {
    current = processImageOperations(leaf.operations, current);
  });
  return current;
}

NestedDirectives::Branches& NestedDirectives::convertToBranches() {
  if (!m_root) {
    m_root = std::make_shared<Cell>(Branches());
  }
  else if (m_root->value.is<Branches>())
    return;

  Leaf& leaf = m_root->value.get<Leaf>();
  Branches newBranches;
  newBranches.emplace_back(std::make_shared<Cell const>(move(leaf)));
  m_root->value = move(newBranches);
  return m_root->value.get<Branches>();
}

size_t NestedDirectives::Leaf::length() const {
  if (operations.size() != strings.size())
    throw DirectivesException("NestedDirectives leaf has mismatching operation/string List sizes");

  return operations.size();
}

NestedDirectives::Cell::Cell() : value(Leaf()) {};
NestedDirectives::Cell::Cell(Leaf&& leaf) : value(move(leaf)) {};
NestedDirectives::Cell::Cell(Branches&& branches) : value(move(branches)) {};
NestedDirectives::Cell::Cell(const Leaf& leaf) : value(leaf) {};
NestedDirectives::Cell::Cell(const Branches& branches) : value(branches) {};

void NestedDirectives::Cell::buildString(String& string) const {
  if (auto leaf = value.ptr<Leaf>())
    for (auto& leafString : leaf->strings) {
      string += "?";
      string += leafString;
    }
  else {
    for (auto& branch : value.get<Branches>())
      branch->buildString(string);
  }
}

void NestedDirectives::Cell::forEach(LeafCallback& callback) const {
  if (auto leaf = value.ptr<Leaf>())
    callback(*leaf);
  else {
    for (auto& branch : value.get<Branches>())
      branch->forEach(callback);
  }
}

bool NestedDirectives::Cell::forEachAbortable(AbortableLeafCallback& callback) const {
  if (auto leaf = value.ptr<Leaf>()) {
    if (!callback(*leaf))
      return false;
  } else {
    for (auto& branch : value.get<Branches>())
      if (!branch->forEachAbortable(callback))
        return false;
  }
  return true;
}

}
#include "StarImage.hpp"
#include "StarImageProcessing.hpp"
#include "StarDirectives.hpp"
#include "StarXXHash.hpp"
#include "StarHash.hpp"

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
    if (!op.empty())
      leaf.entries.emplace_back(imageOperationFromString(op), op);
  }
  m_root = std::make_shared<Cell>(move(leaf));
}

inline bool NestedDirectives::empty() const {
  return (bool)m_root;
}

inline bool NestedDirectives::compare(NestedDirectives const& other) const {
  if (m_root == other.m_root)
    return true;

  return false;
}

void NestedDirectives::append(NestedDirectives const& other) {
  convertToBranches().emplace_back(other.branch());
}

NestedDirectives& NestedDirectives::operator+=(NestedDirectives const& other) {
  append(other);
  return *this;
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
      for (auto& entry : leaf.entries)
        callback(entry.operation, entry.string);
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
      for (auto& entry : leaf.entries) {
        if (!callback(entry.operation, entry.string))
          return false;
      }

      return true;
    };
    return m_root->forEachAbortable(pairCallback);
  }
}

Image NestedDirectives::apply(Image& image) const {
  Image current = image;
  forEachPair([&](ImageOperation const& operation, String const& string) {
    processImageOperation(operation, current);
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

bool NestedDirectives::Leaf::Entry::operator==(NestedDirectives::Leaf::Entry const& other) const {
  return string == other.string;
}

bool NestedDirectives::Leaf::Entry::operator!=(NestedDirectives::Leaf::Entry const& other) const {
  return string != other.string;
}


size_t NestedDirectives::Leaf::length() const {
  return entries.size();
}

bool NestedDirectives::Leaf::operator==(NestedDirectives::Leaf const& other) const {
  size_t len = length();
  if (len != other.length())
    return false;

  for (size_t i = 0; i != len; ++i) {
    if (entries[i] != other.entries[i])
      return false;
  }
  return true;
}

bool NestedDirectives::Leaf::operator!=(NestedDirectives::Leaf const& other) const {
  return !(*this == other);
}

NestedDirectives::Cell::Cell() : value(Leaf()) {};
NestedDirectives::Cell::Cell(Leaf&& leaf) : value(move(leaf)) {};
NestedDirectives::Cell::Cell(Branches&& branches) : value(move(branches)) {};
NestedDirectives::Cell::Cell(const Leaf& leaf) : value(leaf) {};
NestedDirectives::Cell::Cell(const Branches& branches) : value(branches) {};

/*
bool NestedDirectives::Cell::operator==(NestedDirectives::Cell const& other) const {
  if (auto leaf = value.ptr<Leaf>()) {
    if (auto otherLeaf = other.value.ptr<Leaf>())
      return *leaf == *otherLeaf;
    else {

    }
  }
  else {
    for (auto& branch : value.get<Branches>()) {

    }
  }
}


bool NestedDirectives::Cell::operator!=(NestedDirectives::Cell const& other) const {
  return !(*this == other);
}
//*/

void NestedDirectives::Cell::buildString(String& string) const {
  if (auto leaf = value.ptr<Leaf>())
    for (auto& entry : leaf->entries) {
      string += "?";
      string += entry.string;
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
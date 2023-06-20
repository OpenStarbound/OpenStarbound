#include "StarBTree.hpp"
#include "StarString.hpp"
#include "StarMap.hpp"
#include "StarSet.hpp"
#include "StarLexicalCast.hpp"

#include "gtest/gtest.h"

using namespace Star;
using namespace std;

template <typename Key, typename Pointer>
struct SimpleBTreeIndex {
  size_t pointerCount() const;
  Pointer pointer(size_t i) const;
  void updatePointer(size_t i, Pointer p);

  Key const& keyBefore(size_t i) const;
  void updateKeyBefore(size_t i, Key k);

  void removeBefore(size_t i);
  void insertAfter(size_t i, Key k, Pointer p);

  size_t indexLevel() const;
  void setIndexLevel(size_t indexLevel);

  // count is number of elements to shift left *including* right's beginPointer
  void shiftLeft(Key const& mid, SimpleBTreeIndex& right, size_t count);

  // count is number of elements to shift right
  void shiftRight(Key const& mid, SimpleBTreeIndex& left, size_t count);

  // i should be index of pointer that will be the new beginPointer of right
  // node (cannot be 0).
  Key split(SimpleBTreeIndex& right, size_t i);

  struct Element {
    Key key;
    Pointer pointer;
  };
  typedef List<Element> ElementList;

  Pointer self;
  size_t level;
  Maybe<Pointer> beginPointer;
  ElementList pointers;
};

template <typename Key, typename Data, typename Pointer>
struct SimpleBTreeLeaf {
  size_t count() const;
  Key const& key(size_t i) const;
  Data const& data(size_t i) const;

  void insert(size_t i, Key k, Data d);
  void remove(size_t i);

  Maybe<Pointer> nextLeaf() const;
  void setNextLeaf(Maybe<Pointer> n);

  // count is number of elements to shift left
  void shiftLeft(SimpleBTreeLeaf& right, size_t count);

  // count is number of elements to shift right
  void shiftRight(SimpleBTreeLeaf& left, size_t count);

  // i should be index of element that will be the new start of right node.
  // Returns right index node.
  void split(SimpleBTreeLeaf& right, size_t i);

  struct Element {
    Key key;
    Data data;
  };
  typedef List<Element> ElementList;

  Maybe<Pointer> next;
  Pointer self;
  ElementList elements;
};

template <typename Key, typename Pointer>
size_t SimpleBTreeIndex<Key, Pointer>::pointerCount() const {
  // If no begin pointer is set then the index is simply uninitialized.
  if (!beginPointer)
    return 0;
  else
    return pointers.size() + 1;
}

template <typename Key, typename Pointer>
Pointer SimpleBTreeIndex<Key, Pointer>::pointer(size_t i) const {
  if (i == 0)
    return *beginPointer;
  else
    return pointers.at(i - 1).pointer;
}

template <typename Key, typename Pointer>
void SimpleBTreeIndex<Key, Pointer>::updatePointer(size_t i, Pointer p) {
  if (i == 0)
    *beginPointer = p;
  else
    pointers.at(i - 1).pointer = p;
}

template <typename Key, typename Pointer>
Key const& SimpleBTreeIndex<Key, Pointer>::keyBefore(size_t i) const {
  return pointers.at(i - 1).key;
}

template <typename Key, typename Pointer>
void SimpleBTreeIndex<Key, Pointer>::updateKeyBefore(size_t i, Key k) {
  pointers.at(i - 1).key = k;
}

template <typename Key, typename Pointer>
void SimpleBTreeIndex<Key, Pointer>::removeBefore(size_t i) {
  if (i == 0) {
    beginPointer = pointers.at(0).pointer;
    pointers.eraseAt(0);
  } else {
    pointers.eraseAt(i - 1);
  }
}

template <typename Key, typename Pointer>
void SimpleBTreeIndex<Key, Pointer>::insertAfter(size_t i, Key k, Pointer p) {
  pointers.insertAt(i, Element{k, p});
}

template <typename Key, typename Pointer>
size_t SimpleBTreeIndex<Key, Pointer>::indexLevel() const {
  return level;
}

template <typename Key, typename Pointer>
void SimpleBTreeIndex<Key, Pointer>::setIndexLevel(size_t indexLevel) {
  level = indexLevel;
}

template <typename Key, typename Pointer>
void SimpleBTreeIndex<Key, Pointer>::shiftLeft(Key const& mid, SimpleBTreeIndex& right, size_t count) {
  count = std::min(right.pointerCount(), count);

  if (count == 0)
    return;

  pointers.append(Element{mid, *right.beginPointer});

  typename ElementList::iterator s = right.pointers.begin();
  std::advance(s, count - 1);
  pointers.insert(pointers.end(), right.pointers.begin(), s);

  right.pointers.erase(right.pointers.begin(), s);
  if (right.pointers.size() != 0) {
    right.beginPointer = right.pointers.at(0).pointer;
    right.pointers.eraseAt(0);
  } else {
    right.beginPointer.reset();
  }
}

template <typename Key, typename Pointer>
void SimpleBTreeIndex<Key, Pointer>::shiftRight(Key const& mid, SimpleBTreeIndex& left, size_t count) {
  count = std::min(left.pointerCount(), count);

  if (count == 0)
    return;
  --count;

  pointers.insert(pointers.begin(), Element{mid, *beginPointer});

  typename ElementList::iterator s = left.pointers.begin();
  std::advance(s, left.pointers.size() - count);
  pointers.insert(pointers.begin(), s, left.pointers.end());

  left.pointers.erase(s, left.pointers.end());
  if (left.pointers.size() != 0) {
    beginPointer = left.pointers.at(left.pointers.size() - 1).pointer;
    left.pointers.eraseAt(left.pointers.size() - 1);
  } else {
    beginPointer = left.beginPointer.take();
  }
}

template <typename Key, typename Pointer>
Key SimpleBTreeIndex<Key, Pointer>::split(SimpleBTreeIndex& right, size_t i) {
  typename ElementList::iterator s = pointers.begin();
  std::advance(s, i - 1);

  right.beginPointer = s->pointer;
  Key midKey = s->key;
  right.level = level;
  ++s;

  right.pointers.insert(right.pointers.begin(), s, pointers.end());
  --s;

  pointers.erase(s, pointers.end());

  return midKey;
}

template <typename Key, typename Data, typename Pointer>
size_t SimpleBTreeLeaf<Key, Data, Pointer>::count() const {
  return elements.size();
}

template <typename Key, typename Data, typename Pointer>
Key const& SimpleBTreeLeaf<Key, Data, Pointer>::key(size_t i) const {
  return elements.at(i).key;
}

template <typename Key, typename Data, typename Pointer>
Data const& SimpleBTreeLeaf<Key, Data, Pointer>::data(size_t i) const {
  return elements.at(i).data;
}

template <typename Key, typename Data, typename Pointer>
void SimpleBTreeLeaf<Key, Data, Pointer>::insert(size_t i, Key k, Data d) {
  elements.insertAt(i, Element{move(k), move(d)});
}

template <typename Key, typename Data, typename Pointer>
void SimpleBTreeLeaf<Key, Data, Pointer>::remove(size_t i) {
  elements.eraseAt(i);
}

template <typename Key, typename Data, typename Pointer>
void SimpleBTreeLeaf<Key, Data, Pointer>::shiftLeft(SimpleBTreeLeaf& right, size_t count) {
  count = std::min(right.count(), count);

  if (count == 0)
    return;

  typename ElementList::iterator s = right.elements.begin();
  std::advance(s, count);

  elements.insert(elements.end(), right.elements.begin(), s);
  right.elements.erase(right.elements.begin(), s);
}

template <typename Key, typename Data, typename Pointer>
void SimpleBTreeLeaf<Key, Data, Pointer>::shiftRight(SimpleBTreeLeaf& left, size_t count) {
  count = std::min(left.count(), count);

  if (count == 0)
    return;

  typename ElementList::iterator s = left.elements.begin();
  std::advance(s, left.elements.size() - count);

  elements.insert(elements.begin(), s, left.elements.end());
  left.elements.erase(s, left.elements.end());
}

template <typename Key, typename Data, typename Pointer>
void SimpleBTreeLeaf<Key, Data, Pointer>::split(SimpleBTreeLeaf& right, size_t i) {
  typename ElementList::iterator s = elements.begin();
  std::advance(s, i);

  right.elements.insert(right.elements.begin(), s, elements.end());
  elements.erase(s, elements.end());
}

template <typename Key, typename Data, typename Pointer>
Maybe<Pointer> SimpleBTreeLeaf<Key, Data, Pointer>::nextLeaf() const {
  return next;
}

template <typename Key, typename Data, typename Pointer>
void SimpleBTreeLeaf<Key, Data, Pointer>::setNextLeaf(Maybe<Pointer> n) {
  next = move(n);
}

// Testing BTree class that simulates storage by storing in-memory copies of
// nodes.  Used to test BTree algorithm.

struct SimpleBTreeBase {
  typedef int Key;
  typedef String Data;
  typedef int Pointer;

  typedef SimpleBTreeIndex<int, int> Index;
  typedef SimpleBTreeLeaf<int, String, int> Leaf;

  Pointer rootPointer() {
    return root;
  }

  bool rootIsLeaf() {
    return rootleaf;
  }

  void setNewRoot(Pointer pointer, bool isLeaf) {
    root = pointer;
    rootleaf = isLeaf;

    for (int i : deletedLeaves)
      leaves.remove(i);

    for (int i : deletedIndexes)
      indexes.remove(i);

    deletedLeaves.clear();
    deletedIndexes.clear();
  }

  // Should create new empty leaf.
  Leaf createLeaf() {
    Leaf leaf;
    leaf.self = -1;
    return leaf;
  }

  Leaf loadLeaf(Pointer const& pointer) {
    // To make sure to accurately test storage, always *copy* in and out
    return leaves.get(pointer);
  }

  bool leafNeedsShift(Leaf const& leaf) {
    return leaf.count() < (maxLeafSize + 1) / 2;
  }

  bool shouldAppendNewLeaf(Leaf const& leaf) {
    return maxLeafSize == 2 && leaf.count() == 2;
  }

  bool leafShift(Leaf& left, Leaf& right) {
    if (left.count() + right.count() <= maxLeafSize) {
      left.shiftLeft(right, right.count());
      return true;
    } else {
      if (leafNeedsShift(right)) {
        right.shiftRight(left, 1);
        return true;
      } else if (leafNeedsShift(left)) {
        left.shiftLeft(right, 1);
        return true;
      } else {
        return false;
      }
    }
  }

  Maybe<Leaf> leafSplit(Leaf& leaf) {
    if (leaf.count() <= maxLeafSize) {
      return {};
    } else {
      Leaf right;
      right.self = -1;

      leaf.split(right, (leaf.count() + 1) / 2);

      return right;
    }
  }

  Pointer storeLeaf(Leaf leaf) {
    if (leaf.self != -1)
      deleteLeaf(leaf);

    while (leaves.contains(leafId))
      ++leafId;
    leaf.self = leafId;

    // To make sure to accurately test storage, always *copy* in and out
    leaves[leafId] = leaf;

    return leaf.self;
  }

  void deleteLeaf(Leaf const& leaf) {
    deletedLeaves.append(leaf.self);
  }

  // Should create new index with two pointers and one mid key.
  Index createIndex(Pointer beginPointer) {
    Index indexNode;
    indexNode.self = -1;
    indexNode.level = 0;
    indexNode.beginPointer = beginPointer;
    return indexNode;
  }

  Index loadIndex(Pointer const& pointer) {
    return indexes.get(pointer);
  }

  bool indexNeedsShift(Index const& index) {
    return index.pointerCount() < (maxIndexSize + 1) / 2;
  }

  bool indexShift(Index& left, Key const& mid, Index& right) {
    if (left.pointerCount() + right.pointerCount() <= maxIndexSize) {
      left.shiftLeft(mid, right, right.pointerCount());
      return true;
    } else {
      if (indexNeedsShift(right)) {
        right.shiftRight(mid, left, 1);
        return true;
      } else if (indexNeedsShift(left)) {
        left.shiftLeft(mid, right, 1);
        return true;
      } else {
        return false;
      }
    }
  }

  Maybe<pair<Key, Index>> indexSplit(Index& index) {
    if (index.pointerCount() <= maxIndexSize) {
      return {};
    } else {
      Index right;
      right.self = -1;

      Key mid = index.split(right, (index.pointerCount() + 1) / 2);

      return make_pair(mid, right);
    }
  }

  Pointer storeIndex(Index index) {
    if (index.self != -1)
      deleteIndex(index);

    while (indexes.contains(indexId))
      ++indexId;
    index.self = indexId;

    indexes[indexId] = index;

    return index.self;
  }

  void deleteIndex(Index const& index) {
    deletedIndexes.append(index.self);
  }

  size_t indexPointerCount(Index const& index) {
    return index.pointerCount();
  }

  Pointer indexPointer(Index const& index, size_t i) {
    return index.pointer(i);
  }

  void indexUpdatePointer(Index& index, size_t i, Pointer p) {
    index.updatePointer(i, p);
  }

  Key indexKeyBefore(Index const& index, size_t i) {
    return index.keyBefore(i);
  }

  void indexUpdateKeyBefore(Index& index, size_t i, Key k) {
    index.updateKeyBefore(i, k);
  }

  void indexRemoveBefore(Index& index, size_t i) {
    index.removeBefore(i);
  }

  void indexInsertAfter(Index& index, size_t i, Key k, Pointer p) {
    index.insertAfter(i, k, p);
  }

  size_t indexLevel(Index const& index) {
    return index.indexLevel();
  }

  void setIndexLevel(Index& index, size_t indexLevel) {
    index.setIndexLevel(indexLevel);
  }

  size_t leafElementCount(Leaf const& leaf) {
    return leaf.count();
  }

  Key leafKey(Leaf const& leaf, size_t i) {
    return leaf.key(i);
  }

  Data leafData(Leaf const& leaf, size_t i) {
    return leaf.data(i);
  }

  void leafInsert(Leaf& leaf, size_t i, Key k, Data d) {
    return leaf.insert(i, k, d);
  }

  void leafRemove(Leaf& leaf, size_t i) {
    return leaf.remove(i);
  }

  Maybe<Pointer> nextLeaf(Leaf const& leaf) {
    return leaf.nextLeaf();
  }

  void setNextLeaf(Leaf& leaf, Maybe<Pointer> n) {
    leaf.setNextLeaf(n);
  }

  int root;
  bool rootleaf;

  size_t maxIndexSize;
  size_t maxLeafSize;

  int indexId;
  int leafId;

  Map<int, Index> indexes;
  Map<int, Leaf> leaves;

  List<int> deletedLeaves;
  List<int> deletedIndexes;
};

struct SimpleBTree : public BTreeMixin<SimpleBTreeBase> {
  SimpleBTree(size_t maxisize, size_t maxlsize) {
    maxIndexSize = maxisize;
    maxLeafSize = maxlsize;

    leafId = 0;
    indexId = 0;

    createNewRoot();
  }

  void print() {
    forAllNodes(Printer());
    cout << endl;
  }

  struct Printer {
    bool operator()(Index const& index) {
      cout << "[" << index.level << ":" << index.self << "]"
           << " " << index.beginPointer << " ";
      for (Index::Element e : index.pointers) {
        cout << "(" << e.key << ")"
             << " " << e.pointer << " ";
      }
      cout << endl;
      return true;
    }

    bool operator()(Leaf const& leaf) {
      cout << "[" << leaf.self << "]"
           << " ";
      for (Leaf::Element e : leaf.elements) {
        cout << "(" << e.key << ")"
             << " " << e.data << " ";
      }
      cout << endl;
      return true;
    }
  };
};

const int RandFactor = 0xd5a2f037;
const size_t TestCount = 500;
const size_t WriteRepeat = 3;
const size_t ShrinkCount = 5;

String genValue(int k) {
  return toString(k * RandFactor);
}

bool checkValue(int k, String v) {
  return genValue(k) == v;
}

void putAll(SimpleBTree& db, List<int> keys) {
  for (int k : keys)
    db.insert(k, genValue(k));
}

void checkAll(SimpleBTree& db, List<int> keys) {
  for (int k : keys) {
    auto v = db.find(k);
    EXPECT_TRUE(checkValue(k, *v));
  }
}

size_t removeAll(SimpleBTree& db, List<int> keys) {
  size_t totalRemoved = 0;
  Set<int> removed;
  for (int k : keys) {
    if (db.remove(k)) {
      EXPECT_FALSE(removed.contains(k));
      removed.add(k);
      ++totalRemoved;
    }
  }
  return totalRemoved;
}

void testBTree(size_t maxIndexSize, size_t maxLeafSize) {
  srand(time(0));

  SimpleBTree db(maxIndexSize, maxLeafSize);

  Set<int> keySet;
  while (keySet.size() < TestCount)
    keySet.add(rand());

  List<int> keys;
  for (int k : keySet) {
    for (size_t j = 0; j < WriteRepeat; ++j)
      keys.append(k);
  }

  // record writes/reads repeated WriteRepeat times randomly each cycle
  std::random_shuffle(keys.begin(), keys.end());
  putAll(db, keys);

  EXPECT_EQ(db.recordCount(), TestCount);

  std::random_shuffle(keys.begin(), keys.end());
  checkAll(db, keys);

  // Random reads/writes with ShrinkCount cycles...
  for (size_t i = 0; i < ShrinkCount; ++i) {
    std::random_shuffle(keys.begin(), keys.end());
    List<int> keysTemp = keys.slice(0, keys.size() / 2);

    removeAll(db, keysTemp);

    std::random_shuffle(keysTemp.begin(), keysTemp.end());
    putAll(db, keysTemp);

    std::random_shuffle(keysTemp.begin(), keysTemp.end());
    checkAll(db, keys);
  }

  size_t totalRemoved = removeAll(db, keys);
  EXPECT_EQ(totalRemoved, TestCount);
}

TEST(BTreeTest, All) {
  testBTree(3, 2);
  testBTree(6, 6);
}

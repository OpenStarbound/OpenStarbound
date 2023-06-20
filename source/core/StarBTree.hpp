#ifndef STAR_B_TREE_HPP
#define STAR_B_TREE_HPP

#include "StarList.hpp"
#include "StarMaybe.hpp"

namespace Star {

// Mixin class for implementing a simple B+ Tree style database.  LOTS of
// possibilities for improvement, especially in batch deletes / inserts.
//
// The Base class itself must have the following interface:
//
// struct Base {
//   typedef KeyT Key;
//   typedef DataT Data;
//   typedef PointerT Pointer;
//
//   // Index and Leaf types may either be a literal struct, or a pointer, or a
//   // handle or whatever.  They are meant to be opaque.
//   typedef IndexT Index;
//   typedef LeafT Leaf;
//
//   Pointer rootPointer();
//   bool rootIsLeaf();
//   void setNewRoot(Pointer pointer, bool isLeaf);
//
//   Index createIndex(Pointer beginPointer);
//
//   // Load an existing index.
//   Index loadIndex(Pointer pointer);
//
//   size_t indexPointerCount(Index const& index);
//   Pointer indexPointer(Index const& index, size_t i);
//   void indexUpdatePointer(Index& index, size_t i, Pointer p);
//
//   Key indexKeyBefore(Index const& index, size_t i);
//   void indexUpdateKeyBefore(Index& index, size_t i, Key k);
//
//   void indexRemoveBefore(Index& index, size_t i);
//   void indexInsertAfter(Index& index, size_t i, Key k, Pointer p);
//
//   size_t indexLevel(Index const& index);
//   void setIndexLevel(Index& index, size_t indexLevel);
//
//   // Should return true if index should try to shift elements into this index
//   // from sibling index.
//   bool indexNeedsShift(Index const& index);
//
//   // Should return false if no shift done.  If merging, always merge to the
//   // left.
//   bool indexShift(Index& left, Key const& mid, Index& right);
//
//   // If a split has occurred, split right and return the mid-key and new
//   // right node.
//   Maybe<pair<Key, Index>> indexSplit(Index& index);
//
//   // Index updated, needs storing.  Return pointer to stored index (may
//   // change).  Index will not be used after store.
//   Pointer storeIndex(Index index);
//
//   // Index no longer part of BTree.  Index will not be used after delete.
//   void deleteIndex(Index index);
//
//   // Should create new empty leaf.
//   Leaf createLeaf();
//
//   Leaf loadLeaf(Pointer pointer);
//
//   size_t leafElementCount(Leaf const& leaf);
//   Key leafKey(Leaf const& leaf, size_t i);
//   Data leafData(Leaf const& leaf, size_t i);
//
//   void leafInsert(Leaf& leaf, size_t i, Key k, Data d);
//   void leafRemove(Leaf& leaf, size_t i);
//
//   // Set and get next-leaf pointers.  It is not required that next-leaf
//   // pointers be kept or that they be valid, so nextLeaf may return nothing.
//   void setNextLeaf(Leaf& leaf, Maybe<Pointer> n);
//   Maybe<Pointer> nextLeaf(Leaf const& leaf);
//
//   // Should return true if leaf should try to shift elements into this leaf
//   // from sibling leaf.
//   bool leafNeedsShift(Leaf const& l);
//
//   // Should return false if no change necessary.  If merging, always merge to
//   // the left.
//   bool leafShift(Leaf& left, Leaf& right);
//
//   // Always split right and return new right node if split occurs.
//   Maybe<Leaf> leafSplit(Leaf& leaf);
//
//   // Leaf has been updated, and needs to be written to storage.  Return new
//   // pointer (may be different).  Leaf will not be used after store.
//   Pointer storeLeaf(Leaf leaf);
//
//   // Leaf is no longer part of this BTree.  Leaf will not be used after
//   // delete.
//   void deleteLeaf(Leaf leaf);
// };
template <typename Base>
class BTreeMixin : public Base {
public:
  typedef typename Base::Key Key;
  typedef typename Base::Data Data;
  typedef typename Base::Pointer Pointer;

  typedef typename Base::Index Index;
  typedef typename Base::Leaf Leaf;

  bool contains(Key const& k);

  Maybe<Data> find(Key const& k);

  // Range is inclusve on lower bound and exclusive on upper bound.
  List<pair<Key, Data>> find(Key const& lower, Key const& upper);

  // Visitor is called as visitor(key, data).
  template <typename Visitor>
  void forEach(Key const& lower, Key const& upper, Visitor&& visitor);

  // Visitor is called as visitor(key, data).
  template <typename Visitor>
  void forAll(Visitor&& visitor);

  // Recover all key value pairs possible, catching exceptions during scan and
  // reading as much data as possible.  Visitor is called as visitor(key, data),
  // ErrorHandler is called as error(char const*, std::exception const&)
  template <typename Visitor, typename ErrorHandler>
  void recoverAll(Visitor&& visitor, ErrorHandler&& error);

  // Visitor is called either as visitor(Index const&) or visitor(Leaf const&).
  // Return false to halt traversal, true to continue.
  template <typename Visitor>
  void forAllNodes(Visitor&& visitor);

  // returns true if old value overwritten.
  bool insert(Key k, Data data);

  // returns true if key was found.
  bool remove(Key k);

  // Removes list of keys in the given range, returns count removed.
  // TODO: SLOW, right now does lots of different removes separately.  Need to
  // implement batch inserts and deletes.
  List<pair<Key, Data>> remove(Key const& lower, Key const& upper);

  uint64_t indexCount();
  uint64_t leafCount();
  uint64_t recordCount();

  uint32_t indexLevels();

  void createNewRoot();

private:
  struct DataElement {
    Key key;
    Data data;
  };
  typedef List<DataElement> DataList;

  struct DataCollector {
    void operator()(Key const& k, Data const& d);

    List<pair<Key, Data>> list;
  };

  struct RecordCounter {
    bool operator()(Index const& index);
    bool operator()(Leaf const& leaf);

    BTreeMixin* parent;
    uint64_t count;
  };

  struct IndexCounter {
    bool operator()(Index const& index);
    bool operator()(Leaf const&);

    BTreeMixin* parent;
    uint64_t count;
  };

  struct LeafCounter {
    bool operator()(Index const& index);
    bool operator()(Leaf const&);

    BTreeMixin* parent;
    uint64_t count;
  };

  enum ModifyAction {
    InsertAction,
    RemoveAction
  };

  enum ModifyState {
    LeafNeedsJoin,
    IndexNeedsJoin,
    LeafSplit,
    IndexSplit,
    LeafNeedsUpdate,
    IndexNeedsUpdate,
    Done
  };

  struct ModifyInfo {
    ModifyInfo(ModifyAction a, DataElement e);

    DataElement targetElement;
    ModifyAction action;
    bool found;
    ModifyState state;

    Key newKey;
    Pointer newPointer;
  };

  bool contains(Index const& index, Key const& k);
  bool contains(Leaf const& leaf, Key const& k);

  Maybe<Data> find(Index const& index, Key const& k);
  Maybe<Data> find(Leaf const& leaf, Key const& k);

  // Returns the highest key for the last leaf we have searched
  template <typename Visitor>
  Key forEach(Index const& index, Key const& lower, Key const& upper, Visitor&& o);
  template <typename Visitor>
  Key forEach(Leaf const& leaf, Key const& lower, Key const& upper, Visitor&& o);

  // Returns the highest key for the last leaf we have searched
  template <typename Visitor>
  Key forAll(Index const& index, Visitor&& o);
  template <typename Visitor>
  Key forAll(Leaf const& leaf, Visitor&& o);

  template <typename Visitor, typename ErrorHandler>
  void recoverAll(Index const& index, Visitor&& o, ErrorHandler&& error);
  template <typename Visitor, typename ErrorHandler>
  void recoverAll(Leaf const& leaf, Visitor&& o, ErrorHandler&& error);

  // Variable size values mean that merges can happen on inserts, so can't
  // split up into insert / remove methods
  void modify(Leaf& leafNode, ModifyInfo& info);
  void modify(Index& indexNode, ModifyInfo& info);
  bool modify(DataElement e, ModifyAction action);

  // Traverses Indexes down the tree on the left side to get the least valued
  // key that is pointed to by any leaf under this index.  Needed when joining.
  Key getLeftKey(Index const& index);

  template <typename Visitor>
  void forAllNodes(Index const& index, Visitor&& visitor);

  pair<size_t, bool> leafFind(Leaf const& leaf, Key const& key);
  size_t indexFind(Index const& index, Key const& key);
};

template <typename Base>
bool BTreeMixin<Base>::contains(Key const& k) {
  if (Base::rootIsLeaf())
    return contains(Base::loadLeaf(Base::rootPointer()), k);
  else
    return contains(Base::loadIndex(Base::rootPointer()), k);
}

template <typename Base>
auto BTreeMixin<Base>::find(Key const& k) -> Maybe<Data> {
  if (Base::rootIsLeaf())
    return find(Base::loadLeaf(Base::rootPointer()), k);
  else
    return find(Base::loadIndex(Base::rootPointer()), k);
}

template <typename Base>
auto BTreeMixin<Base>::find(Key const& lower, Key const& upper) -> List<pair<Key, Data>> {
  DataCollector collector;
  forEach(lower, upper, collector);
  return collector.list;
}

template <typename Base>
template <typename Visitor>
void BTreeMixin<Base>::forEach(Key const& lower, Key const& upper, Visitor&& visitor) {
  if (Base::rootIsLeaf())
    forEach(Base::loadLeaf(Base::rootPointer()), lower, upper, forward<Visitor>(visitor));
  else
    forEach(Base::loadIndex(Base::rootPointer()), lower, upper, forward<Visitor>(visitor));
}

template <typename Base>
template <typename Visitor>
void BTreeMixin<Base>::forAll(Visitor&& visitor) {
  if (Base::rootIsLeaf())
    forAll(Base::loadLeaf(Base::rootPointer()), forward<Visitor>(visitor));
  else
    forAll(Base::loadIndex(Base::rootPointer()), forward<Visitor>(visitor));
}

template <typename Base>
template <typename Visitor, typename ErrorHandler>
void BTreeMixin<Base>::recoverAll(Visitor&& visitor, ErrorHandler&& error) {
  try {
    if (Base::rootIsLeaf())
      recoverAll(Base::loadLeaf(Base::rootPointer()), forward<Visitor>(visitor), forward<ErrorHandler>(error));
    else
      recoverAll(Base::loadIndex(Base::rootPointer()), forward<Visitor>(visitor), forward<ErrorHandler>(error));
  } catch (std::exception const& e) {
    error("Error loading root index or leaf node", e);
  }
}

template <typename Base>
template <typename Visitor>
void BTreeMixin<Base>::forAllNodes(Visitor&& visitor) {
  if (Base::rootIsLeaf())
    visitor(Base::loadLeaf(Base::rootPointer()));
  else
    forAllNodes(Base::loadIndex(Base::rootPointer()), forward<Visitor>(visitor));
}

template <typename Base>
bool BTreeMixin<Base>::insert(Key k, Data data) {
  return modify(DataElement{move(k), move(data)}, InsertAction);
}

template <typename Base>
bool BTreeMixin<Base>::remove(Key k) {
  return modify(DataElement{move(k), Data()}, RemoveAction);
}

template <typename Base>
auto BTreeMixin<Base>::remove(Key const& lower, Key const& upper) -> List<pair<Key, Data>> {
  DataCollector collector;
  forEach(lower, upper, collector);

  for (auto const& elem : collector.list)
    remove(elem.first);

  return collector.list;
}

template <typename Base>
uint64_t BTreeMixin<Base>::indexCount() {
  IndexCounter counter = {this, 0};
  forAllNodes(counter);
  return counter.count;
}

template <typename Base>
uint64_t BTreeMixin<Base>::leafCount() {
  LeafCounter counter = {this, 0};
  forAllNodes(counter);
  return counter.count;
}

template <typename Base>
uint64_t BTreeMixin<Base>::recordCount() {
  RecordCounter counter = {this, 0};
  forAllNodes(counter);
  return counter.count;
}

template <typename Base>
uint32_t BTreeMixin<Base>::indexLevels() {
  if (Base::rootIsLeaf())
    return 0;
  else
    return Base::indexLevel(Base::loadIndex(Base::rootPointer())) + 1;
}

template <typename Base>
void BTreeMixin<Base>::createNewRoot() {
  Base::setNewRoot(Base::storeLeaf(Base::createLeaf()), true);
}

template <typename Base>
void BTreeMixin<Base>::DataCollector::operator()(Key const& k, Data const& d) {
  list.push_back({k, d});
}

template <typename Base>
bool BTreeMixin<Base>::RecordCounter::operator()(Index const&) {
  return true;
}

template <typename Base>
bool BTreeMixin<Base>::RecordCounter::operator()(Leaf const& leaf) {
  count += parent->leafElementCount(leaf);
  return true;
}

template <typename Base>
bool BTreeMixin<Base>::IndexCounter::operator()(Index const& index) {
  ++count;
  if (parent->indexLevel(index) == 0)
    return false;
  else
    return true;
}

template <typename Base>
bool BTreeMixin<Base>::IndexCounter::operator()(Leaf const&) {
  return false;
}

template <typename Base>
bool BTreeMixin<Base>::LeafCounter::operator()(Index const& index) {
  if (parent->indexLevel(index) == 0) {
    count += parent->indexPointerCount(index);
    return false;
  } else {
    return true;
  }
}

template <typename Base>
bool BTreeMixin<Base>::LeafCounter::operator()(Leaf const&) {
  return false;
}

template <typename Base>
BTreeMixin<Base>::ModifyInfo::ModifyInfo(ModifyAction a, DataElement e)
  : targetElement(move(e)), action(a) {
  found = false;
  state = Done;
}

template <typename Base>
bool BTreeMixin<Base>::contains(Index const& index, Key const& k) {
  size_t i = indexFind(index, k);
  if (Base::indexLevel(index) == 0)
    return contains(Base::loadLeaf(Base::indexPointer(index, i)), k);
  else
    return contains(Base::loadIndex(Base::indexPointer(index, i)), k);
}

template <typename Base>
bool BTreeMixin<Base>::contains(Leaf const& leaf, Key const& k) {
  return leafFind(leaf, k).second;
}

template <typename Base>
auto BTreeMixin<Base>::find(Index const& index, Key const& k) -> Maybe<Data> {
  size_t i = indexFind(index, k);
  if (Base::indexLevel(index) == 0)
    return find(Base::loadLeaf(Base::indexPointer(index, i)), k);
  else
    return find(Base::loadIndex(Base::indexPointer(index, i)), k);
}

template <typename Base>
auto BTreeMixin<Base>::find(Leaf const& leaf, Key const& k) -> Maybe<Data> {
  pair<size_t, bool> res = leafFind(leaf, k);
  if (res.second)
    return Base::leafData(leaf, res.first);
  else
    return {};
}

template <typename Base>
template <typename Visitor>
auto BTreeMixin<Base>::forEach(Index const& index, Key const& lower, Key const& upper, Visitor&& o) -> Key {
  size_t i = indexFind(index, lower);
  Key lastKey;

  if (Base::indexLevel(index) == 0)
    lastKey = forEach(Base::loadLeaf(Base::indexPointer(index, i)), lower, upper, forward<Visitor>(o));
  else
    lastKey = forEach(Base::loadIndex(Base::indexPointer(index, i)), lower, upper, forward<Visitor>(o));

  if (!(lastKey < upper))
    return lastKey;

  while (i < Base::indexPointerCount(index) - 1) {
    ++i;

    // We're visiting the right side of the key, so if lastKey >=
    // indexKeyBefore(index, i), we have already visited this node via nextLeaf
    // pointers, so skip it.
    if (!(lastKey < Base::indexKeyBefore(index, i)))
      continue;

    if (Base::indexLevel(index) == 0)
      lastKey = forEach(Base::loadLeaf(Base::indexPointer(index, i)), lower, upper, forward<Visitor>(o));
    else
      lastKey = forEach(Base::loadIndex(Base::indexPointer(index, i)), lower, upper, forward<Visitor>(o));

    if (!(lastKey < upper))
      break;
  }

  return lastKey;
}

template <typename Base>
template <typename Visitor>
auto BTreeMixin<Base>::forEach(Leaf const& leaf, Key const& lower, Key const& upper, Visitor&& o) -> Key {
  if (Base::leafElementCount(leaf) == 0)
    return Key();

  size_t lowerIndex = leafFind(leaf, lower).first;

  for (size_t i = lowerIndex; i != Base::leafElementCount(leaf); ++i) {
    Key currentKey = Base::leafKey(leaf, i);
    if (!(currentKey < lower)) {
      if (currentKey < upper)
        o(currentKey, Base::leafData(leaf, i));
      else
        return currentKey;
    }
  }

  if (auto nextLeafPointer = Base::nextLeaf(leaf))
    return forEach(Base::loadLeaf(*nextLeafPointer), lower, upper, o);
  else
    return Base::leafKey(leaf, Base::leafElementCount(leaf) - 1);
}

template <typename Base>
template <typename Visitor>
auto BTreeMixin<Base>::forAll(Index const& index, Visitor&& o) -> Key {
  Key lastKey;
  for (size_t i = 0; i < Base::indexPointerCount(index); ++i) {
    // If we're to the right of a given key, but lastKey >= this key, then we
    // must have already visited this node via nextLeaf pointers, so we can
    // skip it.
    if (i > 0 && !(lastKey < Base::indexKeyBefore(index, i)))
      continue;

    if (Base::indexLevel(index) == 0)
      lastKey = forAll(Base::loadLeaf(Base::indexPointer(index, i)), forward<Visitor>(o));
    else
      lastKey = forAll(Base::loadIndex(Base::indexPointer(index, i)), forward<Visitor>(o));
  }

  return lastKey;
}

template <typename Base>
template <typename Visitor>
auto BTreeMixin<Base>::forAll(Leaf const& leaf, Visitor&& o) -> Key {
  if (Base::leafElementCount(leaf) == 0)
    return Key();

  for (size_t i = 0; i != Base::leafElementCount(leaf); ++i) {
    Key currentKey = Base::leafKey(leaf, i);
    o(Base::leafKey(leaf, i), Base::leafData(leaf, i));
  }

  if (auto nextLeafPointer = Base::nextLeaf(leaf))
    return forAll(Base::loadLeaf(*nextLeafPointer), forward<Visitor>(o));
  else
    return Base::leafKey(leaf, Base::leafElementCount(leaf) - 1);
}

template <typename Base>
template <typename Visitor, typename ErrorHandler>
void BTreeMixin<Base>::recoverAll(Index const& index, Visitor&& visitor, ErrorHandler&& error) {
  try {
    for (size_t i = 0; i < Base::indexPointerCount(index); ++i) {
      if (Base::indexLevel(index) == 0) {
        try {
          recoverAll(Base::loadLeaf(Base::indexPointer(index, i)), forward<Visitor>(visitor), forward<ErrorHandler>(error));
        } catch (std::exception const& e) {
          error("Error loading leaf node", e);
        }
      } else {
        try {
          recoverAll(Base::loadIndex(Base::indexPointer(index, i)), forward<Visitor>(visitor), forward<ErrorHandler>(error));
        } catch (std::exception const& e) {
          error("Error loading index node", e);
        }
      }
    }
  } catch (std::exception const& e) {
    error("Error reading index node", e);
  }
}

template <typename Base>
template <typename Visitor, typename ErrorHandler>
void BTreeMixin<Base>::recoverAll(Leaf const& leaf, Visitor&& visitor, ErrorHandler&& error) {
  try {
    for (size_t i = 0; i != Base::leafElementCount(leaf); ++i) {
      Key currentKey = Base::leafKey(leaf, i);
      visitor(Base::leafKey(leaf, i), Base::leafData(leaf, i));
    }
  } catch (std::exception const& e) {
    error("Error reading leaf node", e);
  }
}

template <typename Base>
void BTreeMixin<Base>::modify(Leaf& leafNode, ModifyInfo& info) {
  info.state = Done;

  pair<size_t, bool> res = leafFind(leafNode, info.targetElement.key);
  size_t i = res.first;
  if (res.second) {
    info.found = true;
    Base::leafRemove(leafNode, i);
  }

  // No change necessary.
  if (info.action == RemoveAction && !info.found)
    return;

  if (info.action == InsertAction)
    Base::leafInsert(leafNode, i, info.targetElement.key, move(info.targetElement.data));

  auto splitResult = Base::leafSplit(leafNode);
  if (splitResult) {
    Base::setNextLeaf(*splitResult, Base::nextLeaf(leafNode));
    info.newKey = Base::leafKey(*splitResult, 0);
    info.newPointer = Base::storeLeaf(splitResult.take());

    Base::setNextLeaf(leafNode, info.newPointer);
    info.state = LeafSplit;
  } else if (Base::leafNeedsShift(leafNode)) {
    info.state = LeafNeedsJoin;
  } else {
    info.state = LeafNeedsUpdate;
  }
}

template <typename Base>
void BTreeMixin<Base>::modify(Index& indexNode, ModifyInfo& info) {
  size_t i = indexFind(indexNode, info.targetElement.key);
  Pointer nextPointer = Base::indexPointer(indexNode, i);

  Leaf lowerLeaf;
  Index lowerIndex;
  if (Base::indexLevel(indexNode) == 0) {
    lowerLeaf = Base::loadLeaf(nextPointer);
    modify(lowerLeaf, info);
  } else {
    lowerIndex = Base::loadIndex(nextPointer);
    modify(lowerIndex, info);
  }

  if (info.state == Done)
    return;

  bool selfUpdated = false;

  size_t left = 0;
  size_t right = 0;
  if (i != 0 && i == Base::indexPointerCount(indexNode) - 1) {
    left = i - 1;
    right = i;
  } else {
    left = i;
    right = i + 1;
  }

  if (info.state == LeafNeedsJoin) {
    if (Base::indexPointerCount(indexNode) < 2) {
      // Don't have enough leaves to join, just do the pending update.
      info.state = LeafNeedsUpdate;
    } else {
      Leaf leftLeaf;
      Leaf rightLeaf;

      if (left == i) {
        leftLeaf = lowerLeaf;
        rightLeaf = Base::loadLeaf(Base::indexPointer(indexNode, right));
      } else {
        leftLeaf = Base::loadLeaf(Base::indexPointer(indexNode, left));
        rightLeaf = lowerLeaf;
      }

      if (!Base::leafShift(leftLeaf, rightLeaf)) {
        // Leaves not modified, just do the pending update.
        info.state = LeafNeedsUpdate;
      } else if (Base::leafElementCount(rightLeaf) == 0) {
        // Leaves merged.
        Base::setNextLeaf(leftLeaf, Base::nextLeaf(rightLeaf));
        Base::deleteLeaf(move(rightLeaf));

        // Replace two sibling pointer elements with one pointing to merged
        // leaf.
        if (left != 0)
          Base::indexUpdateKeyBefore(indexNode, left, Base::leafKey(leftLeaf, 0));

        Base::indexUpdatePointer(indexNode, left, Base::storeLeaf(move(leftLeaf)));
        Base::indexRemoveBefore(indexNode, right);

        selfUpdated = true;
      } else {
        // Leaves shifted.
        Base::indexUpdatePointer(indexNode, left, Base::storeLeaf(move(leftLeaf)));

        // Right leaf first key changes on shift, so always need to update
        // left index node.
        Base::indexUpdateKeyBefore(indexNode, right, Base::leafKey(rightLeaf, 0));

        Base::indexUpdatePointer(indexNode, right, Base::storeLeaf(move(rightLeaf)));

        selfUpdated = true;
      }
    }
  }

  if (info.state == IndexNeedsJoin) {
    if (Base::indexPointerCount(indexNode) < 2) {
      // Don't have enough indexes to join, just do the pending update.
      info.state = IndexNeedsUpdate;
    } else {
      Index leftIndex;
      Index rightIndex;

      if (left == i) {
        leftIndex = lowerIndex;
        rightIndex = Base::loadIndex(Base::indexPointer(indexNode, right));
      } else {
        leftIndex = Base::loadIndex(Base::indexPointer(indexNode, left));
        rightIndex = lowerIndex;
      }

      if (!Base::indexShift(leftIndex, getLeftKey(rightIndex), rightIndex)) {
        // Indexes not modified, just do the pending update.
        info.state = IndexNeedsUpdate;

      } else if (Base::indexPointerCount(rightIndex) == 0) {
        // Indexes merged.
        Base::deleteIndex(move(rightIndex));

        // Replace two sibling pointer elements with one pointing to merged
        // index.
        if (left != 0)
          Base::indexUpdateKeyBefore(indexNode, left, getLeftKey(leftIndex));

        Base::indexUpdatePointer(indexNode, left, Base::storeIndex(move(leftIndex)));
        Base::indexRemoveBefore(indexNode, right);

        selfUpdated = true;
      } else {
        // Indexes shifted.
        Base::indexUpdatePointer(indexNode, left, Base::storeIndex(move(leftIndex)));

        // Right index first key changes on shift, so always need to update
        // right index node.
        Key keyForRight = getLeftKey(rightIndex);
        Base::indexUpdatePointer(indexNode, right, Base::storeIndex(move(rightIndex)));
        Base::indexUpdateKeyBefore(indexNode, right, keyForRight);

        selfUpdated = true;
      }
    }
  }

  if (info.state == LeafSplit) {
    Base::indexUpdatePointer(indexNode, i, Base::storeLeaf(move(lowerLeaf)));
    Base::indexInsertAfter(indexNode, i, info.newKey, info.newPointer);
    selfUpdated = true;
  }

  if (info.state == IndexSplit) {
    Base::indexUpdatePointer(indexNode, i, Base::storeIndex(move(lowerIndex)));
    Base::indexInsertAfter(indexNode, i, info.newKey, info.newPointer);
    selfUpdated = true;
  }

  if (info.state == LeafNeedsUpdate) {
    Pointer lowerLeafPointer = Base::storeLeaf(move(lowerLeaf));
    if (lowerLeafPointer != Base::indexPointer(indexNode, i)) {
      Base::indexUpdatePointer(indexNode, i, lowerLeafPointer);
      selfUpdated = true;
    }
  }

  if (info.state == IndexNeedsUpdate) {
    Pointer lowerIndexPointer = Base::storeIndex(move(lowerIndex));
    if (lowerIndexPointer != Base::indexPointer(indexNode, i)) {
      Base::indexUpdatePointer(indexNode, i, lowerIndexPointer);
      selfUpdated = true;
    }
  }

  auto splitResult = Base::indexSplit(indexNode);
  if (splitResult) {
    info.newKey = splitResult->first;
    info.newPointer = Base::storeIndex(splitResult.take().second);
    info.state = IndexSplit;
    selfUpdated = true;
  } else if (Base::indexNeedsShift(indexNode)) {
    info.state = IndexNeedsJoin;
  } else if (selfUpdated) {
    info.state = IndexNeedsUpdate;
  } else {
    info.state = Done;
  }
}

template <typename Base>
bool BTreeMixin<Base>::modify(DataElement e, ModifyAction action) {
  ModifyInfo info(action, move(e));

  Leaf lowerLeaf;
  Index lowerIndex;
  if (Base::rootIsLeaf()) {
    lowerLeaf = Base::loadLeaf(Base::rootPointer());
    modify(lowerLeaf, info);
  } else {
    lowerIndex = Base::loadIndex(Base::rootPointer());
    modify(lowerIndex, info);
  }

  if (info.state == IndexNeedsJoin) {
    if (Base::indexPointerCount(lowerIndex) == 1) {
      // If root index has single pointer, then make that the new root.

      // release index first (to support the common use case of delaying
      // removes until setNewRoot)
      Pointer pointer = Base::indexPointer(lowerIndex, 0);
      size_t level = Base::indexLevel(lowerIndex);
      Base::deleteIndex(move(lowerIndex));
      Base::setNewRoot(pointer, level == 0);
    } else {
      // Else just update.
      info.state = IndexNeedsUpdate;
    }
  }

  if (info.state == LeafNeedsJoin) {
    // Ignore NeedsJoin on LeafNode root, just update.
    info.state = LeafNeedsUpdate;
  }

  if (info.state == LeafSplit || info.state == IndexSplit) {
    Index newRoot;
    if (info.state == IndexSplit) {
      auto rootIndexLevel = Base::indexLevel(lowerIndex) + 1;
      newRoot = Base::createIndex(Base::storeIndex(move(lowerIndex)));
      Base::setIndexLevel(newRoot, rootIndexLevel);
    } else {
      newRoot = Base::createIndex(Base::storeLeaf(move(lowerLeaf)));
      Base::setIndexLevel(newRoot, 0);
    }
    Base::indexInsertAfter(newRoot, 0, info.newKey, info.newPointer);
    Base::setNewRoot(Base::storeIndex(move(newRoot)), false);
  }

  if (info.state == IndexNeedsUpdate) {
    Pointer newRootPointer = Base::storeIndex(move(lowerIndex));
    if (newRootPointer != Base::rootPointer())
      Base::setNewRoot(newRootPointer, false);
  }

  if (info.state == LeafNeedsUpdate) {
    Pointer newRootPointer = Base::storeLeaf(move(lowerLeaf));
    if (newRootPointer != Base::rootPointer())
      Base::setNewRoot(newRootPointer, true);
  }

  return info.found;
}

template <typename Base>
auto BTreeMixin<Base>::getLeftKey(Index const& index) -> Key {
  if (Base::indexLevel(index) == 0) {
    Leaf leaf = Base::loadLeaf(Base::indexPointer(index, 0));
    return Base::leafKey(leaf, 0);
  } else {
    return getLeftKey(Base::loadIndex(Base::indexPointer(index, 0)));
  }
}

template <typename Base>
template <typename Visitor>
void BTreeMixin<Base>::forAllNodes(Index const& index, Visitor&& visitor) {
  if (!visitor(index))
    return;

  for (size_t i = 0; i < Base::indexPointerCount(index); ++i) {
    if (Base::indexLevel(index) != 0) {
      forAllNodes(Base::loadIndex(Base::indexPointer(index, i)), forward<Visitor>(visitor));
    } else {
      if (!visitor(Base::loadLeaf(Base::indexPointer(index, i))))
        return;
    }
  }
}

template <typename Base>
pair<size_t, bool> BTreeMixin<Base>::leafFind(Leaf const& leaf, Key const& key) {
  // Return lower bound binary search result.
  size_t size = Base::leafElementCount(leaf);
  if (size == 0)
    return {0, false};

  size_t len = size;
  size_t first = 0;
  size_t middle = 0;
  size_t half;
  while (len > 0) {
    half = len / 2;
    middle = first + half;
    if (Base::leafKey(leaf, middle) < key) {
      first = middle + 1;
      len = len - half - 1;
    } else {
      len = half;
    }
  }
  return make_pair(first, first < size && !(key < Base::leafKey(leaf, first)));
}

template <typename Base>
size_t BTreeMixin<Base>::indexFind(Index const& index, Key const& key) {
  // Return upper bound binary search result of range [1, size];
  size_t size = Base::indexPointerCount(index);
  if (size == 0)
    return 0;

  size_t len = size - 1;
  size_t first = 1;
  size_t middle = 1;
  size_t half;
  while (len > 0) {
    half = len / 2;
    middle = first + half;
    if (key < Base::indexKeyBefore(index, middle)) {
      len = half;
    } else {
      first = middle + 1;
      len = len - half - 1;
    }
  }
  return first - 1;
}

}

#endif

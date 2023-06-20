#ifndef STAR_SPATIAL_HASH_2D_HPP
#define STAR_SPATIAL_HASH_2D_HPP

#include "StarRect.hpp"
#include "StarMap.hpp"
#include "StarSet.hpp"
#include "StarBlockAllocator.hpp"

namespace Star {

// Dual-map based on key and 2 dimensional bounding rectangle.  Implements a 2d
// spatial hash for fast bounding box queries.  Each entry may have more than
// one bounding rectangle.
template <typename KeyT, typename ScalarT, typename ValueT, typename IntT = int, size_t AllocatorBlockSize = 4096>
class SpatialHash2D {
public:
  typedef KeyT Key;
  typedef ScalarT Scalar;
  typedef Box<ScalarT, 2> Rect;
  typedef typename Rect::Coord Coord;
  typedef ValueT Value;

  struct Entry {
    Entry();

    SmallList<Rect, 2> rects;
    Value value;
  };

  typedef StableHashMap<Key, Entry, hash<Key>, std::equal_to<Key>, BlockAllocator<pair<Key const, Entry>, AllocatorBlockSize>> EntryMap;

  SpatialHash2D(Scalar const& sectorSize);

  List<Key> keys() const;
  List<Value> values() const;
  EntryMap const& entries() const;

  size_t size() const;

  bool contains(Key const& key) const;

  Value const& get(Key const& key) const;
  Value& get(Key const& key);

  // Returns default constructed value if key not found
  Value value(Key const& key) const;

  // Query values from several bounding boxes at once with no duplicates.
  List<Value> queryValues(Rect const& rect) const;
  template <typename RectCollection>
  List<Value> queryValues(RectCollection const& rects) const;

  // Iterate over entries in the given bounding boxes without duplication.  It
  // is safe to modify rects or add entries from the given callback, but it is
  // not safe to remove entries from it.
  template <typename Function>
  void forEach(Rect const& rect, Function&& function) const;
  template <typename RectCollection, typename Function>
  void forEach(RectCollection const& rects, Function&& function) const;

  void set(Key const& key, Coord const& pos);
  void set(Key const& key, Rect const& rect);

  template <typename RectCollection>
  void set(Key const& key, RectCollection const& rects);

  void set(Key const& key, Coord const& pos, Value value);
  void set(Key const& key, Rect const& rect, Value value);

  template <typename RectCollection>
  void set(Key const& key, RectCollection const& rects, Value value);

  Maybe<Value> remove(Key const& key);

  // Recalculates every item in sector map
  void setSectorSize(Scalar const& sectorSize);

private:
  typedef Vector<IntT, 2> Sector;
  typedef Box<IntT, 2> SectorRange;
  typedef HashSet<Entry const*, hash<Entry const*>, std::equal_to<Entry const*>> SectorEntrySet;
  typedef HashMap<Sector, SectorEntrySet> SectorMap;

  SectorRange getSectors(Rect const& r) const;

  void addSpatial(Entry const* entry);
  void removeSpatial(Entry const* entry);

  template <typename RectCollection>
  void updateSpatial(Entry* entry, RectCollection const& rects);

  Scalar m_sectorSize;
  EntryMap m_entryMap;
  SectorMap m_sectorMap;
};

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::Entry::Entry()
  : value() {}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::SpatialHash2D(Scalar const& sectorSize)
  : m_sectorSize(sectorSize) {}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
List<KeyT> SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::keys() const {
  return m_entryMap.keys();
}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
List<typename SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::Value> SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::values() const {
  List<Value> values;
  for (auto const& pair : m_entryMap)
    values.append(pair.second.value);

  return values;
}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
typename SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::EntryMap const&
SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::entries() const {
  return m_entryMap;
}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
size_t SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::size() const {
  return m_entryMap.size();
}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
bool SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::contains(Key const& key) const {
  return m_entryMap.contains(key);
}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
typename SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::Value const& SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::get(
    Key const& key) const {
  return m_entryMap.get(key).value;
}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
typename SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::Value& SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::get(
    Key const& key) {
  return m_entryMap.get(key).value;
}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
typename SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::Value SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::value(
    Key const& key) const {
  auto iter = m_entryMap.find(key);
  if (iter == m_entryMap.end())
    return Value();
  else
    return iter->second.value;
}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
List<ValueT> SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::queryValues(Rect const& rect) const {
  return queryValues(initializer_list<Rect>{rect});
}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
template <typename RectCollection>
List<ValueT> SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::queryValues(RectCollection const& rects) const {
  List<Value> values;
  forEach(rects, [&values](Value const& value) {
      values.append(value);
    });
  return values;
}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
template <typename Function>
void SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::forEach(Rect const& rect, Function&& function) const {
  return forEach(initializer_list<Rect>{rect}, forward<Function>(function));
}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
template <typename RectCollection, typename Function>
void SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::forEach(RectCollection const& rects, Function&& function) const {
  SmallList<Entry const*, 32> foundEntries;

  for (Rect const& rect : rects) {
    if (rect.isNull())
      continue;

    auto sectorResult = getSectors(rect);

    for (IntT x = sectorResult.xMin(); x < sectorResult.xMax(); ++x) {
      for (IntT y = sectorResult.yMin(); y < sectorResult.yMax(); ++y) {
        auto i = m_sectorMap.find(Sector{x, y});
        if (i != m_sectorMap.end()) {
          for (auto e : i->second) {
            for (Rect const& r : e->rects) {
              if (r.intersects(rect)) {
                foundEntries.append(e);
                break;
              }
            }
          }
        }
      }
    }
  }

  // Rather than keep a Set of keys to avoid duplication in found entries, it
  // is much faster to simply keep all encountered intersected entries and then
  // sort them later for all but the most massive and most populated searches,
  // due to the allocation cost of Set and HashSet.
  sort(foundEntries);

  // Looping over the found entries in sorted order with potential duplication,
  // so need to skip over the entry if the previous entry is the same as the
  // current entry
  Entry const* prev = nullptr;
  for (auto const& entry : foundEntries) {
    if (entry == prev)
      continue;
    prev = entry;
    function(entry->value);
  }
}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
void SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::set(Key const& key, Coord const& pos) {
  set(key, {Rect(pos, pos)});
}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
void SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::set(Key const& key, Rect const& rect) {
  set(key, {rect});
}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
template <typename RectCollection>
void SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::set(Key const& key, RectCollection const& rects) {
  updateSpatial(&m_entryMap.get(key), rects);
}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
void SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::set(Key const& key, Coord const& pos, Value value) {
  set(key, {Rect(pos, pos)}, move(value));
}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
void SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::set(Key const& key, Rect const& rect, Value value) {
  set(key, {rect}, move(value));
}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
template <typename RectCollection>
void SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::set(Key const& key, RectCollection const& rects, Value value) {
  Entry& entry = m_entryMap[key];
  entry.value = move(value);
  updateSpatial(&entry, rects);
}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
auto SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::remove(Key const& key) -> Maybe<Value> {
  auto iter = m_entryMap.find(key);
  if (iter == m_entryMap.end())
    return {};

  removeSpatial(&iter->second);
  Maybe<Value> val = move(iter->second.value);
  m_entryMap.erase(iter);
  return val;
}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
void SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::setSectorSize(Scalar const& sectorSize) {
  m_sectorSize = sectorSize;
  m_sectorMap.clear();
  for (auto const& pair : m_entryMap)
    addSpatial(pair.first, pair.second);
}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
typename SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::SectorRange SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::getSectors(Rect const& r) const {
  return SectorRange(
      floor(r.xMin() / m_sectorSize),
      floor(r.yMin() / m_sectorSize),
      ceil(r.xMax() / m_sectorSize),
      ceil(r.yMax() / m_sectorSize));
}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
void SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::addSpatial(Entry const* entry) {
  for (Rect const& rect : entry->rects) {
    if (rect.isNull())
      continue;

    auto sectorResult = getSectors(rect);
    for (IntT x = sectorResult.xMin(); x < sectorResult.xMax(); ++x) {
      for (IntT y = sectorResult.yMin(); y < sectorResult.yMax(); ++y) {
        Sector sector(x, y);
        SectorEntrySet* p = m_sectorMap.ptr(sector);
        if (!p)
          p = &m_sectorMap.add(sector, SectorEntrySet());
        p->add(entry);
      }
    }
  }
}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
void SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::removeSpatial(Entry const* entry) {
  for (Rect const& rect : entry->rects) {
    if (rect.isNull())
      continue;

    auto sectorResult = getSectors(rect);
    for (IntT x = sectorResult.xMin(); x < sectorResult.xMax(); ++x) {
      for (IntT y = sectorResult.yMin(); y < sectorResult.yMax(); ++y) {
        auto i = m_sectorMap.find(Sector{x, y});
        if (i != m_sectorMap.end()) {
          i->second.remove(entry);
          if (i->second.empty())
            m_sectorMap.erase(i);
        }
      }
    }
  }
}

template <typename KeyT, typename ScalarT, typename ValueT, typename IntT, size_t AllocatorBlockSize>
template <typename RectCollection>
void SpatialHash2D<KeyT, ScalarT, ValueT, IntT, AllocatorBlockSize>::updateSpatial(Entry* entry, RectCollection const& rects) {
  removeSpatial(entry);
  entry->rects.clear();
  entry->rects.appendAll(rects);
  addSpatial(entry);
}

}

#endif

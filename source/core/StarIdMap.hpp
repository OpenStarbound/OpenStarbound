#ifndef STAR_ID_MAP_HPP
#define STAR_ID_MAP_HPP

#include "StarMap.hpp"
#include "StarMathCommon.hpp"
#include "StarDataStream.hpp"

namespace Star {

STAR_EXCEPTION(IdMapException, StarException);

// Maps key ids to values with auto generated ids in a given id range.  Tries
// to cycle through ids as new values are added and avoid re-using ids until
// the id space wraps around.
template <typename BaseMap>
class IdMapWrapper : private BaseMap {
public:
  typedef typename BaseMap::iterator iterator;
  typedef typename BaseMap::const_iterator const_iterator;
  typedef typename BaseMap::key_type key_type;
  typedef typename BaseMap::value_type value_type;
  typedef typename BaseMap::mapped_type mapped_type;

  typedef key_type IdType;
  typedef value_type ValueType;
  typedef mapped_type MappedType;

  IdMapWrapper();
  IdMapWrapper(IdType min, IdType max);

  // New valid id that does not exist in this map.  Tries not to immediately
  // recycle ids, to avoid temporally close id repeats.
  IdType nextId();

  // Throws exception if key already exists
  void add(IdType id, MappedType mappedType);

  // Add with automatically allocated id
  IdType add(MappedType mappedType);

  void clear();

  bool operator==(IdMapWrapper const& rhs) const;
  bool operator!=(IdMapWrapper const& rhs) const;

  using BaseMap::keys;
  using BaseMap::values;
  using BaseMap::pairs;
  using BaseMap::contains;
  using BaseMap::size;
  using BaseMap::empty;
  using BaseMap::get;
  using BaseMap::ptr;
  using BaseMap::maybe;
  using BaseMap::take;
  using BaseMap::maybeTake;
  using BaseMap::remove;
  using BaseMap::value;
  using BaseMap::begin;
  using BaseMap::end;
  using BaseMap::erase;

  template <typename Base>
  friend DataStream& operator>>(DataStream& ds, IdMapWrapper<Base>& map);
  template <typename Base>
  friend DataStream& operator<<(DataStream& ds, IdMapWrapper<Base> const& map);

private:
  IdType m_min;
  IdType m_max;
  IdType m_nextId;
};

template <class Key, class Value>
using IdMap = IdMapWrapper<Map<Key, Value>>;

template <class Key, class Value>
using IdHashMap = IdMapWrapper<HashMap<Key, Value>>;

template <typename BaseMap>
IdMapWrapper<BaseMap>::IdMapWrapper()
  : m_min(lowest<IdType>()), m_max(highest<IdType>()), m_nextId(m_min) {}

template <typename BaseMap>
IdMapWrapper<BaseMap>::IdMapWrapper(IdType min, IdType max)
  : m_min(min), m_max(max), m_nextId(m_min) {
  starAssert(m_max > m_min);
}

template <typename BaseMap>
auto IdMapWrapper<BaseMap>::nextId() -> IdType {
  if ((IdType)BaseMap::size() > m_max - m_min)
    throw IdMapException("No id space left in IdMapWrapper");

  IdType nextId = m_nextId;
  while (BaseMap::contains(nextId))
    nextId = cycleIncrement(nextId, m_min, m_max);
  m_nextId = cycleIncrement(nextId, m_min, m_max);
  return nextId;
}

template <typename BaseMap>
void IdMapWrapper<BaseMap>::add(IdType id, MappedType mappedType) {
  if (!BaseMap::insert(make_pair(move(id), move(mappedType))).second)
    throw IdMapException::format("IdMapWrapper::add(id, value) called with pre-existing id '%s'", outputAny(id));
}

template <typename BaseMap>
auto IdMapWrapper<BaseMap>::add(MappedType mappedType) -> IdType {
  auto id = nextId();
  BaseMap::insert(id, mappedType);
  return id;
}

template <typename BaseMap>
void IdMapWrapper<BaseMap>::clear() {
  BaseMap::clear();
  m_nextId = m_min;
}

template <typename BaseMap>
bool IdMapWrapper<BaseMap>::operator==(IdMapWrapper const& rhs) const {
  return tie(m_min, m_max) == tie(rhs.m_min, rhs.m_max) && BaseMap::operator==(rhs);
}

template <typename BaseMap>
bool IdMapWrapper<BaseMap>::operator!=(IdMapWrapper const& rhs) const {
  return !operator==(rhs);
}

template <typename BaseMap>
DataStream& operator>>(DataStream& ds, IdMapWrapper<BaseMap>& map) {
  ds.readMapContainer((BaseMap&)map);
  ds.read(map.m_min);
  ds.read(map.m_max);
  ds.read(map.m_nextId);
  return ds;
}

template <typename BaseMap>
DataStream& operator<<(DataStream& ds, IdMapWrapper<BaseMap> const& map) {
  ds.writeMapContainer((BaseMap const&)map);
  ds.write(map.m_min);
  ds.write(map.m_max);
  ds.write(map.m_nextId);
  return ds;
}

}

#endif

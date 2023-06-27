#ifndef STAR_NET_ELEMENT_CONTAINERS_HPP
#define STAR_NET_ELEMENT_CONTAINERS_HPP

#include "StarMap.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarNetElement.hpp"
#include "StarStrongTypedef.hpp"

namespace Star {

// NetElement map container that is more efficient than the naive serialization
// of an entire Map, because it delta encodes changes to save networking
// traffic.
template <typename BaseMap>
class NetElementMapWrapper : public NetElement, private BaseMap {
public:
  typedef typename BaseMap::iterator iterator;
  typedef typename BaseMap::const_iterator const_iterator;

  typedef typename BaseMap::key_type key_type;
  typedef typename BaseMap::mapped_type mapped_type;
  typedef typename BaseMap::value_type value_type;

  void initNetVersion(NetElementVersion const* version = nullptr) override;

  void enableNetInterpolation(float extrapolationHint = 0.0f) override;
  void disableNetInterpolation() override;
  void tickNetInterpolation(float dt) override;

  void netStore(DataStream& ds) const override;
  void netLoad(DataStream& ds) override;

  bool writeNetDelta(DataStream& ds, uint64_t fromVersion) const override;
  void readNetDelta(DataStream& ds, float interpolationTime = 0.0f) override;

  mapped_type const& get(key_type const& key) const;
  mapped_type const* ptr(key_type const& key) const;

  const_iterator begin() const;
  const_iterator end() const;

  using BaseMap::keys;
  using BaseMap::values;
  using BaseMap::pairs;
  using BaseMap::contains;
  using BaseMap::size;
  using BaseMap::empty;
  using BaseMap::maybe;
  using BaseMap::value;

  pair<const_iterator, bool> insert(value_type v);
  pair<const_iterator, bool> insert(key_type k, mapped_type v);

  void add(key_type k, mapped_type v);
  // Calling set with a matching key and value does not cause a delta to be
  // produced
  void set(key_type k, mapped_type v);
  // set requires that mapped_type implement operator==, push always generates
  // a delta and does not require mapped_type operator==
  void push(key_type k, mapped_type v);

  bool remove(key_type const& k);

  const_iterator erase(const_iterator i);

  mapped_type take(key_type const& k);
  Maybe<mapped_type> maybeTake(key_type const& k);

  void clear();

  BaseMap const& baseMap() const;
  void reset(BaseMap values);
  bool pullUpdated();

  // Sets this map to contain the same keys / values as the given map.  All
  // values in this map not found in the given map are removed.  (Same as
  // reset, but with arbitrary map type).
  template <typename MapType>
  void setContents(MapType const& values);

private:
  // If a delta is written from further back than this many steps, the delta
  // will fall back to a full serialization of the entire state.
  static int64_t const MaxChangeDataVersions = 100;

  struct SetChange {
    key_type key;
    mapped_type value;
  };
  struct RemoveChange {
    key_type key;
  };
  struct ClearChange {};

  typedef Variant<SetChange, RemoveChange, ClearChange> ElementChange;

  static void writeChange(DataStream& ds, ElementChange const& change);
  static ElementChange readChange(DataStream& ds);

  void addChangeData(ElementChange change);

  void addPendingChangeData(ElementChange change, float interpolationTime);
  void applyChange(ElementChange change);

  Deque<pair<uint64_t, ElementChange>> m_changeData;
  Deque<pair<float, ElementChange>> m_pendingChangeData;
  NetElementVersion const* m_netVersion = nullptr;
  uint64_t m_changeDataLastVersion = 0;
  bool m_updated = false;
  bool m_interpolationEnabled = false;
};

template <typename Key, typename Value>
using NetElementMap = NetElementMapWrapper<Map<Key, Value>>;

template <typename Key, typename Value>
using NetElementHashMap = NetElementMapWrapper<HashMap<Key, Value>>;

template <typename BaseMap>
void NetElementMapWrapper<BaseMap>::initNetVersion(NetElementVersion const* version) {
  m_netVersion = version;

  m_changeData.clear();
  m_changeDataLastVersion = 0;

  for (auto& change : Star::take(m_pendingChangeData))
    applyChange(move(change.second));

  addChangeData(ClearChange());
  for (auto const& p : *this)
    addChangeData(SetChange{p.first, p.second});
}

template <typename BaseMap>
void NetElementMapWrapper<BaseMap>::enableNetInterpolation(float) {
  m_interpolationEnabled = true;
}

template <typename BaseMap>
void NetElementMapWrapper<BaseMap>::disableNetInterpolation() {
  m_interpolationEnabled = false;
  for (auto& change : Star::take(m_pendingChangeData))
    applyChange(move(change.second));
}

template <typename BaseMap>
void NetElementMapWrapper<BaseMap>::tickNetInterpolation(float dt) {
  for (auto& p : m_pendingChangeData)
    p.first -= dt;

  while (!m_pendingChangeData.empty() && m_pendingChangeData.first().first <= 0.0f)
    applyChange(m_pendingChangeData.takeFirst().second);
}

template <typename BaseMap>
void NetElementMapWrapper<BaseMap>::netStore(DataStream& ds) const {
  ds.writeVlqU(BaseMap::size() + m_pendingChangeData.size());
  for (auto const& pair : *this)
    writeChange(ds, SetChange{pair.first, pair.second});

  for (auto const& p : m_pendingChangeData)
    writeChange(ds, p.second);
}

template <typename BaseMap>
void NetElementMapWrapper<BaseMap>::netLoad(DataStream& ds) {
  m_changeData.clear();
  m_changeDataLastVersion = m_netVersion ? m_netVersion->current() : 0;
  m_pendingChangeData.clear();
  BaseMap::clear();

  addChangeData(ClearChange());

  uint64_t count = ds.readVlqU();
  for (uint64_t i = 0; i < count; ++i) {
    auto change = readChange(ds);
    addChangeData(change);
    applyChange(move(change));
  }

  m_updated = true;
}

template <typename BaseMap>
bool NetElementMapWrapper<BaseMap>::writeNetDelta(DataStream& ds, uint64_t fromVersion) const {
  bool deltaWritten = false;

  if (fromVersion < m_changeDataLastVersion) {
    deltaWritten = true;
    ds.writeVlqU(1);
    netStore(ds);

  } else {
    for (auto const& p : m_changeData) {
      if (p.first >= fromVersion) {
        deltaWritten = true;
        ds.writeVlqU(2);
        writeChange(ds, p.second);
      }
    }
  }

  if (deltaWritten)
    ds.writeVlqU(0);

  return deltaWritten;
}

template <typename BaseMap>
void NetElementMapWrapper<BaseMap>::readNetDelta(DataStream& ds, float interpolationTime) {
  while (true) {
    uint64_t code = ds.readVlqU();
    if (code == 0) {
      break;
    } else if (code == 1) {
      netLoad(ds);
    } else if (code == 2) {
      auto change = readChange(ds);
      addChangeData(change);

      if (m_interpolationEnabled && interpolationTime > 0.0f)
        addPendingChangeData(move(change), interpolationTime);
      else
        applyChange(move(change));
    } else {
      throw IOException("Improper delta code received in NetElementMapWrapper::readNetDelta");
    }
  }
}

template <typename BaseMap>
auto NetElementMapWrapper<BaseMap>::get(key_type const& key) const -> mapped_type const & {
  return BaseMap::get(key);
}

template <typename BaseMap>
auto NetElementMapWrapper<BaseMap>::ptr(key_type const& key) const -> mapped_type const * {
  return BaseMap::ptr(key);
}

template <typename BaseMap>
auto NetElementMapWrapper<BaseMap>::begin() const -> const_iterator {
  return BaseMap::begin();
}

template <typename BaseMap>
auto NetElementMapWrapper<BaseMap>::end() const -> const_iterator {
  return BaseMap::end();
}

template <typename BaseMap>
auto NetElementMapWrapper<BaseMap>::insert(value_type v) -> pair<const_iterator, bool> {
  auto res = BaseMap::insert(v);
  if (res.second) {
    addChangeData(SetChange{move(v.first), move(v.second)});
    m_updated = true;
  }
  return res;
}

template <typename BaseMap>
auto NetElementMapWrapper<BaseMap>::insert(key_type k, mapped_type v) -> pair<const_iterator, bool> {
  return insert(value_type(move(k), move(v)));
}

template <typename BaseMap>
void NetElementMapWrapper<BaseMap>::add(key_type k, mapped_type v) {
  if (!insert(value_type(move(k), move(v))).second)
    throw MapException::format("Entry with key '{}' already present.", outputAny(k));
}

template <typename BaseMap>
void NetElementMapWrapper<BaseMap>::set(key_type k, mapped_type v) {
  auto i = BaseMap::find(k);
  if (i != BaseMap::end()) {
    if (!(i->second == v)) {
      addChangeData(SetChange{move(k), v});
      i->second = move(v);
      m_updated = true;
    }
  } else {
    addChangeData(SetChange{k, v});
    BaseMap::insert(value_type(move(k), move(v)));
    m_updated = true;
  }
}

template <typename BaseMap>
void NetElementMapWrapper<BaseMap>::push(key_type k, mapped_type v) {
  auto i = BaseMap::find(k);
  if (i != BaseMap::end()) {
    addChangeData(SetChange(move(k), v));
    i->second = move(v);
  } else {
    addChangeData(SetChange(k, v));
    BaseMap::insert(value_type(move(k), move(v)));
  }
  m_updated = true;
}

template <typename BaseMap>
bool NetElementMapWrapper<BaseMap>::remove(key_type const& k) {
  auto i = BaseMap::find(k);
  if (i != BaseMap::end()) {
    BaseMap::erase(i);
    addChangeData(RemoveChange{k});
    m_updated = true;
    return true;
  }
  return false;
}

template <typename BaseMap>
auto NetElementMapWrapper<BaseMap>::erase(const_iterator i) -> const_iterator {
  addChangeData(RemoveChange(i->first));
  m_updated = true;
  return BaseMap::erase(i);
}

template <typename BaseMap>
auto NetElementMapWrapper<BaseMap>::take(key_type const& k) -> mapped_type {
  auto i = BaseMap::find(k);
  if (i == BaseMap::end())
    throw MapException::format("Key '{}' not found in Map::take()", outputAny(k));
  auto m = move(i->second);
  erase(i);
  return m;
}

template <typename BaseMap>
auto NetElementMapWrapper<BaseMap>::maybeTake(key_type const& k) -> Maybe<mapped_type> {
  auto i = BaseMap::find(k);
  if (i == BaseMap::end())
    return {};
  auto m = move(i->second);
  erase(i);
  return Maybe<mapped_type>(move(m));
}

template <typename BaseMap>
void NetElementMapWrapper<BaseMap>::clear() {
  if (!empty()) {
    addChangeData(ClearChange());
    m_updated = true;
    BaseMap::clear();
  }
}

template <typename BaseMap>
BaseMap const& NetElementMapWrapper<BaseMap>::baseMap() const {
  return *this;
}

template <typename BaseMap>
void NetElementMapWrapper<BaseMap>::reset(BaseMap values) {
  for (auto const& p : *this) {
    if (!values.contains(p.first)) {
      addChangeData(RemoveChange{p.first});
      m_updated = true;
    }
  }

  for (auto const& p : values) {
    auto v = ptr(p.first);
    if (!v || !(*v == p.second)) {
      addChangeData(SetChange{p.first, p.second});
      m_updated = true;
    }
  }

  BaseMap::operator=(move(values));
}

template <typename BaseMap>
bool NetElementMapWrapper<BaseMap>::pullUpdated() {
  return Star::take(m_updated);
}

template <typename BaseMap>
template <typename MapType>
void NetElementMapWrapper<BaseMap>::setContents(MapType const& values) {
  reset(BaseMap::from(values));
}

template <typename BaseMap>
void NetElementMapWrapper<BaseMap>::writeChange(DataStream& ds, ElementChange const& change) {
  if (auto sc = change.template ptr<SetChange>()) {
    ds.write<uint8_t>(0);
    ds.write(sc->key);
    ds.write(sc->value);
  } else if (auto rc = change.template ptr<RemoveChange>()) {
    ds.write<uint8_t>(1);
    ds.write(rc->key);
  } else {
    ds.write<uint8_t>(2);
  }
}

template <typename BaseMap>
auto NetElementMapWrapper<BaseMap>::readChange(DataStream& ds) -> ElementChange {
  uint8_t t = ds.read<uint8_t>();
  if (t == 0) {
    SetChange sc;
    ds.read(sc.key);
    ds.read(sc.value);
    return sc;
  } else if (t == 1) {
    RemoveChange rc;
    ds.read(rc.key);
    return rc;
  } else if (t == 2) {
    return ClearChange();
  } else {
    throw IOException("Improper type code received in NetElementMapWrapper::readChange");
  }
}

template <typename BaseMap>
void NetElementMapWrapper<BaseMap>::addChangeData(ElementChange change) {
  uint64_t currentVersion = m_netVersion ? m_netVersion->current() : 0;
  starAssert(m_changeData.empty() || m_changeData.last().first <= currentVersion);

  m_changeData.append({currentVersion, move(change)});

  m_changeDataLastVersion = max<int64_t>((int64_t)currentVersion - MaxChangeDataVersions, 0);
  while (!m_changeData.empty() && m_changeData.first().first < m_changeDataLastVersion)
    m_changeData.removeFirst();
}

template <typename BaseMap>
void NetElementMapWrapper<BaseMap>::addPendingChangeData(ElementChange change, float interpolationTime) {
  if (!m_pendingChangeData.empty() && interpolationTime < m_pendingChangeData.last().first) {
    for (auto& change : Star::take(m_pendingChangeData))
      applyChange(move(change.second));
  }
  m_pendingChangeData.append({interpolationTime, move(change)});
}

template <typename BaseMap>
void NetElementMapWrapper<BaseMap>::applyChange(ElementChange change) {
  if (auto set = change.template ptr<SetChange>())
    BaseMap::set(move(set->key), move(set->value));
  else if (auto remove = change.template ptr<RemoveChange>())
    BaseMap::remove(move(remove->key));
  else
    BaseMap::clear();
  m_updated = true;
}

}

#endif

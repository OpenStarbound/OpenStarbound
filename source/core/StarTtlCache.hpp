#ifndef STAR_TTL_CACHE_HPP
#define STAR_TTL_CACHE_HPP

#include "StarLruCache.hpp"
#include "StarTime.hpp"
#include "StarRandom.hpp"

namespace Star {

template <typename LruCacheType>
class TtlCacheBase {
public:
  typedef typename LruCacheType::Key Key;
  typedef typename LruCacheType::Value::second_type Value;

  typedef function<Value(Key const&)> ProducerFunction;

  TtlCacheBase(int64_t timeToLive = 10000, int timeSmear = 1000, size_t maxSize = NPos, bool ttlUpdateEnabled = true);

  int64_t timeToLive() const;
  void setTimeToLive(int64_t timeToLive);

  int timeSmear() const;
  void setTimeSmear(int timeSmear);

  // If a max size is set, this cache also acts as an LRU cache with the given
  // maximum size.
  size_t maxSize() const;
  void setMaxSize(size_t maxSize = NPos);

  size_t currentSize() const;

  List<Key> keys() const;
  List<Value> values() const;

  // If ttlUpdateEnabled is false, then the time to live for entries will not
  // be updated on access.
  bool ttlUpdateEnabled() const;
  void setTtlUpdateEnabled(bool enabled);

  // If the value is in the cache, returns it and updates the access time,
  // otherwise returns nullptr.
  Value* ptr(Key const& key);

  // Put the given value into the cache.
  void set(Key const& key, Value value);
  // Removes the given value from the cache.  If found and removed, returns
  // true.
  bool remove(Key const& key);

  // Remove all key / value pairs matching a filter.
  void removeWhere(function<bool(Key const&, Value&)> filter);

  // If the value for the key is not found in the cache, produce it with the
  // given producer.  Producer should take the key as an argument and return
  // the Value.
  template <typename Producer>
  Value& get(Key const& key, Producer producer);

  void clear();

  // Cleanup any cached entries that are older than their time to live, if the
  // refreshFilter is given, things that match the refreshFilter instead have
  // their ttl refreshed rather than being removed.
  void cleanup(function<bool(Key const&, Value const&)> refreshFilter = {});

private:
  LruCacheType m_cache;
  int64_t m_timeToLive;
  int m_timeSmear;
  bool m_ttlUpdateEnabled;
};

template <typename Key, typename Value, typename Compare = std::less<Key>, typename Allocator = BlockAllocator<pair<Key const, pair<int64_t, Value>>, 1024>>
using TtlCache = TtlCacheBase<LruCache<Key, pair<int64_t, Value>, Compare, Allocator>>;

template <typename Key, typename Value, typename Hash = Star::hash<Key>, typename Equals = std::equal_to<Key>, typename Allocator = BlockAllocator<pair<Key const, pair<int64_t, Value>>, 1024>>
using HashTtlCache = TtlCacheBase<HashLruCache<Key, pair<int64_t, Value>, Hash, Equals, Allocator>>;

template <typename LruCacheType>
TtlCacheBase<LruCacheType>::TtlCacheBase(int64_t timeToLive, int timeSmear, size_t maxSize, bool ttlUpdateEnabled) {
  m_cache.setMaxSize(maxSize);
  m_timeToLive = timeToLive;
  m_timeSmear = timeSmear;
  m_ttlUpdateEnabled = ttlUpdateEnabled;
}

template <typename LruCacheType>
int64_t TtlCacheBase<LruCacheType>::timeToLive() const {
  return m_timeToLive;
}

template <typename LruCacheType>
void TtlCacheBase<LruCacheType>::setTimeToLive(int64_t timeToLive) {
  m_timeToLive = timeToLive;
}

template <typename LruCacheType>
int TtlCacheBase<LruCacheType>::timeSmear() const {
  return m_timeSmear;
}

template <typename LruCacheType>
void TtlCacheBase<LruCacheType>::setTimeSmear(int timeSmear) {
  m_timeSmear = timeSmear;
}

template <typename LruCacheType>
bool TtlCacheBase<LruCacheType>::ttlUpdateEnabled() const {
  return m_ttlUpdateEnabled;
}

template <typename LruCacheType>
size_t TtlCacheBase<LruCacheType>::maxSize() const {
  return m_cache.maxSize();
}

template <typename LruCacheType>
void TtlCacheBase<LruCacheType>::setMaxSize(size_t maxSize) {
  m_cache.setMaxSize(maxSize);
}

template <typename LruCacheType>
size_t TtlCacheBase<LruCacheType>::currentSize() const {
  return m_cache.currentSize();
}

template <typename LruCacheType>
auto TtlCacheBase<LruCacheType>::keys() const -> List<Key> {
  return m_cache.keys();
}

template <typename LruCacheType>
auto TtlCacheBase<LruCacheType>::values() const -> List<Value> {
  List<Value> values;
  for (auto& p : m_cache.values())
    values.append(std::move(p.second));
  return values;
}

template <typename LruCacheType>
void TtlCacheBase<LruCacheType>::setTtlUpdateEnabled(bool enabled) {
  m_ttlUpdateEnabled = enabled;
}

template <typename LruCacheType>
auto TtlCacheBase<LruCacheType>::ptr(Key const& key) -> Value * {
  if (auto p = m_cache.ptr(key)) {
    if (m_ttlUpdateEnabled)
      p->first = Time::monotonicMilliseconds() + Random::randInt(-m_timeSmear, m_timeSmear);
    return &p->second;
  }
  return nullptr;
}

template <typename LruCacheType>
void TtlCacheBase<LruCacheType>::set(Key const& key, Value value) {
  m_cache.set(key, make_pair(Time::monotonicMilliseconds() + Random::randInt(-m_timeSmear, m_timeSmear), value));
}

template <typename LruCacheType>
bool TtlCacheBase<LruCacheType>::remove(Key const& key) {
  return m_cache.remove(key);
}

template <typename LruCacheType>
void TtlCacheBase<LruCacheType>::removeWhere(function<bool(Key const&, Value&)> filter) {
  m_cache.removeWhere([&filter](auto const& key, auto& value) { return filter(key, value.second); });
}

template <typename LruCacheType>
template <typename Producer>
auto TtlCacheBase<LruCacheType>::get(Key const& key, Producer producer) -> Value & {
  auto& value = m_cache.get(key, [producer](Key const& key) {
      return pair<int64_t, Value>(0, producer(key));
    });
  if (value.first == 0 || m_ttlUpdateEnabled)
    value.first = Time::monotonicMilliseconds() + Random::randInt(-m_timeSmear, m_timeSmear);
  return value.second;
}

template <typename LruCacheType>
void TtlCacheBase<LruCacheType>::clear() {
  m_cache.clear();
}

template <typename LruCacheType>
void TtlCacheBase<LruCacheType>::cleanup(function<bool(Key const&, Value const&)> refreshFilter) {
  int64_t currentTime = Time::monotonicMilliseconds();
  m_cache.removeWhere([&](auto const& key, auto& value) {
      if (refreshFilter && refreshFilter(key, value.second)) {
        value.first = currentTime;
      } else {
        if (currentTime - value.first > m_timeToLive)
          return true;
      }
      return false;
    });
}

}

#endif

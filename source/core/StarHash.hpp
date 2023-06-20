#ifndef STAR_HASH_HPP
#define STAR_HASH_HPP

#include "StarBytes.hpp"

namespace Star {

// To avoid having to specialize std::hash in the std namespace, which is
// slightly annoying, Star type wrappers use Star::hash, which just defaults to
// std::hash.  Star::hash also enables template specialization with a dummy
// Enable parameter.
template <typename T, typename Enable = void>
struct hash : public std::hash<T> {};

inline void hashCombine(size_t& hash, size_t comb) {
  hash ^= comb * 2654435761 + 0x9e3779b9 + (hash << 6) + (hash >> 2);
}

// Paul Larson hashing algorithm, very very *cheap* hashing function.
class PLHasher {
public:
  PLHasher(size_t initial = 0)
    : m_hash(initial) {}

  template <typename T>
  void put(T b) {
    m_hash = m_hash * 101 + (size_t)b;
  }

  size_t hash() const {
    return m_hash;
  }

private:
  size_t m_hash;
};

template <typename first_t, typename second_t>
class hash<std::pair<first_t, second_t>> {
private:
  Star::hash<first_t> firstHasher;
  Star::hash<second_t> secondHasher;

public:
  size_t operator()(std::pair<first_t, second_t> const& a) const {
    size_t hashval = firstHasher(a.first);
    hashCombine(hashval, secondHasher(a.second));
    return hashval;
  }
};

template <typename... TTypes>
class hash<std::tuple<TTypes...>> {
private:
  typedef std::tuple<TTypes...> Tuple;

  template <size_t N>
  size_t operator()(Tuple const&) const {
    return 0;
  }

  template <size_t N, typename THead, typename... TTail>
  size_t operator()(Tuple const& value) const {
    size_t hash = Star::hash<THead>()(std::get<N - sizeof...(TTail) - 1>(value));
    hashCombine(hash, operator()<N, TTail...>(value));
    return hash;
  }

public:
  size_t operator()(Tuple const& value) const {
    return operator()<sizeof...(TTypes), TTypes...>(value);
  }
};

template <typename EnumType>
class hash<EnumType, typename std::enable_if<std::is_enum<EnumType>::value>::type> {
private:
  typedef typename std::underlying_type<EnumType>::type UnderlyingType;

public:
  size_t operator()(EnumType e) const {
    return std::hash<UnderlyingType>()((UnderlyingType)e);
  }
};

template <typename T>
size_t hashOf(T const& t) {
  return Star::hash<T>()(t);
}

template <typename T1, typename T2, typename... TL>
size_t hashOf(T1 const& t1, T2 const& t2, TL const&... rest) {
  size_t hash = hashOf(t1);
  hashCombine(hash, hashOf(t2, rest...));
  return hash;
};

}

#endif

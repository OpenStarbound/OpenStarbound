#pragma once
#include "StarDataStream.hpp"

namespace Star {


enum class NetCompatibilityFilter {
  None = 0,
  Old = 1,
  New = 2
};

struct NetCompatibilityRules {
  NetCompatibilityRules() = default;
  NetCompatibilityRules(uint64_t) = delete;
  NetCompatibilityRules(bool legacy);

  bool checkFilter(NetCompatibilityFilter const& filter) const;

  bool isLegacy = false;
};

inline NetCompatibilityRules::NetCompatibilityRules(bool legacy) : isLegacy(legacy) {}

inline bool NetCompatibilityRules::checkFilter(NetCompatibilityFilter const& filter) const {
  if (filter == NetCompatibilityFilter::None)
    return true;
  else if (isLegacy)
    return filter == NetCompatibilityFilter::Old;
  else
    return filter == NetCompatibilityFilter::New;
}

template <>
struct hash<NetCompatibilityRules> {
  size_t operator()(NetCompatibilityRules const& s) const {
    return s.isLegacy;
  }
};

}
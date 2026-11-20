#pragma once

#include "StarAssetPath.hpp"
#include "StarMap.hpp"

namespace Star {

// Parsing an asset path with directives (?crop=..?replace=..) is expensive and
// scripts hand the same strings to the engine every frame.
//
// A parsed AssetPath only depends on its string and the Directives inside are
// safe to share between threads, so recently parsed ones are kept per thread.
inline AssetPath cachedAssetPath(String const& path) {
  thread_local HashMap<String, AssetPath> cache;
  if (auto found = cache.ptr(path))
    return *found;
  if (cache.size() >= 1024)
    cache.clear();
  return cache.insert(make_pair(path, AssetPath(path))).first->second;
}

}

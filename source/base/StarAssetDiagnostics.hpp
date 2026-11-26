#pragma once

#include "StarAssets.hpp"
#include "StarAssetSource.hpp"
#include "StarFormat.hpp"

namespace Star {
namespace AssetDiagnostics {

inline String describeSource(AssetsConstPtr const& assets, AssetSourcePtr const& source) {
  if (!source)
    return "unknown asset source";

  auto metadata = source->metadata();
  auto sourcePath = assets->assetSourcePath(source).value("unregistered asset source");
  auto name = metadata.value("name", Json());
  auto friendlyName = metadata.value("friendlyName", Json());
  String label;
  if (friendlyName.isType(Json::Type::String))
    label = friendlyName.toString();
  if (name.isType(Json::Type::String) && name.toString() != label) {
    if (!label.empty()) label += " / ";
    label += name.toString();
  }
  return label.empty() ? strf("source '{}'", sourcePath) : strf("mod '{}' (source '{}')", label, sourcePath);
}

inline String describeDefinition(AssetsConstPtr const& assets, String const& name, String const& path) {
  String description = strf("'{}' at '{}'", name, path);
  if (auto descriptor = assets->assetDescriptor(path)) {
    description += " from " + describeSource(assets, descriptor->source);
    // Patches are contributors, not necessarily the cause of a conflict.
    for (auto const& patch : descriptor->patchSources)
      description += strf("\n    Applied patch '{}' from {}", patch.first, describeSource(assets, patch.second));
  } else {
    description += " (asset source unavailable)";
  }
  return description;
}

}
}

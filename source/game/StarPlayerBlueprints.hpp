#pragma once

#include "StarByteArray.hpp"
#include "StarSet.hpp"
#include "StarItemDescriptor.hpp"

namespace Star {

STAR_CLASS(PlayerBlueprints);

class PlayerBlueprints {
public:
  PlayerBlueprints();
  PlayerBlueprints(Json const& json);

  Json toJson() const;

  bool isKnown(ItemDescriptor const& itemDescriptor) const;
  bool isNew(ItemDescriptor const& itemDescriptor) const;
  void add(ItemDescriptor const& itemDescriptor);
  void markAsRead(ItemDescriptor const& itemDescriptor);

private:
  HashSet<ItemDescriptor> m_knownBlueprints;
  HashSet<ItemDescriptor> m_newBlueprints;
};

}

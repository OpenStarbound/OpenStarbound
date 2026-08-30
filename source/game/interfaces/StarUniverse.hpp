#pragma once

#include "StarConfig.hpp"

namespace Star {

STAR_CLASS(Universe);
STAR_CLASS(CelestialDatabase);

// Shared base class for UniverseClient and UniverseServer
class Universe {
public:
  virtual CelestialDatabasePtr celestialDatabase() = 0;
};

}

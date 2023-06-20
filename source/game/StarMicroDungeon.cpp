#include "StarMicroDungeon.hpp"
#include "StarRoot.hpp"
#include "StarInterpolation.hpp"
#include "StarLogging.hpp"
#include "StarDungeonGenerator.hpp"

namespace Star {

// Placed on server so it can keep a cacheing system which allows for a quick
// scan if a piece fits.

MicroDungeonFactory::MicroDungeonFactory() {
  m_generating = false;

  m_placementshifts.push_back(0);
  for (int i = 1; i < 4; ++i)
    m_placementshifts.push_back(i);
  for (int i = 1; i < 4; ++i)
    m_placementshifts.push_back(-i);
}

Maybe<pair<List<RectI>, Set<Vec2I>>> MicroDungeonFactory::generate(RectI const& bounds,
    String const& dungeonName,
    Vec2I const& position,
    uint64_t seed,
    float threatLevel,
    DungeonGeneratorWorldFacadePtr facade,
    bool forcePlacement) {
  Dungeon::DungeonGeneratorWriter writer(facade, {}, {});

  if (m_generating)
    throw DungeonException("Not reentrant.");

  m_generating = true;
  auto generatingGuard = finally([this]() { m_generating = false; });

  DungeonGenerator dungeonGenerator(dungeonName, seed, threatLevel, BiomeMicroDungeonId);

  try {
    // don't bother scanning around because its used in a bruteforce manner for now.
    // try to stay a bit stable generation wise, maybe trash the cache after a sector is done ?

    auto anchorPart = dungeonGenerator.pickAnchor();
    if (!anchorPart) {
      Logger::debug("No valid anchor piece found for microdungeon at %s, skipping", position);
      return {};
    }

    if (forcePlacement) {
      return dungeonGenerator.buildDungeon(anchorPart, position - anchorPart->anchorPoint(), &writer, forcePlacement);
    } else {
      for (int dy : m_placementshifts) {
        auto pos = position - anchorPart->anchorPoint() + Vec2I{0, dy};
        if (!bounds.contains(pos) || !bounds.contains(pos + Vec2I{anchorPart->size()} - Vec2I{1, 1}))
          continue;

        bool collision = false;
        anchorPart->forEachTile([&](Vec2I tilePos, Dungeon::Tile const& tile) -> bool {
            if (tile.usesPlaces()) {
              if (facade->getDungeonIdAt(pos + tilePos) != NoDungeonId) {
                collision = true;
                return true;
              }
            }
            return false;
          });

        if (!collision && anchorPart->canPlace(pos, &writer)) {
          return dungeonGenerator.buildDungeon(anchorPart, pos, &writer, forcePlacement);
        }
      }
    }
  } catch (std::exception const& e) {
    throw DungeonException(strf("Error generating microdungeon named '%s'", dungeonGenerator.definition()->name()), e);
  }

  return {};
}

}

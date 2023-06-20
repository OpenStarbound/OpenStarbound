#ifndef STAR_MATERIAL_RENDER_MATCHING_HPP
#define STAR_MATERIAL_RENDER_MATCHING_HPP

#include "StarRect.hpp"
#include "StarJson.hpp"
#include "StarBiMap.hpp"
#include "StarMultiArray.hpp"
#include "StarGameTypes.hpp"
#include "StarTileDamage.hpp"

namespace Star {

STAR_EXCEPTION(MaterialRenderProfileException, StarException);

enum class MaterialJoinType : uint8_t { All, Any };
extern EnumMap<MaterialJoinType> const MaterialJoinTypeNames;

STAR_STRUCT(MaterialRule);

struct MaterialRule {
  struct RuleEmpty {};
  struct RuleConnects {};
  struct RuleShadows {};
  struct RuleEqualsSelf {
    bool matchHue;
  };

  struct RuleEqualsId {
    uint16_t id;
  };

  struct RulePropertyEquals {
    String propertyName;
    Json compare;
  };

  struct RuleEntry {
    MVariant<RuleEmpty, RuleConnects, RuleShadows, RuleEqualsSelf, RuleEqualsSelf, RuleEqualsId, RulePropertyEquals> rule;
    bool inverse;
  };

  MaterialJoinType join;
  List<RuleEntry> entries;
};
typedef StringMap<MaterialRuleConstPtr> RuleMap;

struct MaterialMatchPoint {
  Vec2I position;
  MaterialRuleConstPtr rule;
};

STAR_STRUCT(MaterialRenderPiece);

struct MaterialRenderPiece {
  size_t pieceId;
  String texture;
  // Maps each MaterialColorVariant to a list of texture coordinates for each
  // random variant
  HashMap<MaterialColorVariant, List<RectF>> variants;
};

STAR_STRUCT(MaterialRenderMatch);
typedef List<MaterialRenderMatchConstPtr> MaterialRenderMatchList;

struct MaterialRenderMatch {
  List<MaterialMatchPoint> matchPoints;
  MaterialJoinType matchJoin;

  // Positions here are in TilePixels
  List<pair<MaterialRenderPieceConstPtr, Vec2F>> resultingPieces;
  MaterialRenderMatchList subMatches;
  Maybe<TileLayer> requiredLayer;
  Maybe<bool> occlude;
  bool haltOnMatch;
  bool haltOnSubMatch;
};

typedef StringMap<MaterialRenderPieceConstPtr> PieceMap;
typedef StringMap<MaterialRenderMatchList> MatchMap;

// This is the maximum distance in either X or Y that material neighbor rules
// are limited to.  This can be used as a maximum limit on the "sphere of
// influence" that a tile can have on other tile's rendering.  A value of 1
// here means "1 away", so would be interpreted as a 3x3 block with the
// rendered tile in the center.
int const MaterialRenderProfileMaxNeighborDistance = 2;

STAR_STRUCT(MaterialRenderProfile);

struct MaterialRenderProfile {
  RuleMap rules;
  PieceMap pieces;
  MatchMap matches;

  String representativePiece;

  MaterialRenderMatchList mainMatchList;
  List<pair<String, Vec2F>> crackingFrames;
  List<pair<String, Vec2F>> protectedFrames;
  Json ruleProperties;

  bool foregroundLightTransparent;
  bool backgroundLightTransparent;
  bool multiColor;
  bool occludesBehind;
  uint32_t zLevel;
  Vec3F radiantLight;

  // Get a single asset path for just a single piece of a material, with the
  // image cropped to the piece itself.
  String pieceImage(String const& pieceName,
      unsigned variant,
      MaterialColorVariant colorVariant = DefaultMaterialColorVariant,
      MaterialHue hueShift = MaterialHue()) const;

  // Get an overlay image for rendering damaged tiles, as well as the offset
  // for it in world coordinates.
  pair<String, Vec2F> const& damageImage(float damageLevel, TileDamageType damageType) const;
};

MaterialRenderProfile parseMaterialRenderProfile(Json const& spec, String const& relativePath = "");

}

#endif

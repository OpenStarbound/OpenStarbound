#include "StarMaterialRenderProfile.hpp"
#include "StarLexicalCast.hpp"
#include "StarJsonExtra.hpp"
#include "StarAssets.hpp"
#include "StarImageMetadataDatabase.hpp"
#include "StarRoot.hpp"

namespace Star {

EnumMap<MaterialJoinType> const MaterialJoinTypeNames = {
    {MaterialJoinType::All, "All"}, {MaterialJoinType::Any, "Any"},
};

MaterialRenderMatchList parseMaterialRenderMatchList(Json const& matchSpec, RuleMap const& ruleMap, PieceMap const& pieceMap, MatchMap const& matchMap) {
  MaterialRenderMatchList matchList;

  for (auto const& matchConfig : matchSpec.toArray()) {
    MaterialRenderMatchPtr match = make_shared<MaterialRenderMatch>();

    Json matchPointList = JsonArray();
    if (auto matchAllPoints = matchConfig.opt("matchAllPoints")) {
      matchPointList = *matchAllPoints;
      match->matchJoin = MaterialJoinType::All;
    } else if (auto matchAnyPoints = matchConfig.opt("matchAnyPoints")) {
      matchPointList = *matchAnyPoints;
      match->matchJoin = MaterialJoinType::Any;
    }

    for (auto const& matchPointConfig : matchPointList.iterateArray()) {
      MaterialMatchPoint matchPoint;
      matchPoint.position = jsonToVec2I(matchPointConfig.get(0));
      if (abs(matchPoint.position[0]) > MaterialRenderProfileMaxNeighborDistance || abs(matchPoint.position[1]) > MaterialRenderProfileMaxNeighborDistance)
        throw MaterialRenderProfileException(strf("Match position {} outside of maximum rule distance {}",
            matchPoint.position, MaterialRenderProfileMaxNeighborDistance));
      matchPoint.rule = ruleMap.get(matchPointConfig.getString(1));
      match->matchPoints.append(std::move(matchPoint));
    }

    for (auto const& piece : matchConfig.getArray("pieces", {}))
      match->resultingPieces.append({pieceMap.get(piece.getString(0)), jsonToVec2F(piece.get(1))});

    auto subMatches = matchConfig.get("subMatches", {});
    if (subMatches.isType(Json::Type::String))
      match->subMatches = matchMap.get(subMatches.toString());
    else if (!subMatches.isNull())
      match->subMatches = parseMaterialRenderMatchList(subMatches, ruleMap, pieceMap, matchMap);

    match->requiredLayer = matchConfig.optString("requiredLayer").apply(bind(&EnumMap<TileLayer>::getLeft, &TileLayerNames, _1));
    match->haltOnMatch = matchConfig.getBool("haltOnMatch", false);
    match->haltOnSubMatch = matchConfig.getBool("haltOnSubMatch", false);

    match->occlude = matchConfig.optBool("occlude");

    matchList.append(match);
  }

  return matchList;
}

String MaterialRenderProfile::pieceImage(String const& pieceName, unsigned variant, MaterialColorVariant colorVariant, MaterialHue hueShift) const {
  auto const& piece = pieces.get(pieceName);

  String texture = piece->texture;
  if (hueShift != MaterialHue())
    texture = strf("{}?hueshift={}", texture, materialHueToDegrees(hueShift));

  auto const& rect = piece->variants.get(colorVariant).wrap(variant);
  return strf("{}?crop={};{};{};{}", texture, rect.xMin(), rect.yMin(), rect.xMax(), rect.yMax());
}

pair<String, Vec2F> const& MaterialRenderProfile::damageImage(float damageLevel, TileDamageType damageType) const {
  if (damageType == TileDamageType::Protected)
    return protectedFrames.at(clamp<unsigned>(damageLevel * crackingFrames.size(), 0, crackingFrames.size() - 1));
  return crackingFrames.at(clamp<unsigned>(damageLevel * crackingFrames.size(), 0, crackingFrames.size() - 1));
}

MaterialRenderProfile parseMaterialRenderProfile(Json const& spec, String const& relativePath) {
  MaterialRenderProfile profile;

  bool lightTransparent = spec.getBool("lightTransparent", false);
  profile.foregroundLightTransparent = spec.getBool("foregroundLightTransparent", lightTransparent);
  profile.backgroundLightTransparent = spec.getBool("backgroundLightTransparent", lightTransparent);
  profile.multiColor = spec.getBool("multiColored", false);
  profile.occludesBehind = spec.getBool("occludesBelow", true);
  profile.zLevel = spec.getUInt("zLevel", 0);
  profile.radiantLight = Color::rgb(jsonToVec3B(spec.get("radiantLight", JsonArray{0, 0, 0}))).toRgbF();

  profile.representativePiece = spec.getString("representativePiece");

  for (auto const& pair : spec.get("rules").iterateObject()) {
    auto rule = make_shared<MaterialRule>();
    rule->join = MaterialJoinTypeNames.getLeft(pair.second.getString("join", "all"));
    for (auto const& ruleEntry : pair.second.getArray("entries", {})) {
      bool inverse = ruleEntry.getBool("inverse", false);
      String type = ruleEntry.getString("type");
      if (type.equalsIgnoreCase("Connects")) {
        rule->entries.append({MaterialRule::RuleConnects(), inverse});
      } else if (type.equalsIgnoreCase("Shadows")) {
        rule->entries.append({MaterialRule::RuleShadows(), inverse});
      } else if (type.equalsIgnoreCase("EqualsSelf")) {
        rule->entries.append({MaterialRule::RuleEqualsSelf{ruleEntry.getBool("matchHue", false)}, inverse});
      } else if (type.equalsIgnoreCase("EqualsId")) {
        rule->entries.append({MaterialRule::RuleEqualsId{(uint16_t)ruleEntry.getUInt("id")}, inverse});
      } else if (type.equalsIgnoreCase("PropertyEquals")) {
        rule->entries.append({MaterialRule::RulePropertyEquals{ruleEntry.getString("propertyName"), ruleEntry.get("propertyValue")}, inverse});
      }
    }
    profile.rules[pair.first] = std::move(rule);
  }

  for (auto const& pair : spec.get("pieces").iterateObject()) {
    auto renderPiece = make_shared<MaterialRenderPiece>();
    renderPiece->pieceId = profile.pieces.size();

    renderPiece->texture = AssetPath::relativeTo(relativePath, pair.second.getString("texture", spec.getString("texture")));
    unsigned variants = pair.second.getUInt("variants", spec.getUInt("variants", 1));

    Vec2F textureSize = jsonToVec2F(pair.second.get("textureSize"));
    Vec2F texturePosition = jsonToVec2F(pair.second.get("texturePosition"));
    Vec2F variantStride = jsonToVec2F(pair.second.get("variantStride", JsonArray{0, 0}));
    Vec2F colorStride = jsonToVec2F(pair.second.get("colorStride", JsonArray{0, 0}));

    // Need to flip texture coordinates because material rendering configs
    // assume top down image coordinates
    unsigned imageHeight = Root::singleton().imageMetadataDatabase()->imageSize(renderPiece->texture)[1];
    auto flipTextureCoordinates = [imageHeight](
        RectF const& rect) { return RectF::withSize(Vec2F(rect.xMin(), imageHeight - rect.yMax()), rect.size()); };
    for (unsigned v = 0; v < variants; ++v) {
      if (profile.multiColor) {
        for (MaterialColorVariant c = 0; c <= MaxMaterialColorVariant; ++c) {
          RectF textureRect = RectF::withSize(texturePosition + variantStride * v + colorStride * c, textureSize);
          renderPiece->variants[c].append(flipTextureCoordinates(textureRect));
        }
      } else {
        RectF textureRect = RectF::withSize(texturePosition + variantStride * v, textureSize);
        renderPiece->variants[DefaultMaterialColorVariant].append(flipTextureCoordinates(textureRect));
      }
    }

    profile.pieces[pair.first] = std::move(renderPiece);
  }

  for (auto const& pair : spec.get("matches").iterateArray())
    profile.matches[pair.getString(0)] = parseMaterialRenderMatchList(pair.get(1), profile.rules, profile.pieces, profile.matches);

  profile.mainMatchList = profile.matches.get("main");

  // TODO: Completely hard-coded for now
  profile.crackingFrames = {
    {"/tiles/blockdamage.png:1", Vec2F(0, 0)},
    {"/tiles/blockdamage.png:2", Vec2F(0, 0)},
    {"/tiles/blockdamage.png:3", Vec2F(0, 0)},
    {"/tiles/blockdamage.png:4", Vec2F(0, 0)},
    {"/tiles/blockdamage.png:5", Vec2F(0, 0)}
  };

  profile.protectedFrames = {
    {"/tiles/blockprotection.png:1", Vec2F(0, 0)},
    {"/tiles/blockprotection.png:2", Vec2F(0, 0)},
    {"/tiles/blockprotection.png:3", Vec2F(0, 0)},
    {"/tiles/blockprotection.png:4", Vec2F(0, 0)},
    {"/tiles/blockprotection.png:5", Vec2F(0, 0)}
  };

  profile.ruleProperties = spec.get("ruleProperties", JsonObject());

  return profile;
}

}

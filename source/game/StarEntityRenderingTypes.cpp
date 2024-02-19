#include "StarEntityRenderingTypes.hpp"
#include "StarJsonExtra.hpp"
#include "StarLexicalCast.hpp"

namespace Star {

EntityRenderLayer parseRenderLayer(String renderLayer) {
  static CaseInsensitiveStringMap<EntityRenderLayer> RenderLayerMap{
    {"BackgroundOverlay", RenderLayerBackgroundOverlay},
    {"BackgroundTile", RenderLayerBackgroundTile},
    {"Platform", RenderLayerPlatform},
    {"Plant", RenderLayerPlant},
    {"PlantDrop", RenderLayerPlantDrop},
    {"Object", RenderLayerObject},
    {"PreviewObject", RenderLayerPreviewObject},
    {"BackParticle", RenderLayerBackParticle},
    {"Vehicle", RenderLayerVehicle},
    {"Effect", RenderLayerEffect},
    {"Projectile", RenderLayerProjectile},
    {"Monster", RenderLayerMonster},
    {"Npc", RenderLayerNpc},
    {"Player", RenderLayerPlayer},
    {"ItemDrop", RenderLayerItemDrop},
    {"Liquid", RenderLayerLiquid},
    {"MiddleParticle", RenderLayerMiddleParticle},
    {"ForegroundTile", RenderLayerForegroundTile},
    {"ForegroundEntity", RenderLayerForegroundEntity},
    {"ForegroundOverlay", RenderLayerForegroundOverlay},
    {"FrontParticle", RenderLayerFrontParticle},
    {"Overlay", RenderLayerOverlay},
  };

  int offset = 0;
  if (renderLayer.contains("+")) {
    StringList parts = renderLayer.split("+", 1);
    renderLayer = parts.at(0);
    offset = lexicalCast<int>(parts.at(1));
  } else if (renderLayer.contains("-")) {
    StringList parts = renderLayer.split("-", 1);
    renderLayer = parts.at(0);
    offset = -lexicalCast<int>(parts.at(1));
  }

  return RenderLayerMap.get(renderLayer) + offset;
}

PreviewTile::PreviewTile()
  : foreground(false),
    liqId(EmptyLiquidId),
    matId(NullMaterialId),
    hueShift(0),
    updateMatId(false),
    colorVariant(DefaultMaterialColorVariant),
    updateLight(false) {}

PreviewTile::PreviewTile(
    Vec2I const& position, bool foreground, MaterialId matId, MaterialHue hueShift, bool updateMatId)
  : position(position),
    foreground(foreground),
    liqId(EmptyLiquidId),
    matId(matId),
    hueShift(hueShift),
    updateMatId(updateMatId),
    colorVariant(DefaultMaterialColorVariant),
    updateLight(false) {}

PreviewTile::PreviewTile(Vec2I const& position, bool foreground, Vec3B const& light, bool updateLight)
  : position(position),
    foreground(foreground),
    liqId(EmptyLiquidId),
    matId(NullMaterialId),
    hueShift(0),
    updateMatId(false),
    colorVariant(DefaultMaterialColorVariant),
    light(light),
    updateLight(updateLight) {}

PreviewTile::PreviewTile(Vec2I const& position,
    bool foreground,
    MaterialId matId,
    MaterialHue hueShift,
    bool updateMatId,
    Vec3B const& light,
    bool updateLight,
    MaterialColorVariant colorVariant)
  : position(position),
    foreground(foreground),
    liqId(EmptyLiquidId),
    matId(matId),
    hueShift(hueShift),
    updateMatId(updateMatId),
    colorVariant(colorVariant),
    light(light),
    updateLight(updateLight) {}

PreviewTile::PreviewTile(Vec2I const& position, LiquidId liqId)
  : position(position),
    foreground(true),
    liqId(liqId),
    matId(NullMaterialId),
    hueShift(0),
    updateMatId(false),
    colorVariant(DefaultMaterialColorVariant),
    updateLight(false) {}

OverheadBar::OverheadBar() : percentage(0.0f), detailOnly(false) {}

OverheadBar::OverheadBar(Json const& json) {
  entityPosition = json.opt("position").apply(jsonToVec2F).value();
  icon = json.optString("icon");
  percentage = json.getFloat("percentage");
  color = jsonToColor(json.get("color"));
  detailOnly = json.getBool("detailOnly", false);
}

OverheadBar::OverheadBar(Maybe<String> icon, float percentage, Color color, bool detailOnly)
  : icon(std::move(icon)), percentage(percentage), color(std::move(color)), detailOnly(detailOnly) {}

EnumMap<EntityHighlightEffectType> const EntityHighlightEffectTypeNames{
  {EntityHighlightEffectType::None, "none"},
  {EntityHighlightEffectType::Interactive, "interactive"},
  {EntityHighlightEffectType::Inspectable, "inspectable"},
  {EntityHighlightEffectType::Interesting, "interesting"},
  {EntityHighlightEffectType::Inspected, "inspected"}
};

}

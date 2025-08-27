#include "StarInspectionTool.hpp"
#include "StarJsonExtra.hpp"
#include "StarAssets.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarLiquidsDatabase.hpp"

namespace Star {

InspectionTool::InspectionTool(Json const& config, String const& directory, Json const& parameters)
  : Item(config, directory, parameters) {
  m_image = AssetPath::relativeTo(directory, instanceValue("image").toString());
  m_handPosition = jsonToVec2F(instanceValue("handPosition"));
  m_lightPosition = jsonToVec2F(instanceValue("lightPosition"));
  m_lightColor = jsonToColor(instanceValue("lightColor"));
  m_beamWidth = instanceValue("beamLevel").toFloat();
  m_ambientFactor = instanceValue("beamAmbience").toFloat();

  m_showHighlights = instanceValue("showHighlights").toBool();
  m_allowScanning = instanceValue("allowScanning").toBool();
  m_requireLineOfSight = instanceValue("requireLineOfSight", true) == Json(true);
  m_inspectionAngles = jsonToVec2F(instanceValue("inspectionAngles"));
  m_inspectionRanges = jsonToVec2F(instanceValue("inspectionRanges"));
  m_ambientInspectionRadius = instanceValue("ambientInspectionRadius").toFloat();
  m_fullInspectionSpaces = instanceValue("fullInspectionSpaces").toUInt();
  m_minimumInspectionLevel = instanceValue("minimumInspectionLevel").toFloat();
  if (auto typeFilter = instanceValue("inspectableTypeFilter"); typeFilter.isType(Json::Type::Array)) {
    m_inspectableTypeFilter.emplace();
    for (auto& str : typeFilter.toArray()) {
      if (!str.isType(Json::Type::String))
        continue;
      if (auto entityType = EntityTypeNames.leftPtr(str.toString()))
        m_inspectableTypeFilter->add(*entityType);
    }
  }

  m_lastFireMode = FireMode::None;
}

ItemPtr InspectionTool::clone() const {
  return make_shared<InspectionTool>(*this);
}

void InspectionTool::update(float, FireMode fireMode, bool, HashSet<MoveControlType> const&) {
  m_currentAngle = world()->geometry().diff(owner()->aimPosition(), owner()->position()).angle();
  m_currentPosition = owner()->position() + owner()->handPosition(hand(), m_lightPosition - m_handPosition);
  SpatialLogger::logPoint("world", m_currentPosition, {0, 0, 255, 255});

  if (fireMode != m_lastFireMode) {
    if (fireMode != FireMode::None)
      m_inspectionResults.append(inspect(owner()->aimPosition()));
  }

  m_lastFireMode = fireMode;
}

List<Drawable> InspectionTool::drawables() const {
  return {Drawable::makeImage(m_image, 1.0f / TilePixels, true, -m_handPosition)};
}

List<LightSource> InspectionTool::lightSources() const {
  if (!initialized())
    return {};

  float angle = world()->geometry().diff(owner()->aimPosition(), owner()->position()).angle();
  LightSource lightSource;
  lightSource.type = LightType::Point;
  lightSource.position = owner()->position() + owner()->handPosition(hand(), m_lightPosition - m_handPosition);
  lightSource.color = m_lightColor.toRgbF();
  lightSource.pointBeam = m_beamWidth;
  lightSource.beamAngle = angle;
  lightSource.beamAmbience = m_ambientFactor;
  return {std::move(lightSource)};
}

float InspectionTool::inspectionHighlightLevel(InspectableEntityPtr const& inspectable) const {
  if (m_showHighlights)
    return inspectionLevel(inspectable);
  return 0;
}

List<InspectionTool::InspectionResult> InspectionTool::pullInspectionResults() {
  return Star::take(m_inspectionResults);
}

float InspectionTool::inspectionLevel(InspectableEntityPtr const& inspectable) const {
  if (!initialized() || !inspectable->inspectable())
    return 0;

  if (m_inspectableTypeFilter && !m_inspectableTypeFilter->contains(inspectable->entityType()))
    return 0;

  if (auto tileEntity = as<TileEntity>(inspectable)) {
    float totalLevel = 0;

    // convert spaces to a set of world positions
    Set<Vec2I> spaceSet;
    for (auto space : tileEntity->spaces())
      spaceSet.add(tileEntity->tilePosition() + space);

    for (auto space : spaceSet) {
      float pointLevel = pointInspectionLevel(centerOfTile(space));
      if (pointLevel > 0 && hasLineOfSight(space, spaceSet))
        totalLevel += pointLevel;
    }
    return clamp(totalLevel / min(spaceSet.size(), m_fullInspectionSpaces), 0.0f, 1.0f);
  }
  else
    return pointInspectionLevel(inspectable->position());
}

float InspectionTool::pointInspectionLevel(Vec2F const& position) const {
  Vec2F gdiff = world()->geometry().diff(position, m_currentPosition);
  float gdist = gdiff.magnitude();
  float angleFactor = (abs(angleDiff(gdiff.angle(), m_currentAngle)) - m_inspectionAngles[0]) / (m_inspectionAngles[1] - m_inspectionAngles[0]);
  float distFactor = (gdist - m_inspectionRanges[0]) / (m_inspectionRanges[1] - m_inspectionRanges[0]);
  float ambientFactor = gdist / m_ambientInspectionRadius;
  return 1 - clamp(max(distFactor, min(ambientFactor, angleFactor)), 0.0f, 1.0f);
}

bool InspectionTool::hasLineOfSight(Vec2I const& position, Set<Vec2I> const& targetSpaces) const {
  if (!m_requireLineOfSight)
    return true;
  auto collisions = world()->collidingTilesAlongLine(centerOfTile(m_currentPosition), centerOfTile(position));
  for (auto collision : collisions) {
    if (collision != position && !targetSpaces.contains(collision))
      return false;
  }
  return true;
}

InspectionTool::InspectionResult InspectionTool::inspect(Vec2F const& position) {
  auto assets = Root::singleton().assets();
  auto species = owner()->species();

  // if there's a candidate InspectableEntity at the position, make sure that entity's total inspection level
  // is above the minimum threshold
  auto check = [&](InspectableEntityPtr entity) -> Maybe<InspectionTool::InspectionResult> {
    if (m_inspectableTypeFilter && !m_inspectableTypeFilter->contains(entity->entityType()))
      return {};
    if (entity->inspectable() && inspectionLevel(entity) >= m_minimumInspectionLevel) {
      if (m_allowScanning)
        return { { entity->inspectionDescription(species).value(), entity->inspectionLogName(), entity->entityId() } };
      else
        return { { entity->inspectionDescription(species).value(), {}, {} } };
    }
    return {};
  };


  WorldGeometry geometry = world()->geometry();
  for (auto& entity : world()->query<InspectableEntity>(RectF::withCenter(position, Vec2F()), [&](InspectableEntityPtr const& entity) {
    if (entity->entityType() == EntityType::Object)
      return false;
    else if (!geometry.rectContains(entity->metaBoundBox().translated(entity->position()), position))
      return false;
    else {
      auto hitPoly = entity->hitPoly();
      return hitPoly && geometry.polyContains(*hitPoly, position);
    }
  })) {
    if (auto result = check(entity))
      return *result;
  }

  for (auto& entity : world()->atTile<InspectableEntity>(Vec2I::floor(position))) {
    if (auto result = check(entity))
      return *result;
  }

  // check the inspection level at the selected tile
  if (!hasLineOfSight(Vec2I::floor(position)) || pointInspectionLevel(centerOfTile(position)) < m_minimumInspectionLevel)
    return {inspectionFailureText("outOfRangeText", species), {}};

  // check the tile for foreground mod or material
  MaterialId fgMaterial = world()->material(Vec2I::floor(position), TileLayer::Foreground);
  MaterialId fgMod = world()->mod(Vec2I(position.floor()), TileLayer::Foreground);
  auto materialDatabase = Root::singleton().materialDatabase();
  if (isRealMaterial(fgMaterial)) {
    if (isRealMod(fgMod))
      return {materialDatabase->modDescription(fgMod, species), {}};
    else
      return {materialDatabase->materialDescription(fgMaterial, species), {}};
  }

  // check for liquid at the tile
  auto liquidLevel = world()->liquidLevel(Vec2I::floor(position));
  auto liquidsDatabase = Root::singleton().liquidsDatabase();
  if (liquidLevel.liquid != EmptyLiquidId)
    return {liquidsDatabase->liquidDescription(liquidLevel.liquid, species), {}};

  // check the tile for background mod or material
  MaterialId bgMaterial = world()->material(Vec2I::floor(position), TileLayer::Background);
  MaterialId bgMod = world()->mod(Vec2I(position.floor()), TileLayer::Background);
  if (isRealMaterial(bgMaterial)) {
    if (isRealMod(bgMod))
      return {materialDatabase->modDescription(bgMod, species), {}};
    else
      return {materialDatabase->materialDescription(bgMaterial, species), {}};
  }

  // at this point you're just staring into the void
  return {inspectionFailureText("nothingThereText", species), {}};
}

String InspectionTool::inspectionFailureText(String const& failureType, String const& species) const {
  JsonArray textOptions;
  Json nothingThere = instanceValue(failureType);
  if (nothingThere.contains(species))
    textOptions = nothingThere.getArray(species);
  else
    textOptions = nothingThere.getArray("default");
  return textOptions.wrap(Random::randu64()).toString();
}

}

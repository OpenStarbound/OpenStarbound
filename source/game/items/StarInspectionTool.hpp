#ifndef STAR_INSPECTION_TOOL_HPP
#define STAR_INSPECTION_TOOL_HPP

#include "StarItem.hpp"
#include "StarPointableItem.hpp"
#include "StarToolUserItem.hpp"
#include "StarEntityRendering.hpp"
#include "StarInspectableEntity.hpp"

namespace Star {

STAR_CLASS(InspectionTool);

class InspectionTool
  : public Item,
    public PointableItem,
    public ToolUserItem {
public:

  struct InspectionResult {
    String message;
    Maybe<String> objectName = {};
    Maybe<EntityId> entityId = {};
  };

  InspectionTool(Json const& config, String const& directory, Json const& parameters = JsonObject());

  ItemPtr clone() const override;

  void update(FireMode fireMode, bool shifting, HashSet<MoveControlType> const& moves) override;

  List<Drawable> drawables() const override;

  List<LightSource> lightSources() const;

  float inspectionHighlightLevel(InspectableEntityPtr const& inspectableEntity) const;

  List<InspectionResult> pullInspectionResults();

private:
  InspectionResult inspect(Vec2F const& position);

  float inspectionLevel(InspectableEntityPtr const& inspectableEntity) const;
  float pointInspectionLevel(Vec2F const& position) const;
  bool hasLineOfSight(Vec2I const& targetPosition, Set<Vec2I> const& targetSpaces = {}) const;

  String inspectionFailureText(String const& failureType, String const& species) const;

  float m_currentAngle;
  Vec2F m_currentPosition;

  String m_image;
  Vec2F m_handPosition;
  Vec2F m_lightPosition;
  Color m_lightColor;
  float m_beamWidth;
  float m_ambientFactor;

  bool m_showHighlights;
  bool m_allowScanning;

  Vec2F m_inspectionAngles;
  Vec2F m_inspectionRanges;
  float m_ambientInspectionRadius;
  size_t m_fullInspectionSpaces;
  float m_minimumInspectionLevel;

  FireMode m_lastFireMode;
  List<InspectionResult> m_inspectionResults;
};

}

#endif

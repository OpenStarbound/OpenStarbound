#include "StarDamage.hpp"
#include "StarJsonExtra.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarRoot.hpp"

namespace Star {

DamageSource::DamageSource()
  : damageType(DamageType::NoDamage), damage(0.0), sourceEntityId(NullEntityId), rayCheck(false) {}

DamageSource::DamageSource(Json const& config) {
  if (auto dtype = config.optString("damageType"))
    damageType = DamageTypeNames.getLeft(*dtype);
  else
    damageType = DamageType::Damage;

  if (config.contains("poly"))
    damageArea = jsonToPolyF(config.get("poly"));
  else if (config.contains("line"))
    damageArea = jsonToLine2F(config.get("line"));
  else
    throw StarException("No 'poly' or 'line' key in DamageSource config");

  damage = config.getFloat("damage");

  trackSourceEntity = config.getBool("trackSourceEntity", true);
  sourceEntityId = config.getInt("sourceEntityId", NullEntityId);

  if (auto tconfig = config.opt("team")) {
    team = EntityDamageTeam(*tconfig);
  } else {
    team.type = TeamTypeNames.getLeft(config.getString("teamType", "passive"));
    team.team = config.getUInt("teamNumber", 0);
  }

  damageRepeatGroup = config.optString("damageRepeatGroup");
  damageRepeatTimeout = config.optFloat("damageRepeatTimeout");

  damageSourceKind = config.getString("damageSourceKind", "");

  statusEffects = config.getArray("statusEffects", {}).transformed(jsonToEphemeralStatusEffect);

  auto knockbackJson = config.get("knockback", Json(0.0f));
  if (knockbackJson.isType(Json::Type::Array))
    knockback = jsonToVec2F(knockbackJson);
  else
    knockback = knockbackJson.toFloat();

  rayCheck = config.getBool("rayCheck", false);
}

DamageSource::DamageSource(DamageType damageType,
    DamageArea damageArea,
    float damage,
    bool trackSourceEntity,
    EntityId sourceEntityId,
    EntityDamageTeam team,
    Maybe<String> damageRepeatGroup,
    Maybe<float> damageRepeatTimeout,
    String damageSourceKind,
    List<EphemeralStatusEffect> statusEffects,
    Knockback knockback,
    bool rayCheck)
  : damageType(move(damageType)),
    damageArea(move(damageArea)),
    damage(move(damage)),
    trackSourceEntity(move(trackSourceEntity)),
    sourceEntityId(move(sourceEntityId)),
    team(move(team)),
    damageRepeatGroup(move(damageRepeatGroup)),
    damageRepeatTimeout(move(damageRepeatTimeout)),
    damageSourceKind(move(damageSourceKind)),
    statusEffects(move(statusEffects)),
    knockback(move(knockback)),
    rayCheck(move(rayCheck)) {}

Json DamageSource::toJson() const {
  Json damageAreaJson;
  if (auto p = damageArea.ptr<PolyF>())
    damageAreaJson = jsonFromPolyF(*p);
  else if (auto l = damageArea.ptr<Line2F>())
    damageAreaJson = jsonFromLine2F(*l);

  Json knockbackJson;
  if (auto p = knockback.ptr<float>())
    knockbackJson = *p;
  else if (auto p = knockback.ptr<Vec2F>())
    knockbackJson = jsonFromVec2F(*p);

  return JsonObject{{"damageType", DamageTypeNames.getRight(damageType)},
      {"damageArea", damageAreaJson},
      {"damage", damage},
      {"trackSourceEntity", trackSourceEntity},
      {"sourceEntityId", sourceEntityId},
      {"team", team.toJson()},
      {"damageRepeatGroup", jsonFromMaybe(damageRepeatGroup)},
      {"damageRepeatTimeout", jsonFromMaybe(damageRepeatTimeout)},
      {"damageSourceKind", damageSourceKind},
      {"statusEffects", statusEffects.transformed(jsonFromEphemeralStatusEffect)},
      {"knockback", knockbackJson},
      {"rayCheck", rayCheck}};
}

bool DamageSource::intersectsWithPoly(WorldGeometry const& geometry, PolyF const& targetPoly) const {
  if (auto poly = damageArea.ptr<PolyF>())
    return geometry.polyIntersectsPoly(*poly, targetPoly);
  else if (auto line = damageArea.ptr<Line2F>())
    return geometry.lineIntersectsPoly(*line, targetPoly);
  else
    return false;
}

Vec2F DamageSource::knockbackMomentum(WorldGeometry const& worldGeometry, Vec2F const& targetCenter) const {
  if (auto v = knockback.ptr<Vec2F>()) {
    return *v;
  } else if (auto s = knockback.ptr<float>()) {
    if (*s != 0) {
      if (auto poly = damageArea.ptr<PolyF>())
        return worldGeometry.diff(targetCenter, poly->center()).normalized() * *s;
      else if (auto line = damageArea.ptr<Line2F>())
        return vnorm(line->diff()) * *s;
    }
  }

  return Vec2F();
}

bool DamageSource::operator==(DamageSource const& rhs) const {
  return tie(damageType, damageArea, damage, trackSourceEntity, sourceEntityId, team, damageSourceKind, statusEffects, knockback, rayCheck)
      == tie(rhs.damageType,
             rhs.damageArea,
             rhs.damage,
             rhs.trackSourceEntity,
             rhs.sourceEntityId,
             rhs.team,
             rhs.damageSourceKind,
             rhs.statusEffects,
             rhs.knockback,
             rhs.rayCheck);
}

DamageSource& DamageSource::translate(Vec2F const& position) {
  if (auto poly = damageArea.ptr<PolyF>())
    poly->translate(position);
  else if (auto line = damageArea.ptr<Line2F>())
    line->translate(position);
  return *this;
}

DataStream& operator<<(DataStream& ds, DamageSource const& damageSource) {
  ds.write(damageSource.damageType);
  ds.write(damageSource.damageArea);
  ds.write(damageSource.damage);
  ds.write(damageSource.trackSourceEntity);
  ds.write(damageSource.sourceEntityId);
  ds.write(damageSource.team);
  ds.write(damageSource.damageRepeatGroup);
  ds.write(damageSource.damageRepeatTimeout);
  ds.write(damageSource.damageSourceKind);
  ds.write(damageSource.statusEffects);
  ds.write(damageSource.knockback);
  ds.write(damageSource.rayCheck);
  return ds;
}

DataStream& operator>>(DataStream& ds, DamageSource& damageSource) {
  ds.read(damageSource.damageType);
  ds.read(damageSource.damageArea);
  ds.read(damageSource.damage);
  ds.read(damageSource.trackSourceEntity);
  ds.read(damageSource.sourceEntityId);
  ds.read(damageSource.team);
  ds.read(damageSource.damageRepeatGroup);
  ds.read(damageSource.damageRepeatTimeout);
  ds.read(damageSource.damageSourceKind);
  ds.read(damageSource.statusEffects);
  ds.read(damageSource.knockback);
  ds.read(damageSource.rayCheck);
  return ds;
}

DamageRequest::DamageRequest()
  : hitType(HitType::Hit), damageType(DamageType::Damage), damage(0.0f), sourceEntityId(NullEntityId) {}

DamageRequest::DamageRequest(Json const& v) {
  hitType = HitTypeNames.getLeft(v.getString("hitType", "hit"));
  damageType = DamageTypeNames.getLeft(v.getString("damageType", "damage"));
  damage = v.getFloat("damage");
  knockbackMomentum = jsonToVec2F(v.get("knockbackMomentum", JsonArray{0, 0}));
  sourceEntityId = v.getInt("sourceEntityId", NullEntityId);
  damageSourceKind = v.getString("damageSourceKind", "");
  statusEffects = v.getArray("statusEffects", {}).transformed(jsonToEphemeralStatusEffect);
}

DamageRequest::DamageRequest(HitType hitType,
    DamageType damageType,
    float damage,
    Vec2F const& knockbackMomentum,
    EntityId sourceEntityId,
    String const& damageSourceKind,
    List<EphemeralStatusEffect> const& statusEffects)
  : hitType(hitType),
    damageType(damageType),
    damage(damage),
    knockbackMomentum(knockbackMomentum),
    sourceEntityId(sourceEntityId),
    damageSourceKind(damageSourceKind),
    statusEffects(statusEffects) {}

Json DamageRequest::toJson() const {
  return JsonObject{{"hitType", HitTypeNames.getRight(hitType)},
      {"damageType", DamageTypeNames.getRight(damageType)},
      {"damage", damage},
      {"knockbackMomentum", jsonFromVec2F(knockbackMomentum)},
      {"sourceEntityId", sourceEntityId},
      {"damageSourceKind", damageSourceKind},
      {"statusEffects", statusEffects.transformed(jsonFromEphemeralStatusEffect)}};
}

DataStream& operator<<(DataStream& ds, DamageRequest const& damageRequest) {
  ds << damageRequest.hitType;
  ds << damageRequest.damageType;
  ds << damageRequest.damage;
  ds << damageRequest.knockbackMomentum;
  ds << damageRequest.sourceEntityId;
  ds << damageRequest.damageSourceKind;
  ds << damageRequest.statusEffects;
  return ds;
}

DataStream& operator>>(DataStream& ds, DamageRequest& damageRequest) {
  ds >> damageRequest.hitType;
  ds >> damageRequest.damageType;
  ds >> damageRequest.damage;
  ds >> damageRequest.knockbackMomentum;
  ds >> damageRequest.sourceEntityId;
  ds >> damageRequest.damageSourceKind;
  ds >> damageRequest.statusEffects;
  return ds;
}

DamageNotification::DamageNotification() : sourceEntityId(), targetEntityId(), damageDealt(), healthLost() {}

DamageNotification::DamageNotification(Json const& v) {
  sourceEntityId = v.getInt("sourceEntityId");
  targetEntityId = v.getInt("targetEntityId");
  position = jsonToVec2F(v.get("position"));
  damageDealt = v.getFloat("damageDealt");
  healthLost = v.getFloat("healthLost");
  hitType = HitTypeNames.getLeft(v.getString("hitType"));
  damageSourceKind = v.getString("damageSourceKind");
  targetMaterialKind = v.getString("targetMaterialKind");
}

DamageNotification::DamageNotification(EntityId sourceEntityId,
    EntityId targetEntityId,
    Vec2F position,
    float damageDealt,
    float healthLost,
    HitType hitType,
    String damageSourceKind,
    String targetMaterialKind)
  : sourceEntityId(sourceEntityId),
    targetEntityId(targetEntityId),
    position(position),
    damageDealt(damageDealt),
    healthLost(healthLost),
    hitType(hitType),
    damageSourceKind(move(damageSourceKind)),
    targetMaterialKind(move(targetMaterialKind)) {}

Json DamageNotification::toJson() const {
  return JsonObject{{"sourceEntityId", sourceEntityId},
      {"targetEntityId", targetEntityId},
      {"position", jsonFromVec2F(position)},
      {"damageDealt", damageDealt},
      {"healthLost", healthLost},
      {"hitType", HitTypeNames.getRight(hitType)},
      {"damageSourceKind", damageSourceKind},
      {"targetMaterialKind", targetMaterialKind}};
}

DataStream& operator<<(DataStream& ds, DamageNotification const& damageNotification) {
  ds.viwrite(damageNotification.sourceEntityId);
  ds.viwrite(damageNotification.targetEntityId);
  ds.vfwrite(damageNotification.position[0], 0.01f);
  ds.vfwrite(damageNotification.position[1], 0.01f);
  ds.write(damageNotification.damageDealt);
  ds.write(damageNotification.healthLost);
  ds.write(damageNotification.hitType);
  ds.write(damageNotification.damageSourceKind);
  ds.write(damageNotification.targetMaterialKind);

  return ds;
}

DataStream& operator>>(DataStream& ds, DamageNotification& damageNotification) {
  ds.viread(damageNotification.sourceEntityId);
  ds.viread(damageNotification.targetEntityId);
  ds.vfread(damageNotification.position[0], 0.01f);
  ds.vfread(damageNotification.position[1], 0.01f);
  ds.read(damageNotification.damageDealt);
  ds.read(damageNotification.healthLost);
  ds.read(damageNotification.hitType);
  ds.read(damageNotification.damageSourceKind);
  ds.read(damageNotification.targetMaterialKind);

  return ds;
}

}

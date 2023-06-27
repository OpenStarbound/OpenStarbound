#include "StarForceRegions.hpp"
#include "StarJsonExtra.hpp"
#include "StarDataStreamExtra.hpp"

namespace Star {

PhysicsCategoryFilter PhysicsCategoryFilter::whitelist(StringSet categories) {
  return PhysicsCategoryFilter(Whitelist, move(categories));
}

PhysicsCategoryFilter PhysicsCategoryFilter::blacklist(StringSet categories) {
  return PhysicsCategoryFilter(Blacklist, move(categories));
}

PhysicsCategoryFilter::PhysicsCategoryFilter(Type type, StringSet categories)
  : type(type), categories(move(categories)) {}

bool PhysicsCategoryFilter::check(StringSet const& otherCategories) const {
  bool intersection = categories.hasIntersection(otherCategories);
  if (type == Whitelist)
    return intersection;
  else
    return !intersection;
}

bool PhysicsCategoryFilter::operator==(PhysicsCategoryFilter const& rhs) const {
  return tie(type, categories) == tie(rhs.type, rhs.categories);
}

DataStream& operator>>(DataStream& ds, PhysicsCategoryFilter& pcf) {
  ds >> pcf.type;
  ds >> pcf.categories;
  return ds;
}

DataStream& operator<<(DataStream& ds, PhysicsCategoryFilter const& pcf) {
  ds << pcf.type;
  ds << pcf.categories;
  return ds;
}

PhysicsCategoryFilter jsonToPhysicsCategoryFilter(Json const& json) {
  auto whitelist = json.opt("categoryWhitelist");
  auto blacklist = json.opt("categoryBlacklist");
  if (whitelist && blacklist)
    throw JsonException::format("Cannot specify both a physics category whitelist and blacklist");
  if (whitelist)
    return PhysicsCategoryFilter::whitelist(jsonToStringSet(*whitelist));
  if (blacklist)
    return PhysicsCategoryFilter::blacklist(jsonToStringSet(*blacklist));
  return PhysicsCategoryFilter();
}

DirectionalForceRegion DirectionalForceRegion::fromJson(Json const& json) {
  DirectionalForceRegion dfr;
  if (json.contains("polyRegion"))
    dfr.region = jsonToPolyF(json.get("polyRegion"));
  else
    dfr.region = PolyF(jsonToRectF(json.get("rectRegion")));
  dfr.xTargetVelocity = json.optFloat("xTargetVelocity");
  dfr.yTargetVelocity = json.optFloat("yTargetVelocity");
  dfr.controlForce = json.getFloat("controlForce");
  dfr.categoryFilter = jsonToPhysicsCategoryFilter(json);
  return dfr;
}

RectF DirectionalForceRegion::boundBox() const {
  return region.boundBox();
}

void DirectionalForceRegion::translate(Vec2F const& pos) {
  region.translate(pos);
}

bool DirectionalForceRegion::operator==(DirectionalForceRegion const& rhs) const {
  return tie(region, xTargetVelocity, yTargetVelocity, controlForce, categoryFilter)
      == tie(rhs.region, rhs.xTargetVelocity, rhs.yTargetVelocity, rhs.controlForce, rhs.categoryFilter);
}

DataStream& operator>>(DataStream& ds, DirectionalForceRegion& dfr) {
  ds >> dfr.region;
  ds >> dfr.xTargetVelocity;
  ds >> dfr.yTargetVelocity;
  ds >> dfr.controlForce;
  ds >> dfr.categoryFilter;
  return ds;
}

DataStream& operator<<(DataStream& ds, DirectionalForceRegion const& dfr) {
  ds << dfr.region;
  ds << dfr.xTargetVelocity;
  ds << dfr.yTargetVelocity;
  ds << dfr.controlForce;
  ds << dfr.categoryFilter;
  return ds;
}

RadialForceRegion RadialForceRegion::fromJson(Json const& json) {
  RadialForceRegion rfr;
  rfr.center = json.opt("center").apply(jsonToVec2F).value();
  rfr.outerRadius = json.getFloat("outerRadius");
  rfr.innerRadius = json.getFloat("innerRadius");
  rfr.targetRadialVelocity = json.getFloat("targetRadialVelocity");
  rfr.controlForce = json.getFloat("controlForce");
  rfr.categoryFilter = jsonToPhysicsCategoryFilter(json);
  return rfr;
}

RectF RadialForceRegion::boundBox() const {
  return RectF::withCenter(center, Vec2F::filled(outerRadius));
}

void RadialForceRegion::translate(Vec2F const& pos) {
  center += pos;
}

bool RadialForceRegion::operator==(RadialForceRegion const& rhs) const {
  return tie(center, outerRadius, innerRadius, targetRadialVelocity, controlForce, categoryFilter)
      == tie(rhs.center, rhs.outerRadius, rhs.innerRadius, rhs.targetRadialVelocity, rhs.controlForce, rhs.categoryFilter);
}

DataStream& operator>>(DataStream& ds, RadialForceRegion& rfr) {
  ds >> rfr.center;
  ds >> rfr.outerRadius;
  ds >> rfr.innerRadius;
  ds >> rfr.targetRadialVelocity;
  ds >> rfr.controlForce;
  ds >> rfr.categoryFilter;
  return ds;
}

DataStream& operator<<(DataStream& ds, RadialForceRegion const& gfr) {
  ds << gfr.center;
  ds << gfr.outerRadius;
  ds << gfr.innerRadius;
  ds << gfr.targetRadialVelocity;
  ds << gfr.controlForce;
  ds << gfr.categoryFilter;
  return ds;
}

GradientForceRegion GradientForceRegion::fromJson(Json const& json) {
  GradientForceRegion gfr;
  if (json.contains("polyRegion"))
    gfr.region = jsonToPolyF(json.get("polyRegion"));
  else
    gfr.region = PolyF(jsonToRectF(json.get("rectRegion")));
  gfr.gradient = jsonToLine2F(json.get("gradient"));
  gfr.baseTargetVelocity = json.getFloat("baseTargetVelocity");
  gfr.baseControlForce = json.getFloat("baseControlForce");
  gfr.categoryFilter = jsonToPhysicsCategoryFilter(json);
  return gfr;
}

RectF GradientForceRegion::boundBox() const {
  return region.boundBox();
}

void GradientForceRegion::translate(Vec2F const& pos) {
  region.translate(pos);
  gradient.translate(pos);
}

bool GradientForceRegion::operator==(GradientForceRegion const& rhs) const {
  return tie(region, gradient, baseTargetVelocity, baseControlForce, categoryFilter)
      == tie(rhs.region, rhs.gradient, rhs.baseTargetVelocity, rhs.baseControlForce, rhs.categoryFilter);
}

DataStream& operator>>(DataStream& ds, GradientForceRegion& gfr) {
  ds >> gfr.region;
  ds >> gfr.gradient;
  ds >> gfr.baseTargetVelocity;
  ds >> gfr.baseControlForce;
  ds >> gfr.categoryFilter;
  return ds;
}

DataStream& operator<<(DataStream& ds, GradientForceRegion const& gfr) {
  ds << gfr.region;
  ds << gfr.gradient;
  ds << gfr.baseTargetVelocity;
  ds << gfr.baseControlForce;
  ds << gfr.categoryFilter;
  return ds;
}

PhysicsForceRegion jsonToPhysicsForceRegion(Json const& json) {
  String type = json.getString("type");
  if (type.equalsIgnoreCase("DirectionalForceRegion"))
    return DirectionalForceRegion::fromJson(json);
  else if (type.equalsIgnoreCase("RadialForceRegion"))
    return RadialForceRegion::fromJson(json);
  else if (type.equalsIgnoreCase("GradientForceRegion"))
    return GradientForceRegion::fromJson(json);
  else
    throw JsonException::format("No such physics force region type '{}'", type);
}

}

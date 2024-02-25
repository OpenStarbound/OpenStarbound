#pragma once

#include "StarPoly.hpp"
#include "StarVariant.hpp"
#include "StarJson.hpp"

namespace Star {

struct PhysicsCategoryFilter {
  enum Type { Whitelist, Blacklist };

  static PhysicsCategoryFilter whitelist(StringSet categories);
  static PhysicsCategoryFilter blacklist(StringSet categories);

  PhysicsCategoryFilter(Type type = Blacklist, StringSet categories = {});

  bool check(StringSet const& categories) const;

  bool operator==(PhysicsCategoryFilter const& rhs) const;

  Type type;
  StringSet categories;
};

DataStream& operator>>(DataStream& ds, PhysicsCategoryFilter& rfr);
DataStream& operator<<(DataStream& ds, PhysicsCategoryFilter const& rfr);

PhysicsCategoryFilter jsonToPhysicsCategoryFilter(Json const& json);

struct DirectionalForceRegion {
  static DirectionalForceRegion fromJson(Json const& json);

  RectF boundBox() const;

  void translate(Vec2F const& pos);

  bool operator==(DirectionalForceRegion const& rhs) const;

  PolyF region;
  Maybe<float> xTargetVelocity;
  Maybe<float> yTargetVelocity;
  float controlForce;
  PhysicsCategoryFilter categoryFilter;
};

DataStream& operator>>(DataStream& ds, DirectionalForceRegion& rfr);
DataStream& operator<<(DataStream& ds, DirectionalForceRegion const& rfr);

struct RadialForceRegion {
  static RadialForceRegion fromJson(Json const& json);

  RectF boundBox() const;

  void translate(Vec2F const& pos);

  bool operator==(RadialForceRegion const& rhs) const;

  Vec2F center;
  float outerRadius;
  float innerRadius;
  float targetRadialVelocity;
  float controlForce;
  PhysicsCategoryFilter categoryFilter;
};

DataStream& operator>>(DataStream& ds, RadialForceRegion& rfr);
DataStream& operator<<(DataStream& ds, RadialForceRegion const& rfr);

struct GradientForceRegion {
  static GradientForceRegion fromJson(Json const& json);

  RectF boundBox() const;

  void translate(Vec2F const& pos);

  bool operator==(GradientForceRegion const& rhs) const;

  PolyF region;
  Line2F gradient;
  float baseTargetVelocity;
  float baseControlForce;
  PhysicsCategoryFilter categoryFilter;
};

DataStream& operator>>(DataStream& ds, GradientForceRegion& rfr);
DataStream& operator<<(DataStream& ds, GradientForceRegion const& rfr);

typedef Variant<DirectionalForceRegion, RadialForceRegion, GradientForceRegion> PhysicsForceRegion;

PhysicsForceRegion jsonToPhysicsForceRegion(Json const& json);

}

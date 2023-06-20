#ifndef STAR_DANCE_DATABASE_HPP
#define STAR_DANCE_DATABASE_HPP

#include "StarRoot.hpp"
#include "StarAssets.hpp"

namespace Star {

STAR_STRUCT(DanceStep);
STAR_STRUCT(Dance);
STAR_CLASS(DanceDatabase);

struct DanceStep {
  Maybe<String> bodyFrame;
  Maybe<String> frontArmFrame;
  Maybe<String> backArmFrame;
  Vec2F headOffset;
  Vec2F frontArmOffset;
  Vec2F backArmOffset;
  float frontArmRotation;
  float backArmRotation;
};

struct Dance {
  String name;
  List<String> states;
  float cycle;
  bool cyclic;
  float duration;
  List<DanceStep> steps;
};

class DanceDatabase {
public:
  DanceDatabase();

  DancePtr getDance(String const& name) const;

private:
  static DancePtr readDance(String const& path);

  StringMap<DancePtr> m_dances;
};

}

#endif

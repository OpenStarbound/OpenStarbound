#ifndef STAR_INSPECTABLE_ENTITY_HPP
#define STAR_INSPECTABLE_ENTITY_HPP

#include "StarEntity.hpp"

namespace Star {

STAR_CLASS(InspectableEntity);

class InspectableEntity : public virtual Entity {
public:
  // Default implementation returns true
  virtual bool inspectable() const;

  // If this entity can be entered into the player log, will return the log
  // identifier.
  virtual Maybe<String> inspectionLogName() const;

  // Long description to display when inspected, if any
  virtual Maybe<String> inspectionDescription(String const& species) const;
};

inline bool InspectableEntity::inspectable() const {
  return true;
}

inline Maybe<String> InspectableEntity::inspectionLogName() const {
  return {};
}

inline Maybe<String> InspectableEntity::inspectionDescription(String const&) const {
  return {};
}

}

#endif

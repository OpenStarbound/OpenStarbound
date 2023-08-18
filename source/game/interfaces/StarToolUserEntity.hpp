#ifndef STAR_TOOL_USER_ENTITY_HPP
#define STAR_TOOL_USER_ENTITY_HPP

#include "StarEntity.hpp"
#include "StarParticle.hpp"
#include "StarStatusTypes.hpp"
#include "StarInteractionTypes.hpp"

namespace Star {

STAR_CLASS(Item);
STAR_CLASS(ToolUserEntity);
STAR_CLASS(ActorMovementController);
STAR_CLASS(StatusController);

// FIXME: This interface is a complete mess.
class ToolUserEntity : public virtual Entity {
public:
  // Translates the given arm position into it's final entity space position
  // based on the given facing direction, and arm angle, and an offset from the
  // rotation center of the arm.
  virtual Vec2F armPosition(ToolHand hand, Direction facingDirection, float armAngle, Vec2F offset = {}) const = 0;
  // The offset to give to armPosition to get the position of the hand.
  virtual Vec2F handOffset(ToolHand hand, Direction facingDirection) const = 0;

  // Gets the world position of the current aim point.
  virtual Vec2F aimPosition() const = 0;

  virtual bool isAdmin() const = 0;
  virtual Vec4B favoriteColor() const = 0;
  virtual String species() const = 0;

  virtual void requestEmote(String const& emote) = 0;

  virtual ActorMovementController* movementController() = 0;
  virtual StatusController* statusController() = 0;

  // FIXME: This is effectively unusable, because since tool user items control
  // the angle and facing direction of the owner, and this uses the facing
  // direction and angle as input, the result will always be behind.
  virtual Vec2F handPosition(ToolHand hand, Vec2F const& handOffset = Vec2F()) const = 0;

  // FIXME: This was used for an Item to get an ItemPtr to itself, which was
  // super bad and weird, but it COULD be used to get the item in the owner's
  // other hand, which is LESS bad.
  virtual ItemPtr handItem(ToolHand hand) const = 0;

  // FIXME: What is the difference between interactRadius (which defines a tool
  // range) and inToolRange (which also defines a tool range indirectly).
  // inToolRange() implements based on the center of the tile of the aim
  // position (NOT the aim position!) but inToolRange(Vec2F) uses the given
  // position, which is again redundant.  Also, what is beamGunRadius and why
  // is it different than interact radius?  Can different tools have a
  // different interact radius?
  virtual float interactRadius() const = 0;
  virtual bool inToolRange() const = 0;
  virtual bool inToolRange(Vec2F const& position) const = 0;
  virtual float beamGunRadius() const = 0;

  // FIXME: Too specific to Player, just cast to Player if you have to and do
  // that, NPCs cannot possibly implement these properly (and do not implement
  // them at all).
  virtual void queueUIMessage(String const& message) = 0;
  virtual void interact(InteractAction const& action) = 0;

  // FIXME: Ditto here, instrumentPlaying() is just an accessor to the songbook
  // for when the songbook has had a song selected, and the instrument decides
  // when to cancel music anyway, also instrumentEquipped(String) is a straight
  // up ridiculous way of notifying the Player that the player itself is
  // holding an instrument, which it already knows.
  virtual bool instrumentPlaying() = 0;
  virtual void instrumentEquipped(String const& instrumentKind) = 0;

  // FIXME: how is this related to the hand position and isn't it already
  // included in the hand position and why is it necessary?
  virtual Vec2F armAdjustment() const = 0;

  // FIXME: These were all fine, just need to be fixed because now we have the
  // movement controller itself and can use that directly
  virtual Vec2F position() const = 0;
  virtual Vec2F velocity() const = 0;
  virtual Direction facingDirection() const = 0;
  virtual Direction walkingDirection() const = 0;

  // FIXME: Ditto here, except we now have the status controller directly.
  virtual float powerMultiplier() const = 0;
  virtual bool fullEnergy() const = 0;
  virtual float energy() const = 0;
  virtual bool consumeEnergy(float energy) = 0;
  virtual bool energyLocked() const = 0;
  virtual void addEphemeralStatusEffects(List<EphemeralStatusEffect> const& statusEffects) = 0;
  virtual ActiveUniqueStatusEffectSummary activeUniqueStatusEffectSummary() const = 0;

  // FIXME: This is a dumb way of getting limited animation support
  virtual void addEffectEmitters(StringSet const& emitters) = 0;
  virtual void addParticles(List<Particle> const& particles) = 0;
  virtual void addSound(String const& sound, float volume = 1.0f, float pitch = 1.0f) = 0;

  virtual void setCameraFocusEntity(Maybe<EntityId> const& cameraFocusEntity) = 0;
};

}

#endif

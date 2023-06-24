#ifndef STAR_PARTICLE_HPP
#define STAR_PARTICLE_HPP

#include "StarJson.hpp"
#include "StarColor.hpp"
#include "StarBiMap.hpp"
#include "StarAnimation.hpp"
#include "StarAssetPath.hpp"

namespace Star {

struct Particle {
  enum class Type {
    // Variance is basically a null type, used only for varying other particles
    // by amounts.
    Variance,

    Ember,
    Textured,
    Animated,
    Streak,
    Text
  };
  static EnumMap<Type> const TypeNames;

  enum class DestructionAction {
    None,
    Image,
    Fade,
    Shrink
  };
  static EnumMap<DestructionAction> const DestructionActionNames;

  enum class Layer {
    Back,
    Middle,
    Front
  };
  static EnumMap<Layer> const LayerNames;

  Particle();
  // If particle is type Textured, then the image name is considered relative
  // to the given asset path
  explicit Particle(Json const& config, String const& assetsPath = "/");

  Json toJson() const;

  void translate(Vec2F const& pos);

  // Updates position, velocity, rotation, and timeToLive.
  void update(float dt, Vec2F const& wind = Vec2F());

  bool dead() const;

  // Apply random variance to this particle based on a "variance" particle that
  // contains the maximum amount of variance for each field.
  void applyVariance(Particle const& variance);

  // Stops particle and sets time to live to 0.0 (triggering destruction)
  void collide(Vec2F const& collisionPosition);
  // Immediately triggers destruction of particle with / without destruction
  // action
  void destroy(bool withDestruction);

  // Internally called by update() / collide() / destruct()
  void destructionUpdate();

  void initializeAnimation();

  Type type;

  // Defaults to 1.0, 1.0 will produce a reasonable size particle for whatever
  // the type is.
  float size;
  float baseSize; // track the original size for shrink destruction action

  // Used differently depending on the type of the particle.
  String string;
  AssetPath image;
  Directives directives;

  Color color;
  Color light;
  float fade;
  bool fullbright;

  Vec2F position;
  Vec2F velocity;
  Vec2F finalVelocity;
  Vec2F approach;

  bool flippable;
  bool flip;

  float rotation;
  float angularVelocity;

  float length;

  DestructionAction destructionAction;
  AssetPath destructionImage;
  float destructionTime;

  float timeToLive;
  Layer layer;

  bool collidesForeground;
  bool collidesLiquid;
  bool underwaterOnly;

  bool ignoreWind;

  bool trail;

  Maybe<Animation> animation;
};

DataStream& operator<<(DataStream& ds, Particle const& particle);
DataStream& operator>>(DataStream& ds, Particle& particle);

typedef function<Particle()> ParticleVariantCreator;
ParticleVariantCreator makeParticleVariantCreator(Particle particle, Particle variance);

}

#endif

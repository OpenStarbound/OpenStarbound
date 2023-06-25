#include "StarParticle.hpp"
#include "StarAssets.hpp"
#include "StarAnimation.hpp"
#include "StarJsonExtra.hpp"
#include "StarDataStreamExtra.hpp"
#include "StarRandom.hpp"

namespace Star {

EnumMap<Particle::Type> const Particle::TypeNames{{Particle::Type::Variance, "variance"},
    {Particle::Type::Ember, "ember"},
    {Particle::Type::Textured, "textured"},
    {Particle::Type::Animated, "animated"},
    {Particle::Type::Streak, "streak"},
    {Particle::Type::Text, "text"}};

EnumMap<Particle::DestructionAction> const Particle::DestructionActionNames{{Particle::DestructionAction::None, "none"},
    {Particle::DestructionAction::Image, "image"},
    {Particle::DestructionAction::Fade, "fade"},
    {Particle::DestructionAction::Shrink, "shrink"}};

EnumMap<Particle::Layer> const Particle::LayerNames{
    {Particle::Layer::Back, "back"}, {Particle::Layer::Middle, "middle"}, {Particle::Layer::Front, "front"}};

Particle::Particle() {
  type = Type::Variance;
  size = 0;
  baseSize = 0;
  color = Color::White;
  light = Color::Clear;
  fade = 0;
  fullbright = false;
  position = velocity = finalVelocity = approach = Vec2F();
  rotation = 0.0f;
  angularVelocity = 0.0f;
  timeToLive = 0.0f;
  layer = Layer::Middle;
  collidesForeground = true;
  collidesLiquid = true;
  underwaterOnly = false;
  ignoreWind = true;
  length = 0;
  destructionAction = DestructionAction::None;
  destructionTime = 0.0f;
  destructionSet = false;
  trail = false;
  flippable = true;
  flip = false;
}

Particle::Particle(Json const& config, String const& path) {
  type = TypeNames.getLeft(config.getString("type", "variance"));
  if (type == Type::Variance) {
    size = 0.0f;
    color = Color::Clear;
  } else {
    size = 1.0f;
    color = Color::White;
  }

  size = config.getFloat("size", size);
  baseSize = size;

  string = config.getString(
      "image", config.getString("text", config.getString("animation", config.getString("string", ""))));
  if (type == Type::Textured || type == Type::Animated)
    string = AssetPath::relativeTo(path, string);

  if (type == Type::Animated)
    initializeAnimation();

  auto pathEnd = string.find('?');
  if (pathEnd == NPos)
    directives = "";
  else
    directives.parse(string.substr(pathEnd));

  if (config.contains("color"))
    color = jsonToColor(config.get("color"));

  light = jsonToColor(config.get("light", JsonArray{0, 0, 0, 0}));

  fade = config.getFloat("fade", 0.0f);
  fullbright = config.getBool("fullbright", false);

  position = jsonToVec2F(config.get("position", JsonArray{0.0f, 0.0f}));
  velocity = jsonToVec2F(config.get("initialVelocity", config.get("velocity", JsonArray{0.0f, 0.0f})));

  if (type == Type::Variance)
    finalVelocity = {0.0f, 0.0f};
  else
    finalVelocity = velocity;
  finalVelocity = jsonToVec2F(config.get("finalVelocity", JsonArray::from(finalVelocity)));

  approach = jsonToVec2F(config.get("approach", JsonArray{0.0f, 0.0f}));

  flip = config.getBool("flip", false);
  flippable = config.getBool("flippable", true);

  rotation = config.getFloat("rotation", 0.0f) * Constants::pi / 180.0f;
  angularVelocity = config.getFloat("angularVelocity", 0.0f) * Constants::pi / 180.0f;
  length = config.getFloat("length", 10.0f);

  destructionAction = DestructionActionNames.getLeft(config.getString("destructionAction", "None"));
  String destructionImagePath = config.getString("destructionImage", "");
  if (destructionAction == DestructionAction::Image)
    destructionImagePath = AssetPath::relativeTo(path, destructionImagePath);
  destructionImage = destructionImagePath;

  destructionTime = config.getFloat("destructionTime", 0.0f);

  timeToLive = config.getFloat("timeToLive", 0.0f);
  layer = LayerNames.getLeft(config.getString("layer", "middle"));

  underwaterOnly = config.getBool("underwaterOnly", false);

  collidesForeground = config.getBool("collidesForeground", layer != Layer::Front || underwaterOnly);
  // only valid for collidesForeground particles
  collidesLiquid = config.getBool("collidesLiquid", type == Type::Ember);

  ignoreWind = config.getBool("ignoreWind", true);

  trail = config.getBool("trail", false);
}

Json Particle::toJson() const {
  return JsonObject{{"type", TypeNames.getRight(type)},
      {"size", size},
      {"string", string},
      {"color", jsonFromColor(color)},
      {"light", jsonFromColor(light)},
      {"fade", fade},
      {"fullbright", fullbright},
      {"position", jsonFromVec2F(position)},
      {"velocity", jsonFromVec2F(velocity)},
      {"finalVelocity", jsonFromVec2F(finalVelocity)},
      {"approach", jsonFromVec2F(approach)},
      {"flip", flip},
      {"flippable", flippable},
      {"rotation", rotation * 180.0f / Constants::pi},
      {"angularVelocity", angularVelocity * 180.0f / Constants::pi},
      {"length", length},
      {"destructionAction", DestructionActionNames.getRight(destructionAction)},
      {"destructionImage", AssetPath::join(destructionImage)},
      {"destructionTime", destructionTime},
      {"timeToLive", timeToLive},
      {"layer", LayerNames.getRight(layer)},
      {"collidesForeground", collidesForeground},
      {"collidesLiquid", collidesLiquid},
      {"underwaterOnly", underwaterOnly},
      {"ignoreWind", ignoreWind},
      {"trail", trail}};
}

void Particle::translate(Vec2F const& pos) {
  position += pos;
}

void Particle::update(float dt, Vec2F const& wind) {
  auto prevVelocity = velocity;
  if (ignoreWind) {
    velocity[0] = Star::approach<float>(finalVelocity[0], velocity[0], approach[0] * dt);
    velocity[1] = Star::approach<float>(finalVelocity[1], velocity[1], approach[1] * dt);
  } else {
    velocity[0] = Star::approach<float>(finalVelocity[0] + wind[0], velocity[0], approach[0] * dt);
    velocity[1] = Star::approach<float>(finalVelocity[1] + wind[1], velocity[1], approach[1] * dt);
  }
  position += (prevVelocity + velocity) * 0.5f * dt;

  rotation += angularVelocity * dt;

  if (light != Color::Clear)
    light.fade(fade * dt);

  timeToLive -= dt;

  if (timeToLive < 0.0f)
    destructionUpdate();

  if (type == Type::Animated) {
    initializeAnimation();
    animation->update(dt);
  }
}

bool Particle::dead() const {
  return timeToLive < -destructionTime;
}

void Particle::applyVariance(Particle const& variance) {
  size += variance.size * Random::randf(-1.0f, 1.0f);
  position += {variance.position[0] * Random::randf(-1.0f, 1.0f), variance.position[1] * Random::randf(-1.0f, 1.0f)};
  velocity += {variance.velocity[0] * Random::randf(-1.0f, 1.0f), variance.velocity[1] * Random::randf(-1.0f, 1.0f)};
  finalVelocity +=
      {variance.finalVelocity[0] * Random::randf(-1.0f, 1.0f), variance.finalVelocity[1] * Random::randf(-1.0f, 1.0f)};
  rotation += variance.rotation * Random::randf(-1.0f, 1.0f);
  angularVelocity += variance.angularVelocity * Random::randf(-1.0f, 1.0f);
  length += variance.length * Random::randf(-1.0f, 1.0f);
  timeToLive += variance.timeToLive * Random::randf(-1.0f, 1.0f);
}

void Particle::collide(Vec2F const& collisionPosition) {
  position = collisionPosition;
  approach = Vec2F();
  velocity = finalVelocity = Vec2F();
  destroy(true);
}

void Particle::destroy(bool withDestruction) {
  if (withDestruction) {
    if (timeToLive >= 0.0f) {
      timeToLive = 0.0f;
      destructionUpdate();
    }
  } else {
    timeToLive = -destructionTime - 1.0f;
  }
}

void Particle::destructionUpdate() {
  if (destructionTime > 0) {
    float destructionFactor = (timeToLive + destructionTime) / destructionTime;
    if (destructionAction == DestructionAction::Shrink) {
      size = baseSize * destructionFactor;
    } else if (destructionAction == DestructionAction::Fade) {
      color.setAlphaF(destructionFactor);
    } else if (destructionAction == DestructionAction::Image) {
      if (!destructionSet) {
        size = 1.0f;
        color = Color::White;
        type = Particle::Type::Textured;
        image = destructionImage;
        angularVelocity = 0.0f;
        length = 0.0f;
        rotation = 0.0f;
        destructionSet = true;
      }
    }
  }
}

void Particle::initializeAnimation() {
  if (!animation) {
    animation = Animation(AssetPath::removeDirectives(string));
    animation->setProcessing(directives);
  }
}

DataStream& operator<<(DataStream& ds, Particle const& particle) {
  ds.viwrite(particle.type);
  ds.write(particle.size);
  ds.write(particle.string);
  ds.write(particle.color);
  ds.write(particle.light);
  ds.write(particle.fade);
  ds.write(particle.position);
  ds.write(particle.velocity);
  ds.write(particle.finalVelocity);
  ds.write(particle.approach);
  ds.write(particle.rotation);
  ds.write(particle.flippable);
  ds.write(particle.flip);
  ds.write(particle.angularVelocity);
  ds.write(particle.length);
  ds.viwrite(particle.destructionAction);
  ds.write(particle.destructionImage);
  ds.write(particle.destructionTime);
  ds.write(particle.timeToLive);
  ds.write(particle.layer);
  ds.write(particle.collidesForeground);
  ds.write(particle.collidesLiquid);
  ds.write(particle.underwaterOnly);
  ds.write(particle.ignoreWind);

  return ds;
}

DataStream& operator>>(DataStream& ds, Particle& particle) {
  ds.viread(particle.type);
  ds.read(particle.size);
  ds.read(particle.string);
  ds.read(particle.color);
  ds.read(particle.light);
  ds.read(particle.fade);
  ds.read(particle.position);
  ds.read(particle.velocity);
  ds.read(particle.finalVelocity);
  ds.read(particle.approach);
  ds.read(particle.rotation);
  ds.read(particle.flippable);
  ds.read(particle.flip);
  ds.read(particle.angularVelocity);
  ds.read(particle.length);
  ds.viread(particle.destructionAction);
  ds.read(particle.destructionImage);
  ds.read(particle.destructionTime);
  ds.read(particle.timeToLive);
  ds.read(particle.layer);
  ds.read(particle.collidesForeground);
  ds.read(particle.collidesLiquid);
  ds.read(particle.underwaterOnly);
  ds.read(particle.ignoreWind);

  return ds;
}

ParticleVariantCreator makeParticleVariantCreator(Particle particle, Particle variance) {
  return [particle, variance]() -> Particle {
    auto p = particle;
    p.applyVariance(variance);
    return p;
  };
}

}

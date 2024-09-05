#include "StarProjectileDatabase.hpp"
#include "StarProjectile.hpp"
#include "StarJsonExtra.hpp"
#include "StarAssets.hpp"
#include "StarRoot.hpp"

namespace Star {

ProjectileDatabase::ProjectileDatabase() {
  auto assets = Root::singleton().assets();

  auto& files = assets->scanExtension("projectile");
  assets->queueJsons(files);
  for (auto& file : files) {
    try {
      auto projectileConfig = readConfig(file);
      if (m_configs.contains(projectileConfig->typeName))
        Logger::error("Duplicate projectile asset typeName {}. configfile {}", projectileConfig->typeName, file);
      else
        m_configs[projectileConfig->typeName] = projectileConfig;
    } catch (std::exception const& e) {
      Logger::error("Could not read projectile '{}', error: {}", file, outputException(e, false));
    }
  }
}

StringList ProjectileDatabase::allProjectileTypes() const {
  return m_configs.keys();
}

bool ProjectileDatabase::isProjectile(String const& projectileName) const {
  return m_configs.contains(projectileName);
}

Json ProjectileDatabase::projectileConfig(String const& type) const {
  if (!m_configs.contains(type))
    throw ProjectileDatabaseException(strf("Unknown projectile with typeName {}.", type));
  return m_configs.get(type)->config;
}

ProjectilePtr ProjectileDatabase::createProjectile(String const& type, Json const& parameters) const {
  if (!m_configs.contains(type))
    throw ProjectileDatabaseException(strf("Unknown projectile with typeName {}.", type));
  return make_shared<Projectile>(m_configs.get(type), parameters);
}

String ProjectileDatabase::damageKindImage(String const& type) const {
  if (!m_configs.contains(type))
    throw ProjectileDatabaseException(strf("Unknown projectile with typeName {}.", type));
  auto& config = m_configs.get(type);
  return config->damageKindImage;
}

float ProjectileDatabase::gravityMultiplier(String const& type) const {
  if (!m_configs.contains(type))
    throw ProjectileDatabaseException(strf("Unknown projectile with typeName {}.", type));
  auto& config = m_configs.get(type);
  return config->movementSettings.getFloat("gravityMultiplier", 1);
}

ProjectilePtr ProjectileDatabase::netLoadProjectile(ByteArray const& netStore, NetCompatibilityRules rules) const {
  DataStreamBuffer ds(netStore);
  ds.setStreamCompatibilityVersion(rules);
  String typeName = ds.read<String>();
  return make_shared<Projectile>(m_configs.get(typeName), ds, rules);
}

ProjectileConfigPtr ProjectileDatabase::readConfig(String const& path) {
  auto assets = Root::singleton().assets();

  Json config = assets->json(path);

  auto projectileConfig = make_shared<ProjectileConfig>();
  projectileConfig->config = config;
  projectileConfig->typeName = config.getString("projectileName");
  projectileConfig->directory = AssetPath::directory(path);

  projectileConfig->description = config.getString("description", "");

  projectileConfig->boundBox = config.opt("boundBox").apply(jsonToRectF).value({-5, -5, 5, 5});

  auto physicsType = config.getString("physics", "default");
  JsonObject movementSettings = config.getObject("movementSettings", JsonObject());
  projectileConfig->movementSettings =
      jsonMerge(assets->json(strf("/projectiles/physics.config:{}", physicsType)), movementSettings);

  projectileConfig->initialSpeed = config.getFloat("speed", 50);
  projectileConfig->acceleration = config.getFloat("acceleration", 0);
  projectileConfig->power = config.getFloat("power", 1);
  if (config.contains("damagePoly")) {
    projectileConfig->damagePoly = jsonToPolyF(config.get("damagePoly"));
    projectileConfig->damagePoly.scale(1.0f / TilePixels);
  }
  projectileConfig->piercing = config.getBool("piercing", false);
  projectileConfig->falldown = config.getBool("falldown", false);
  projectileConfig->bounces = config.getInt("bounces", 0);

  projectileConfig->actionOnCollide = config.getArray("actionOnCollide", {});
  projectileConfig->actionOnReap = config.getArray("actionOnReap", {});
  projectileConfig->actionOnHit = config.getArray("actionOnHit", {});
  projectileConfig->actionOnTimeout = config.getArray("actionOnTimeout", {});

  for (auto const& c : config.getArray("periodicActions", {}))
    projectileConfig->periodicActions.append(make_tuple(c.getFloat("time"), c.getBool("repeat", true), c));

  projectileConfig->image = AssetPath::relativeTo(path, config.getString("image"));
  projectileConfig->frameNumber = config.getUInt("frameNumber", 1);
  projectileConfig->animationCycle = config.getFloat("animationCycle", 1.0);
  projectileConfig->animationLoops = config.getBool("animationLoops", true);
  projectileConfig->windupFrames = config.getUInt("windupFrames", 0);
  projectileConfig->intangibleWindup = config.getBool("intangibleWindup", false);
  projectileConfig->winddownFrames = config.getUInt("winddownFrames", 0);
  projectileConfig->intangibleWinddown = config.getBool("intangibleWinddown", false);
  projectileConfig->flippable = config.getBool("flippable", false);
  projectileConfig->orientationLocked = config.getBool("orientationLocked", false);

  projectileConfig->fullbright = config.getBool("fullbright", false);
  projectileConfig->renderLayer = parseRenderLayer(config.getString("renderLayer", "Projectile"));

  projectileConfig->lightColor = jsonToColor(config.get("lightColor", JsonArray{0, 0, 0}));
  projectileConfig->lightPosition = jsonToVec2F(config.get("lightPosition", JsonArray{0, 0}));
  if (auto lightType = config.optString("lightType"))
    projectileConfig->lightType = LightTypeNames.getLeft(*lightType);
  else
    projectileConfig->lightType = (LightType)config.getBool("pointLight", false);

  projectileConfig->persistentAudio = config.getString("persistentAudio", "");

  // Initialize timeToLive after animationCycle so we can have the default be
  // based on animationCycle
  if (!projectileConfig->animationLoops)
    projectileConfig->timeToLive = config.getFloat("timeToLive", projectileConfig->animationCycle);
  else
    projectileConfig->timeToLive = config.getFloat("timeToLive", 5.0);

  projectileConfig->damageKindImage = config.getString("damageKindImage", "");

  projectileConfig->damageKind = config.getString("damageKind", "");
  projectileConfig->damageType = config.getString("damageType", "damage");
  projectileConfig->damageTeam = config.get("damageTeam", Json());
  projectileConfig->damageRepeatGroup = config.optString("damageRepeatGroup");
  projectileConfig->damageRepeatTimeout = config.optFloat("damageRepeatTimeout");

  if (!projectileConfig->damageKindImage.empty())
    projectileConfig->damageKindImage =
        AssetPath::relativeTo(projectileConfig->directory, projectileConfig->damageKindImage);

  projectileConfig->statusEffects = config.getArray("statusEffects", {}).transformed(jsonToEphemeralStatusEffect);

  projectileConfig->emitters = jsonToStringSet(config.getArray("emitters", JsonArray()));

  projectileConfig->hydrophobic = config.getBool("hydrophobic", false);

  projectileConfig->rayCheckToSource = config.getBool("rayCheckToSource", false);
  projectileConfig->knockback = config.getFloat("knockback", 0.0f);
  projectileConfig->knockbackDirectional = config.getBool("knockbackDirectional", false);

  projectileConfig->onlyHitTerrain = config.getBool("onlyHitTerrain", false);

  projectileConfig->clientEntityMode = ClientEntityModeNames.getLeft(config.getString("clientEntityMode", "ClientMasterAllowed"));
  projectileConfig->masterOnly = config.getBool("masterOnly", false);

  projectileConfig->scripts =
      jsonToStringList(config.get("scripts", JsonArray())).transformed(bind(AssetPath::relativeTo, path, _1));

  projectileConfig->physicsForces = config.get("physicsForces", JsonObject());
  projectileConfig->physicsCollisions = config.get("physicsCollisions", JsonObject());

  projectileConfig->persistentStatusEffects = config.getArray("persistentStatusEffects", JsonArray()).transformed(jsonToPersistentStatusEffect);
  projectileConfig->statusEffectArea = jsonToPolyF(config.get("statusEffectArea", JsonArray()));

  return projectileConfig;
}

}

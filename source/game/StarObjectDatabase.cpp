#include "StarObjectDatabase.hpp"
#include "StarObject.hpp"
#include "StarJsonExtra.hpp"
#include "StarIterator.hpp"
#include "StarWorld.hpp"
#include "StarAssets.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarRoot.hpp"
#include "StarImageMetadataDatabase.hpp"
#include "StarLogging.hpp"
#include "StarLoungeableObject.hpp"
#include "StarContainerObject.hpp"
#include "StarFarmableObject.hpp"
#include "StarTeleporterObject.hpp"
#include "StarPhysicsObject.hpp"

namespace Star {

ObjectOrientation::ParticleEmissionEntry ObjectOrientation::parseParticleEmitter(
    String const& path, Json const& config) {
  ObjectOrientation::ParticleEmissionEntry result;
  result.particleEmissionRate = config.getFloat("emissionRate", 0.0);
  result.particleEmissionRateVariance = config.getFloat("emissionVariance", 0.0);
  result.particle = Particle(config.getObject("particle", {}), path);
  result.particleVariance = Particle(config.getObject("particleVariance", {}), path);
  result.particle.position += jsonToVec2F(config.get("pixelOrigin", JsonArray{TilePixels / 2, TilePixels / 2})) / TilePixels;
  result.placeInSpaces = config.getBool("placeInSpaces", false);
  return result;
};

bool ObjectOrientation::placementValid(World const* world, Vec2I const& position) const {
  if (!world)
    return false;

  for (Vec2I space : spaces) {
    space += position;
    if (world->tileIsOccupied(space, TileLayer::Foreground, false, true) || world->isTileProtected(space))
      return false;
  }
  return true;
}

bool ObjectOrientation::anchorsValid(World const* world, Vec2I const& position) const {
  if (!world)
    return false;

  if (anchors.size() == 0)
    return true;

  auto materialDatabase = Root::singleton().materialDatabase();

  auto anchorValid = [&](Anchor const& anchor) -> bool {
      auto space = position + anchor.position;
      if (!world->isTileConnectable(space, anchor.layer))
        return false;
      if (anchor.tilled) {
        if (!materialDatabase->isTilledMod(world->mod(space, anchor.layer)))
          return false;
      }
      if (anchor.soil) {
        if (!materialDatabase->isSoil(world->material(space, anchor.layer)))
          return false;
      }
      if (anchor.material) {
        if (world->material(space, anchor.layer) != *anchor.material)
          return false;
      }
      return true;
    };

  bool anyValid = false;
  for (auto anchor : anchors) {
    auto valid = anchorValid(anchor);
    if (valid)
      anyValid = true;
    else if (!anchorAny)
      return false;
  }
  return anyValid;
}

size_t ObjectConfig::findValidOrientation(World const* world, Vec2I const& position, Maybe<Direction> directionAffinity) const {
  // If we are given a direction affinity, try and find an orientation with a
  // matching affinity *first*
  if (directionAffinity) {
    for (size_t i = 0; i < orientations.size(); ++i) {
      if (!orientations[i]->directionAffinity || *directionAffinity != *orientations[i]->directionAffinity)
        continue;

      if (orientations[i]->placementValid(world, position) && orientations[i]->anchorsValid(world, position))
        return i;
    }
  }

  // Then, fallback and try and find any valid affinity
  for (size_t i = 0; i < orientations.size(); ++i) {
    if (orientations[i]->placementValid(world, position) && orientations[i]->anchorsValid(world, position))
      return i;
  }

  return NPos;
}

Json ObjectDatabase::parseTouchDamage(String const& path, Json const& config) {
  auto touchDamage = config.get("touchDamage", {});
  if (touchDamage.isType(Json::Type::String)) {
    auto assets = Root::singleton().assets();
    return assets->fetchJson(AssetPath::relativeTo(path, touchDamage.toString()));
  }

  return touchDamage;
}

List<ObjectOrientationPtr> ObjectDatabase::parseOrientations(String const& path, Json const& configList) {
  auto& root = Root::singleton();
  auto materialDatabase = root.materialDatabase();
  List<ObjectOrientationPtr> res;
  JsonArray configs = configList.toArray();

  // Preprocess the orientation list for config format backwards compatibility.
  // If dualImage or left/right Image is set, generate two identical
  // orientations with the appropriate image directives.
  auto it = makeSMutableIterator(configs);
  while (it.hasNext()) {
    JsonObject config = it.next().toObject();
    if (config.contains("dualImage")) {
      it.remove();

      JsonObject configLeft = config;
      configLeft["image"] = config["dualImage"];
      configLeft["flipImages"] = true;
      configLeft["direction"] = "left";
      it.insert(configLeft);

      JsonObject configRight = config;
      configRight["image"] = config["dualImage"];
      configRight["direction"] = "right";
      it.insert(configRight);

    } else if (config.contains("leftImage")) {
      it.remove();

      JsonObject configLeft = config;
      configLeft["image"] = config["leftImage"];
      configLeft["direction"] = "left";
      it.insert(configLeft);

      JsonObject configRight = config;
      configRight["image"] = config["rightImage"];
      configRight["direction"] = "right";
      it.insert(configRight);
    }
  }

  for (auto const& orientationSettings : configs) {
    auto orientation = make_shared<ObjectOrientation>();
    orientation->config = orientationSettings;

    if (orientationSettings.contains("imageLayers")) {
      for (Json layer : orientationSettings.get("imageLayers").iterateArray()) {
        if (auto image = layer.opt("image"))
          layer = layer.set("image", AssetPath::relativeTo(path, image->toString()));
        Drawable drawable(layer.set("centered", layer.getBool("centered", false)));
        drawable.scale(1.0f / TilePixels);
        orientation->imageLayers.append(drawable);
      }
    } else {
      Drawable drawable = Drawable::makeImage(
          AssetPath::relativeTo(path, orientationSettings.getString("image")), 1.0 / TilePixels, false, {});
      drawable.fullbright = orientationSettings.getBool("fullbright", false);
      orientation->imageLayers.append(drawable);
    }

    orientation->renderLayer = parseRenderLayer(orientationSettings.getString("renderLayer", "Object"));

    orientation->flipImages = orientationSettings.getBool("flipImages", false);

    Vec2F imagePosition = jsonToVec2F(orientationSettings.getArray("imagePosition", {0, 0}));

    orientation->imagePosition = imagePosition / TilePixels;
    orientation->frames = orientationSettings.getInt("frames", 1);
    orientation->animationCycle = orientationSettings.getDouble("animationCycle", 1.0);

    if (orientationSettings.contains("spaces")) {
      for (auto v : orientationSettings.getArray("spaces"))
        orientation->spaces.append(jsonToVec2I(v));
    } else {
      orientation->spaces = {{0, 0}};
    }

    if (orientationSettings.contains("spaceScan")) {
      auto spaceScanSpaces = Set<Vec2I>::from(orientation->spaces);
      StringMap<String> imageKeys;
      imageKeys.set("color", orientationSettings.get("color", "default").toString().takeUtf8());
      for (auto p : orientationSettings.get("defaultImageKeys", JsonObject()).toObject())
        imageKeys.set(p.first, p.second.toString());

      for (auto const& layer : orientation->imageLayers) {
        if (layer.isImage())
          spaceScanSpaces.addAll(root.imageMetadataDatabase()->imageSpaces(
            AssetPath::join(layer.imagePart().image).replaceTags(imageKeys, true, "default"),
            imagePosition + (layer.position + layer.imagePart().transformation.transformVec2(Vec2F())) * TilePixels,
            orientationSettings.getDouble("spaceScan"),
            orientation->flipImages));
      }

      orientation->spaces = spaceScanSpaces.values();
    }

    orientation->boundBox = RectI::boundBoxOfPoints(orientation->spaces);

    orientation->metaBoundBox = orientationSettings.opt("metaBoundBox").apply(jsonToRectF);

    // Specify "anchors" to simplify fg / bg anchor listing

    bool tilled = orientationSettings.getBool("requireTilledAnchors", false);
    bool soil = orientationSettings.getBool("requireSoilAnchors", false);
    Maybe<MaterialId> anchorMaterial;
    if (auto anchorMaterialName = orientationSettings.optString("anchorMaterial"))
      anchorMaterial = materialDatabase->materialId(*anchorMaterialName);

    for (auto type : orientationSettings.getArray("anchors", {})) {
      String anchorType = type.toString();
      if (anchorType == "left") {
        for (auto space : orientation->spaces) {
          if (space[0] == orientation->boundBox.xMin())
            orientation->anchors.append({TileLayer::Foreground, space + Vec2I(-1, 0), tilled, soil, anchorMaterial});
        }
      } else if (anchorType == "bottom") {
        for (auto space : orientation->spaces) {
          if (space[1] == orientation->boundBox.yMin())
            orientation->anchors.append({TileLayer::Foreground, space + Vec2I(0, -1), tilled, soil, anchorMaterial});
        }
      } else if (anchorType == "right") {
        for (auto space : orientation->spaces) {
          if (space[0] == orientation->boundBox.xMax())
            orientation->anchors.append({TileLayer::Foreground, space + Vec2I(1, 0), tilled, soil, anchorMaterial});
        }
      } else if (anchorType == "top") {
        for (auto space : orientation->spaces) {
          if (space[1] == orientation->boundBox.yMax())
            orientation->anchors.append({TileLayer::Foreground, space + Vec2I(0, 1), tilled, soil, anchorMaterial});
        }
      } else if (anchorType == "background") {
        for (auto space : orientation->spaces)
          orientation->anchors.append({TileLayer::Background, space, tilled, soil, anchorMaterial});
      } else {
        throw ObjectException(strf("Unknown anchor type: {}", anchorType));
      }
    }

    for (auto v : orientationSettings.getArray("bgAnchors", {}))
      orientation->anchors.append({TileLayer::Background, jsonToVec2I(v), tilled, soil, anchorMaterial});

    for (auto v : orientationSettings.getArray("fgAnchors", {}))
      orientation->anchors.append({TileLayer::Foreground, jsonToVec2I(v), tilled, soil, anchorMaterial});

    orientation->anchorAny = orientationSettings.getBool("anchorAny", false);

    if (orientationSettings.contains("direction"))
      orientation->directionAffinity = DirectionNames.getLeft(orientationSettings.getString("direction", "left"));

    auto collisionType = orientationSettings.getString("collision", "none");
    if (orientationSettings.contains("materialSpaces")) {
      for (auto space : orientationSettings.get("materialSpaces").iterateArray()) {
        String materialName = space.get(1).toString();
        orientation->materialSpaces.append({jsonToVec2I(space.get(0)), materialDatabase->materialId(materialName)});
      }
    } else if (collisionType == "solid") {
      if (orientationSettings.contains("collisionSpaces")) {
        for (auto space : orientationSettings.get("collisionSpaces").iterateArray())
          orientation->materialSpaces.append({jsonToVec2I(space), ObjectSolidMaterialId});
      } else {
        for (auto space : orientation->spaces)
          orientation->materialSpaces.append({space, ObjectSolidMaterialId});
      }
    } else if (collisionType == "platform") {
      if (orientationSettings.contains("collisionSpaces")) {
        for (auto space : orientationSettings.get("collisionSpaces").iterateArray())
          orientation->materialSpaces.append({jsonToVec2I(space), ObjectPlatformMaterialId});
      } else {
        for (auto space : orientation->spaces) {
          if (space[1] == orientation->boundBox.yMax())
            orientation->materialSpaces.append({space, ObjectPlatformMaterialId});
        }
      }
    }

    if (orientationSettings.contains("interactiveSpaces")) {
      List<Vec2I> iSpaces;
      for (auto space : orientationSettings.get("interactiveSpaces").iterateArray())
        iSpaces.append(jsonToVec2I(space));
      orientation->interactiveSpaces = iSpaces;
    }

    orientation->lightPosition = jsonToVec2F(orientationSettings.getArray("lightPosition", {0, 0}));
    orientation->beamAngle = orientationSettings.getFloat("beamAngle", 0.0f) * Constants::deg2rad;

    if (orientationSettings.contains("particleEmitter"))
      orientation->particleEmitters.append(
          ObjectOrientation::parseParticleEmitter(path, orientationSettings.get("particleEmitter")));
    for (auto particleEmitterConfig : orientationSettings.getArray("particleEmitters", {}))
      orientation->particleEmitters.append(ObjectOrientation::parseParticleEmitter(path, particleEmitterConfig));

    orientation->statusEffectArea = orientationSettings.opt("statusEffectArea").apply(jsonToPolyF);

    orientation->touchDamageConfig = parseTouchDamage(path, orientationSettings);

    res.append(std::move(orientation));
  }

  return res;
}

ObjectDatabase::ObjectDatabase() {
  auto assets = Root::singleton().assets();

  auto& files = assets->scanExtension("object");
  assets->queueJsons(files);
  for (auto& file : files) {
    try {
      String name = assets->json(file).getString("objectName");
      if (m_paths.contains(name))
        Logger::error("Object {} defined twice, second time from {}", name, file);
      else
        m_paths[name] = file;
    } catch (std::exception const& e) {
      Logger::error("Error loading object file {}: {}", file, outputException(e, true));
    }
  }
}

void ObjectDatabase::cleanup() {
  MutexLocker locker(m_cacheMutex);
  m_configCache.cleanup([](String const&, ObjectConfigPtr const& config) {
      return !config.unique();
    });
}

StringList ObjectDatabase::allObjects() const {
  return m_paths.keys();
}

bool ObjectDatabase::isObject(String const& objectName) const {
  return m_paths.contains(objectName);
}

ObjectConfigPtr ObjectDatabase::getConfig(String const& objectName) const {
  MutexLocker locker(m_cacheMutex);
  return m_configCache.get(objectName,
      [this](String const& objectName) -> ObjectConfigPtr {
        if (auto path = m_paths.maybe(objectName))
          return readConfig(*path);
        throw ObjectException(strf("No such object named '{}'", objectName));
      });
}

List<ObjectOrientationPtr> const& ObjectDatabase::getOrientations(String const& objectName) const {
  return getConfig(objectName)->orientations;
}

ObjectPtr ObjectDatabase::createObject(String const& objectName, Json const& parameters) const {
  auto config = getConfig(objectName);

  if (config->type == "object") {
    return make_shared<Object>(config, parameters);
  } else if (config->type == "loungeable") {
    return make_shared<LoungeableObject>(config, parameters);
  } else if (config->type == "container") {
    return make_shared<ContainerObject>(config, parameters);
  } else if (config->type == "farmable") {
    return make_shared<FarmableObject>(config, parameters);
  } else if (config->type == "teleporter") {
    return make_shared<TeleporterObject>(config, parameters);
  } else if (config->type == "physics") {
    return make_shared<PhysicsObject>(config, parameters);
  } else {
    throw ObjectException(strf("Unknown objectType '{}' constructing object '{}'", config->type, objectName));
  }
}

ObjectPtr ObjectDatabase::diskLoadObject(Json const& diskStore) const {
  auto object = createObject(diskStore.getString("name"), diskStore.get("parameters"));
  object->readStoredData(diskStore);
  object->setNetStates();
  return object;
}

ObjectPtr ObjectDatabase::netLoadObject(ByteArray const& netStore, NetCompatibilityRules rules) const {
  DataStreamBuffer ds(netStore);
  ds.setStreamCompatibilityVersion(rules);
  String name = ds.read<String>();
  Json parameters = ds.read<Json>();
  return createObject(name, parameters);
}

bool ObjectDatabase::canPlaceObject(World const* world, Vec2I const& position, String const& objectName) const {
  return getConfig(objectName)->findValidOrientation(world, position) != NPos;
}

ObjectPtr ObjectDatabase::createForPlacement(World const* world, String const& objectName, Vec2I const& position,
    Direction direction, Json const& parameters) const {
  if (!canPlaceObject(world, position, objectName))
    return {};

  ObjectPtr object = createObject(objectName, parameters);
  object->setTilePosition(world->geometry().xwrap(position));
  object->setDirection(direction);

  return object;
}

ObjectConfigPtr ObjectDatabase::readConfig(String const& path) {
  try {
    auto assets = Root::singleton().assets();

    Json config = assets->json(path);

    auto objectConfig = make_shared<ObjectConfig>();
    objectConfig->path = path;
    objectConfig->config = config;

    objectConfig->name = config.getString("objectName");
    objectConfig->type = config.getString("objectType", "object");
    objectConfig->race = config.getString("race", "generic");
    objectConfig->category = config.getString("category", "other");
    objectConfig->colonyTags = jsonToStringList(config.get("colonyTags", JsonArray()));

    objectConfig->scripts = jsonToStringList(config.get("scripts", JsonArray())).transformed(bind(AssetPath::relativeTo, path, _1));
    objectConfig->animationScripts = jsonToStringList(config.get("animationScripts", JsonArray())).transformed(bind(AssetPath::relativeTo, path, _1));

    objectConfig->price = config.getInt("price", 0);
    if (objectConfig->price == 0)
      objectConfig->price = 1;

    objectConfig->hasObjectItem = config.getBool("hasObjectItem", true);

    objectConfig->scannable = config.getBool("scannable", true);
    objectConfig->printable = objectConfig->hasObjectItem && config.getBool("printable", objectConfig->scannable);

    objectConfig->retainObjectParametersInItem = config.getBool("retainObjectParametersInItem", false);

    if (config.contains("breakDropPool"))
      objectConfig->breakDropPool = config.getString("breakDropPool");

    if (config.contains("breakDropOptions")) {
      for (auto dropChoiceGroups : config.get("breakDropOptions").iterateArray()) {
        List<ItemDescriptor> group;
        for (auto dropChoiceEntry : dropChoiceGroups.iterateArray())
          group.append(
              {dropChoiceEntry.getString(0), (size_t)dropChoiceEntry.getUInt(1), dropChoiceEntry.getObject(2)});
        objectConfig->breakDropOptions.append(group);
      }
      // If breakDropOptions is set but empty, then the object should always
      // drop nothing.
      if (objectConfig->breakDropOptions.empty())
        objectConfig->breakDropOptions.append({});
    }

    if (config.contains("smashDropPool"))
      objectConfig->smashDropPool = config.getString("smashDropPool");

    for (auto dropChoiceGroups : config.get("smashDropOptions", JsonArray()).iterateArray()) {
      List<ItemDescriptor> group;
      for (auto dropChoiceEntry : dropChoiceGroups.iterateArray())
        group.append(ItemDescriptor(dropChoiceEntry));
      objectConfig->smashDropOptions.append(group);
    }

    for (auto& sound : config.get("smashSounds", JsonArray()).iterateArray())
      objectConfig->smashSoundOptions.append(AssetPath::relativeTo(path, sound.toString()));

    if (config.contains("smashParticles"))
      objectConfig->smashParticles = config.getArray("smashParticles");

    objectConfig->smashable = config.getBool("smashable", false);

    objectConfig->smashOnBreak = config.getBool("smashOnBreak", objectConfig->smashable);

    objectConfig->unbreakable = config.getBool("unbreakable", false);
    if (objectConfig->unbreakable)
      objectConfig->smashable = false;

    objectConfig->tileDamageParameters = TileDamageParameters(
        assets->fetchJson(config.get("damageTable", "/objects/defaultParameters.config:damageTable")),
        config.optFloat("health"),
        config.optUInt("harvestLevel"));

    objectConfig->damageShakeMagnitude = config.getFloat("damageShakeMagnitude", 0.2f);
    objectConfig->damageMaterialKind = config.getString("damageMaterialKind", "solid");

    if (config.contains("damageTeam")) {
      auto damageTeam = config.get("damageTeam");
      objectConfig->damageTeam.type = TeamTypeNames.getLeft(damageTeam.getString("type", "environment"));
      objectConfig->damageTeam.team = damageTeam.getUInt("team", 0);
    }

    if (config.contains("lightColor")) {
      objectConfig->lightColors["default"] = jsonToColor(config.get("lightColor"));
    } else if (config.contains("lightColors")) {
      for (auto const& pair : config.get("lightColors").iterateObject())
        objectConfig->lightColors[pair.first] = jsonToColor(pair.second);
    }

    if (auto lightType = config.optString("lightType"))
      objectConfig->lightType = LightTypeNames.getLeft(*lightType);
    else
      objectConfig->lightType = (LightType)config.getBool("pointLight", false);
    objectConfig->pointBeam = config.getFloat("pointBeam", 0.0f);
    objectConfig->beamAmbience = config.getFloat("beamAmbience", 0.0f);

    if (config.contains("flickerPeriod")) {
      objectConfig->lightFlickering = PeriodicFunction<float>(config.getFloat("flickerPeriod"),
          config.getFloat("flickerMinIntensity", 0.0),
          config.getFloat("flickerMaxIntensity", 0.0),
          config.getFloat("flickerPeriodVariance", 0.0),
          config.getFloat("flickerIntensityVariance", 0.0));
    }

    objectConfig->soundEffect = config.getString("soundEffect", "");
    objectConfig->soundEffectRangeMultiplier = config.getFloat("soundEffectRangeMultiplier", 1.0f);

    objectConfig->statusEffects = config.getArray("statusEffects", {}).transformed(jsonToPersistentStatusEffect);
    objectConfig->touchDamageConfig = parseTouchDamage(path, config);

    objectConfig->minimumLiquidLevel = config.optFloat("minimumLiquidLevel");
    objectConfig->maximumLiquidLevel = config.optFloat("maximumLiquidLevel");
    objectConfig->liquidCheckInterval = config.getFloat("liquidCheckInterval", 0.5);

    objectConfig->health = config.getFloat("health", 1);

    if (auto animationConfig = config.get("animation", {})) {
      objectConfig->animationConfig = assets->fetchJson(animationConfig, path);
      if (auto customConfig = config.get("animationCustom", {}))
        objectConfig->animationConfig = jsonMerge(objectConfig->animationConfig, assets->fetchJson(customConfig, path));
    }

    objectConfig->orientations = ObjectDatabase::parseOrientations(path, config.get("orientations"));

    // For compatibility, allow particle emitter specs in the base config as
    // well as in individual orientations.

    List<ObjectOrientation::ParticleEmissionEntry> particleEmitters;
    if (config.contains("particleEmitter"))
      particleEmitters.append(ObjectOrientation::parseParticleEmitter(path, config.get("particleEmitter")));
    for (auto particleEmitterConfig : config.getArray("particleEmitters", {}))
      particleEmitters.append(ObjectOrientation::parseParticleEmitter(path, particleEmitterConfig));

    for (auto orientation : objectConfig->orientations)
      orientation->particleEmitters.appendAll(particleEmitters);

    objectConfig->rooting = config.getBool("rooting", false);

    objectConfig->biomePlaced = config.getBool("biomePlaced", false);

    return objectConfig;
  } catch (std::exception const& e) {
    throw ObjectException::format("Error loading object '{}': {}", path, outputException(e, false));
  }
}

List<Drawable> ObjectDatabase::cursorHintDrawables(World const* world, String const& objectName, Vec2I const& position,
    Direction direction, Json parameters) const {
  List<Drawable> drawables;

  auto config = getConfig(objectName);
  parameters = jsonMerge(config->config, parameters);

  if (auto placementImage = parameters.optString("placementImage")) {
    if (direction == Direction::Left)
      *placementImage += "?flipx";
    drawables = {Drawable::makeImage(AssetPath::relativeTo(config->path, *placementImage),
        1.0 / TilePixels, false, Vec2F(position) + jsonToVec2F(parameters.get("placementImagePosition")) / TilePixels)};
  } else {
    size_t orientationIndex = config->findValidOrientation(world, position, direction);
    if (orientationIndex == NPos) {
      // If we aren't in a valid orientation, still need to draw something at
      // the cursor.  Draw the first orientation whose direction affinity
      // matches our current direction, or if that fails just the first
      // orientation.
      List<Drawable> result;
      for (size_t i = 0; i < config->orientations.size(); ++i) {
        if (config->orientations[i]->directionAffinity == direction)
          orientationIndex = i;
      }
      if (orientationIndex == NPos)
        orientationIndex = 0;
    }

    auto orientation = config->orientations.at(orientationIndex);

    StringMap<String> imageKeys;
    imageKeys.set("color", orientation->config.get("color", "default").toString().takeUtf8());
    for (auto p : orientation->config.get("defaultImageKeys", JsonObject()).toObject())
      imageKeys.set(p.first, p.second.toString());

    for (auto const& layer : orientation->imageLayers) {
      Drawable drawable = layer;
      auto& image = drawable.imagePart().image;

      image = AssetPath::join(image).replaceTags(imageKeys, true, "default");
      if (orientation->flipImages)
        drawable.scale(Vec2F(-1, 1), drawable.boundBox(false).center() - drawable.position);
      drawables.append(std::move(drawable));
    }
    Drawable::translateAll(drawables, Vec2F(position) + orientation->imagePosition);
  }

  return drawables;
}

}

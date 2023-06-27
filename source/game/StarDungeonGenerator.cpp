#include "StarDungeonGenerator.hpp"
#include "StarCasting.hpp"
#include "StarRandom.hpp"
#include "StarLogging.hpp"
#include "StarAssets.hpp"
#include "StarLexicalCast.hpp"
#include "StarJsonExtra.hpp"
#include "StarMaterialDatabase.hpp"
#include "StarRoot.hpp"
#include "StarLiquidsDatabase.hpp"
#include "StarDungeonImagePart.hpp"
#include "StarDungeonTMXPart.hpp"

namespace Star {

size_t const DefinitionsCacheSize = 20;

namespace Dungeon {

  EnumMap<Dungeon::Direction> const DungeonDirectionNames{
      {Dungeon::Direction::Left, "left"},
      {Dungeon::Direction::Right, "right"},
      {Dungeon::Direction::Up, "up"},
      {Dungeon::Direction::Down, "down"},
      {Dungeon::Direction::Unknown, "unknown"},
      {Dungeon::Direction::Any, "any"},
  };

  Direction flipDirection(Direction direction) {
    if (direction == Direction::Left)
      return Direction::Right;
    if (direction == Direction::Right)
      return Direction::Left;
    if (direction == Direction::Up)
      return Direction::Down;
    if (direction == Direction::Down)
      return Direction::Up;
    if (direction == Direction::Any)
      return Direction::Any;
    throw DungeonException("Invalid direction");
  }

  MaterialId biomeMaterialForJson(int variant) {
    if (variant == 0)
      return BiomeMaterialId;
    if (variant == 1)
      return Biome1MaterialId;
    if (variant == 2)
      return Biome2MaterialId;
    if (variant == 3)
      return Biome3MaterialId;
    if (variant == 4)
      return Biome4MaterialId;
    starAssert(variant == 5);
    return Biome5MaterialId;
  }

  ConnectorConstPtr chooseOption(List<ConnectorConstPtr>& options, RandomSource& rnd) {
    float distribution = 0;
    for (size_t i = 0; i < options.size(); i++)
      distribution += options[i]->part()->chance();
    float pick = rnd.randf() * distribution;
    for (size_t i = 0; i < options.size(); i++) {
      pick -= options[i]->part()->chance();
      if (pick <= 0)
        return options.takeAt(i);
    }
    // float rounding is always fun
    return options.takeAt(options.size() - 1);
  }

  List<RuleConstPtr> Rule::readRules(Json const& rules) {
    List<RuleConstPtr> result;
    for (auto const& list : rules.iterateArray()) {
      Maybe<RuleConstPtr> rule = Rule::parse(list);
      if (rule.isValid())
        result.push_back(*rule);
    }
    return result;
  }

  List<BrushConstPtr> Brush::readBrushes(Json const& brushes) {
    List<BrushConstPtr> result;
    for (auto const& list : brushes.iterateArray())
      result.push_back(Brush::parse(list));
    return result;
  }

  Maybe<RuleConstPtr> Rule::parse(Json const& rule) {
    String key = rule.getString(0);
    if (key == "worldGenMustContainLiquid")
      return as<Rule>(make_shared<const WorldGenMustContainLiquidRule>());
    if (key == "worldGenMustNotContainLiquid")
      return as<Rule>(make_shared<const WorldGenMustNotContainLiquidRule>());

    if (key == "worldGenMustContainSolidForeground")
      return as<Rule>(make_shared<const WorldGenMustContainSolidRule>(TileLayer::Foreground));
    if (key == "worldGenMustContainAirForeground")
      return as<Rule>(make_shared<const WorldGenMustContainAirRule>(TileLayer::Foreground));

    if (key == "worldGenMustContainSolidBackground")
      return as<Rule>(make_shared<const WorldGenMustContainSolidRule>(TileLayer::Background));
    if (key == "worldGenMustContainAirBackground")
      return as<Rule>(make_shared<const WorldGenMustContainAirRule>(TileLayer::Background));

    if (key == "allowOverdrawing")
      return as<Rule>(make_shared<const AllowOverdrawingRule>());
    if (key == "ignorePartMaximumRule")
      return as<Rule>(make_shared<const IgnorePartMaximumRule>());
    if (key == "maxSpawnCount")
      return as<Rule>(make_shared<const MaxSpawnCountRule>(rule));
    if (key == "doNotConnectToPart")
      return as<Rule>(make_shared<const DoNotConnectToPartRule>(rule));
    if (key == "doNotCombineWith")
      return as<Rule>(make_shared<const DoNotCombineWithRule>(rule));

    Logger::error("Unknown dungeon rule: {}", key);
    return Maybe<RuleConstPtr>();
  }

  bool Rule::checkTileCanPlace(Vec2I, DungeonGeneratorWriter*) const {
    return true;
  }

  bool Rule::overdrawable() const {
    return false;
  }

  bool Rule::ignorePartMaximum() const {
    return false;
  }

  bool Rule::allowSpawnCount(int) const {
    return true;
  }

  bool Rule::doesNotConnectToPart(String const&) const {
    return false;
  }

  bool Rule::checkPartCombinationsAllowed(StringMap<int> const&) const {
    return true;
  }

  bool Rule::requiresOpen() const {
    return false;
  }

  bool Rule::requiresSolid() const {
    return false;
  }

  bool Rule::requiresLiquid() const {
    return false;
  }

  BrushConstPtr parseFrontBrush(Json const& brush) {
    String material;
    Maybe<String> mod;
    Maybe<float> hueshift, modhueshift;
    Maybe<MaterialColorVariant> colorVariant;

    if (brush.isType(Json::Type::Object)) {
      material = brush.getString("material");
      mod = brush.optString("mod");
      hueshift = brush.optFloat("hueshift");
      modhueshift = brush.optFloat("modhueshift");
      colorVariant = brush.optFloat("colorVariant");
    } else {
      material = brush.getString(1);
      if (brush.size() > 2)
        mod = brush.getString(2);
    }
    return make_shared<const FrontBrush>(material, mod, hueshift, modhueshift, colorVariant);
  }

  BrushConstPtr parseBackBrush(Json const& brush) {
    String material;
    Maybe<String> mod;
    Maybe<float> hueshift, modhueshift;
    Maybe<MaterialColorVariant> colorVariant;

    if (brush.isType(Json::Type::Object)) {
      material = brush.getString("material");
      mod = brush.optString("mod");
      hueshift = brush.optFloat("hueshift");
      modhueshift = brush.optFloat("modhueshift");
      colorVariant = brush.optFloat("colorVariant");
    } else {
      material = brush.getString(1);
      if (brush.size() > 2)
        mod = brush.getString(2);
    }
    return make_shared<const BackBrush>(material, mod, hueshift, modhueshift, colorVariant);
  }

  BrushConstPtr parseObjectBrush(Json const& brush) {
    String object;
    Star::Direction direction;
    Json parameters;

    object = brush.getString(1);
    JsonObject settings;
    if (brush.size() > 2)
      settings = brush.getObject(2);
    if (settings.contains("direction"))
      direction = DirectionNames.getLeft(settings.get("direction").toString());
    else
      direction = Star::Direction::Left;

    if (settings.contains("parameters"))
      parameters = settings.get("parameters");
    return make_shared<const ObjectBrush>(object, direction, parameters);
  }

  BrushConstPtr parseSurfaceBrush(Json const& brush) {
    Json settings = Json::ofType(Json::Type::Object);
    if (brush.size() > 1)
      settings = brush.get(1);
    return make_shared<const SurfaceBrush>(settings.optInt("variant"), settings.optString("mod"));
  }

  BrushConstPtr parseSurfaceBackgroundBrush(Json const& brush) {
    Json settings = Json::ofType(Json::Type::Object);
    if (brush.size() > 1)
      settings = brush.get(1);
    return make_shared<const SurfaceBackgroundBrush>(settings.optInt("variant"), settings.optString("mod"));
  }

  BrushConstPtr parseWireBrush(Json const& brush) {
    Json settings = brush.get(1);
    String group = settings.getString("group");
    bool local = settings.getBool("local", true);
    return make_shared<const WireBrush>(group, local);
  }

  BrushConstPtr parseItemBrush(Json const& brush) {
    ItemDescriptor item(brush.getString(1), 1);
    return make_shared<const ItemBrush>(item);
  }

  BrushConstPtr Brush::parse(Json const& brush) {
    String key = brush.getString(0);
    if (key == "clear")
      return as<const Brush>(make_shared<ClearBrush>());

    if (key == "front")
      return parseFrontBrush(brush);
    if (key == "back")
      return parseBackBrush(brush);
    if (key == "object")
      return parseObjectBrush(brush);
    if (key == "biomeitems")
      return as<Brush>(make_shared<BiomeItemsBrush>());
    if (key == "biometree")
      return as<Brush>(make_shared<BiomeTreeBrush>());
    if (key == "item")
      return parseItemBrush(brush);
    if (key == "npc")
      return as<Brush>(make_shared<NpcBrush>(brush.get(1)));
    if (key == "stagehand")
      return as<Brush>(make_shared<StagehandBrush>(brush.get(1)));
    if (key == "random")
      return as<Brush>(make_shared<RandomBrush>(brush));
    if (key == "surface")
      return parseSurfaceBrush(brush);
    if (key == "surfacebackground")
      return parseSurfaceBackgroundBrush(brush);
    if (key == "liquid")
      return as<Brush>(make_shared<LiquidBrush>(brush.getString(1), 1.0f, brush.getBool(2, false)));
    if (key == "wire")
      return parseWireBrush(brush);
    if (key == "playerstart")
      return as<Brush>(make_shared<PlayerStartBrush>());
    throw DungeonException::format("Unknown dungeon brush: {}", key);
  }

  RandomBrush::RandomBrush(Json const& brush) {
    JsonArray options = brush.getArray(1);
    for (auto const& option : options)
      m_brushes.append(Brush::parse(option));
    m_seed = Random::randi64();
  }

  void RandomBrush::paint(Vec2I position, Phase phase, DungeonGeneratorWriter* writer) const {
    size_t rnd = (size_t)staticRandomI32(m_seed, position[0], position[1]);
    m_brushes[rnd % m_brushes.size()]->paint(position, phase, writer);
  }

  void ClearBrush::paint(Vec2I position, Phase phase, DungeonGeneratorWriter* writer) const {
    if (phase != Phase::ClearPhase)
      return;

    // TODO: delete objects too?
    writer->setLiquid(position, LiquidStore(EmptyLiquidId, 0.0f, 0.0f, false));
    writer->setForegroundMaterial(position, EmptyMaterialId, 0, DefaultMaterialColorVariant);
    writer->setBackgroundMaterial(position, EmptyMaterialId, 0, DefaultMaterialColorVariant);
    writer->setForegroundMod(position, NoModId, 0);
    writer->setBackgroundMod(position, NoModId, 0);
  }

  FrontBrush::FrontBrush(String const& material, Maybe<String> mod, Maybe<float> hueshift, Maybe<float> modhueshift, Maybe<MaterialColorVariant> colorVariant) {
    m_material = material;
    m_mod = mod;
    m_materialHue = hueshift.apply(materialHueFromDegrees).value(0);
    m_modHue = modhueshift.apply(materialHueFromDegrees).value(0);
    m_materialColorVariant = colorVariant.value(DefaultMaterialColorVariant);
  }

  void FrontBrush::paint(Vec2I position, Phase phase, DungeonGeneratorWriter* writer) const {
    if (phase != Phase::WallPhase)
      return;

    auto materialDatabase = Root::singleton().materialDatabase();
    MaterialId material = materialDatabase->materialId(m_material);

    ModId mod = NoModId;
    if (m_mod)
      mod = materialDatabase->modId(*m_mod);

    if (isSolidColliding(materialDatabase->materialCollisionKind(material)))
      writer->setLiquid(position, LiquidStore(EmptyLiquidId, 0.0f, 0.0f, false));
    writer->setForegroundMaterial(position, material, m_materialHue, m_materialColorVariant);
    if (isRealMod(mod)) {
      writer->setForegroundMod(position, mod, m_modHue);
    }
  }

  BackBrush::BackBrush(String const& material, Maybe<String> mod, Maybe<float> hueshift, Maybe<float> modhueshift, Maybe<MaterialColorVariant> colorVariant) {
    m_material = material;
    m_mod = mod;
    m_materialHue = hueshift.apply(materialHueFromDegrees).value(0);
    m_modHue = modhueshift.apply(materialHueFromDegrees).value(0);
    m_materialColorVariant = colorVariant.value(DefaultMaterialColorVariant);
  }

  void BackBrush::paint(Vec2I position, Phase phase, DungeonGeneratorWriter* writer) const {
    if (phase != Phase::WallPhase)
      return;

    auto materialDatabase = Root::singleton().materialDatabase();
    MaterialId material = materialDatabase->materialId(m_material);

    ModId mod = NoModId;
    if (m_mod)
      mod = materialDatabase->modId(*m_mod);

    writer->setBackgroundMaterial(position, material, m_materialHue, m_materialColorVariant);
    if (isRealMod(mod)) {
      writer->setBackgroundMod(position, mod, m_modHue);
    }
  }

  ObjectBrush::ObjectBrush(String const& object, Star::Direction direction, Json const& parameters) {
    m_object = object;
    m_direction = direction;
    m_parameters = parameters;
  }

  void ObjectBrush::paint(Vec2I position, Phase phase, DungeonGeneratorWriter* writer) const {
    if (phase != Phase::ObjectPhase)
      return;
    writer->placeObject(position, m_object, m_direction, m_parameters);
  }

  VehicleBrush::VehicleBrush(String const& vehicle, Json const& parameters) {
    m_vehicle = vehicle;
    m_parameters = parameters;
  }

  void VehicleBrush::paint(Vec2I position, Phase phase, DungeonGeneratorWriter* writer) const {
    if (phase != Phase::ObjectPhase)
      return;
    writer->placeVehicle(Vec2F(position), m_vehicle, m_parameters);
  }

  void BiomeItemsBrush::paint(Vec2I position, Phase phase, DungeonGeneratorWriter* writer) const {
    if (phase != Phase::BiomeItemsPhase)
      return;
    writer->placeSurfaceBiomeItems(position);
  }

  void BiomeTreeBrush::paint(Vec2I position, Phase phase, DungeonGeneratorWriter* writer) const {
    if (phase != Phase::BiomeTreesPhase)
      return;
    writer->placeBiomeTree(position);
  }

  ItemBrush::ItemBrush(ItemDescriptor const& item) : m_item(item) {}

  void ItemBrush::paint(Vec2I position, Phase phase, DungeonGeneratorWriter* writer) const {
    if (phase != Phase::ItemPhase)
      return;
    writer->addDrop(Vec2F(position), m_item);
  }

  NpcBrush::NpcBrush(Json const& brush) {
    m_npc = brush;
    auto map = m_npc.toObject();
    if (map.value("seed") == Json("stable"))
      map["seed"] = Random::randu64();
    m_npc = map;
  }

  void NpcBrush::paint(Vec2I position, Phase phase, DungeonGeneratorWriter* writer) const {
    if (phase != Phase::NpcPhase)
      return;

    if (m_npc.contains("species")) {
      // interpret species as a comma separated list of unquoted strings
      StringList speciesOptions = m_npc.get("species").toString().replace(" ", "").split(",");
      writer->spawnNpc(Vec2F(position), m_npc.set("species", Random::randFrom(speciesOptions)));
    } else {
      writer->spawnNpc(Vec2F(position), m_npc);
    }
  }

  StagehandBrush::StagehandBrush(Json const& definition) {
    m_definition = definition;
  }

  void StagehandBrush::paint(Vec2I position, Phase phase, DungeonGeneratorWriter* writer) const {
    if (phase != Phase::NpcPhase)
      return;
    writer->spawnStagehand(Vec2F(position), m_definition);
  }

  DungeonIdBrush::DungeonIdBrush(DungeonId dungeonId) {
    m_dungeonId = dungeonId;
  }

  void DungeonIdBrush::paint(Vec2I position, Phase phase, DungeonGeneratorWriter* writer) const {
    if (phase != Phase::DungeonIdPhase)
      return;
    writer->setDungeonId(position, m_dungeonId);
  }

  SurfaceBrush::SurfaceBrush(Maybe<int> variant, Maybe<String> mod) {
    m_variant = variant.value(0);
    m_mod = mod;
  }

  void SurfaceBrush::paint(Vec2I position, Phase phase, DungeonGeneratorWriter* writer) const {
    if (phase == Phase::WallPhase) {
      writer->setForegroundMaterial(position, biomeMaterialForJson(m_variant), 0, DefaultMaterialColorVariant);
      writer->setBackgroundMaterial(position, biomeMaterialForJson(m_variant), 0, DefaultMaterialColorVariant);
    }
    if (phase == Phase::ModsPhase) {
      if (m_mod.isValid()) {
        auto materialDatabase = Root::singleton().materialDatabase();
        writer->setForegroundMod(position, materialDatabase->modId(*m_mod), 0);
      } else {
        if (writer->needsForegroundBiomeMod(position)) {
          writer->setForegroundMod(position, BiomeModId, 0);
        }
      }
    }
  }

  SurfaceBackgroundBrush::SurfaceBackgroundBrush(Maybe<int> variant, Maybe<String> mod) {
    m_variant = variant.value(0);
    m_mod = mod;
  }

  void SurfaceBackgroundBrush::paint(Vec2I position, Phase phase, DungeonGeneratorWriter* writer) const {
    if (phase == Phase::WallPhase) {
      writer->setBackgroundMaterial(position, biomeMaterialForJson(m_variant), 0, DefaultMaterialColorVariant);
    }
    if (phase == Phase::ModsPhase) {
      if (m_mod.isValid()) {
        auto materialDatabase = Root::singleton().materialDatabase();
        writer->setBackgroundMod(position, materialDatabase->modId(*m_mod), 0);
      } else {
        if (writer->needsBackgroundBiomeMod(position)) {
          writer->setBackgroundMod(position, BiomeModId, 0);
        }
      }
    }
  }

  LiquidBrush::LiquidBrush(String const& liquidName, float quantity, bool source)
    : m_liquid(liquidName), m_quantity(quantity), m_source(source) {}

  void LiquidBrush::paint(Vec2I position, Phase phase, DungeonGeneratorWriter* writer) const {
    auto liquidsDatabase = Root::singleton().liquidsDatabase();
    LiquidId liquidId = liquidsDatabase->liquidId(m_liquid);
    LiquidStore liquid(liquidId, m_quantity, 1.0f, m_source);
    if (phase == Phase::WallPhase) {
      writer->requestLiquid(position, liquid);
    }
  }

  void WireBrush::paint(Vec2I position, Phase phase, DungeonGeneratorWriter* writer) const {
    if (phase != Phase::WirePhase)
      return;
    writer->requestWire(position, m_wireGroup, m_partLocal);
  }

  void PlayerStartBrush::paint(Vec2I position, Phase phase, DungeonGeneratorWriter* writer) const {
    if (phase == Phase::NpcPhase)
      writer->setPlayerStart(Vec2F(position));
  }

  InvalidBrush::InvalidBrush(Maybe<String> nameHint) : m_nameHint(nameHint) {}

  void InvalidBrush::paint(Vec2I, Phase, DungeonGeneratorWriter*) const {
    if (m_nameHint)
      Logger::error("Invalid tile '{}'", *m_nameHint);
    else
      Logger::error("Invalid tile");
  }

  bool Tile::canPlace(Vec2I position, DungeonGeneratorWriter* writer) const {
    if (writer->otherDungeonPresent(position))
      return false;
    else if (position[1] < 0)
      return false;
    for (size_t i = 0; i < rules.size(); i++)
      if (!rules[i]->checkTileCanPlace(position, writer))
        return false;
    return true;
  }

  void Tile::place(Vec2I position, Phase phase, DungeonGeneratorWriter* writer) const {
    for (size_t i = 0; i < brushes.size(); i++) {
      brushes[i]->paint(position, phase, writer);
    }
  }

  bool Tile::usesPlaces() const {
    if (brushes.size() == 0)
      return false;
    for (size_t i = 0; i < rules.size(); i++)
      if (rules[i]->overdrawable())
        return false;
    return true;
  }

  bool Tile::modifiesPlaces() const {
    return brushes.size() != 0;
  }

  bool Tile::collidesWithPlaces() const {
    return usesPlaces();
  }

  bool Tile::requiresOpen() const {
    for (size_t i = 0; i < rules.size(); i++)
      if (rules[i]->requiresOpen())
        return true;
    return false;
  }

  bool Tile::requiresSolid() const {
    for (size_t i = 0; i < rules.size(); i++)
      if (rules[i]->requiresSolid())
        return true;
    return false;
  }

  bool Tile::requiresLiquid() const {
    for (size_t i = 0; i < rules.size(); i++)
      if (rules[i]->requiresLiquid())
        return true;
    return false;
  }

  PartConstPtr parsePart(DungeonDefinition* dungeon, Json const& definition, Maybe<ImageTilesetConstPtr> tileset) {
    String kind = definition.get("def").getString(0);
    if (kind == "image") {
      if (tileset.isNothing())
        throw DungeonException("Dungeon parts designed in images require the 'tiles' key in the .dungeon file");
      return make_shared<const Part>(dungeon, definition, make_shared<ImagePartReader>(*tileset));
    } else if (kind == "tmx")
      return make_shared<const Part>(dungeon, definition, make_shared<TMXPartReader>());
    throw DungeonException::format("Unknown dungeon part kind: {}", kind);
  }

  Part::Part(DungeonDefinition* dungeon, Json const& part, PartReaderPtr reader) {
    m_dungeon = dungeon;
    m_name = part.getString("name");
    m_rules = Rule::readRules(part.get("rules"));
    m_chance = part.getFloat("chance", 1);
    if (m_chance <= 0)
      m_chance = 0.0001f;
    m_markDungeonId = part.getBool("markDungeonId", true);
    m_overrideAllowAlways = part.getBool("overrideAllowAlways", false);
    m_minimumThreatLevel = part.optFloat("minimumThreatLevel");
    m_maximumThreatLevel = part.optFloat("maximumThreatLevel");
    m_clearAnchoredObjects = part.getBool("clearAnchoredObjects", true);

    m_reader = reader;
    Json const& def = part.get("def");
    if (def.get(1).type() == Json::Type::String) {
      reader->readAsset(AssetPath::relativeTo(dungeon->directory(), def.get(1).toString()));
    } else {
      for (auto const& asset : def.get(1).iterateArray())
        reader->readAsset(AssetPath::relativeTo(dungeon->directory(), asset.toString()));
    }
    m_size = m_reader->size();
    scanConnectors();
    scanAnchor();
  }

  String const& Part::name() const {
    return m_name;
  }

  Vec2U Part::size() const {
    return m_size;
  }

  Vec2I Part::anchorPoint() const {
    return m_anchorPoint;
  }

  float Part::chance() const {
    return m_chance;
  }

  bool Part::markDungeonId() const {
    return m_markDungeonId;
  }

  Maybe<float> Part::minimumThreatLevel() const {
    return m_minimumThreatLevel;
  }

  Maybe<float> Part::maximumThreatLevel() const {
    return m_maximumThreatLevel;
  }

  bool Part::clearAnchoredObjects() const {
    return m_clearAnchoredObjects;
  }

  int Part::placementLevelConstraint() const {
    Vec2I air = {0, size().y()};
    Vec2I ground = {0, 0};
    Vec2I liquid = {0, 0};
    m_reader->forEachTile([&ground, &air, &liquid](Vec2I tilePos, Tile const& tile) -> bool {
      for (auto const& rule : tile.rules) {
        if (is<WorldGenMustContainSolidRule>(rule) && tilePos.y() > ground.y()) {
          ground = tilePos;
        }
        if (is<WorldGenMustContainAirRule>(rule) && tilePos.y() < air.y()) {
          air = tilePos;
        }
        if ((is<WorldGenMustContainLiquidRule>(rule) || is<WorldGenMustNotContainLiquidRule>(rule)) && tilePos.y() > liquid.y()) {
          liquid = tilePos;
        }
      }
      return false;
    });
    ground[1] = max(ground[1], liquid[1]);
    if (air.y() < ground.y())
      throw DungeonException("Invalid ground vs air contraint. Ground at: " + toString(ground.y()) + " Air at: "
          + toString(air.y())
          + " Pixels: highest ground:"
          + toString(ground)
          + " lowest air:"
          + toString(air));
    return air.y();
  }

  bool Part::ignoresPartMaximum() const {
    for (size_t i = 0; i < m_rules.size(); i++)
      if (m_rules[i]->ignorePartMaximum())
        return true;
    return false;
  }

  bool Part::allowsPlacement(int currentPlacementCount) const {
    for (size_t i = 0; i < m_rules.size(); i++)
      if (!m_rules[i]->allowSpawnCount(currentPlacementCount))
        return false;
    return true;
  }

  List<ConnectorConstPtr> const& Part::connections() const {
    return m_connections;
  }

  bool Part::doesNotConnectTo(Part* part) const {
    for (size_t i = 0; i < m_rules.size(); i++)
      if (m_rules[i]->doesNotConnectToPart(part->name()))
        return true;
    for (size_t i = 0; i < part->m_rules.size(); i++)
      if (part->m_rules[i]->doesNotConnectToPart(m_name))
        return true;
    return false;
  }

  bool Part::checkPartCombinationsAllowed(StringMap<int> const& placementCounter) const {
    for (size_t i = 0; i < m_rules.size(); i++)
      if (!m_rules[i]->checkPartCombinationsAllowed(placementCounter))
        return false;
    return true;
  }

  bool Part::collidesWithPlaces(Vec2I pos, Set<Vec2I>& places) const {
    if (m_overrideAllowAlways)
      return true;

    bool result = false;
    m_reader->forEachTile([&result, pos, &places](Vec2I tilePos, Tile const& tile) -> bool {
      if (tile.collidesWithPlaces())
        if (places.contains(pos + tilePos)) {
          Logger::debug("Tile collided with place at {}", pos + tilePos);
          result = true;
          return true;
        }
      return false;
    });

    return result;
  }

  bool Part::canPlace(Vec2I pos, DungeonGeneratorWriter* writer) const {
    if (m_overrideAllowAlways)
      return true;

    // Speed up repeated failing calls by first checking the tile that failed
    // last time (if it did).
    bool result = true;
    m_reader->forEachTile([&result, pos, writer](Vec2I tilePos, Tile const& tile) -> bool {
      Vec2I position = pos + tilePos;
      if (!tile.canPlace(position, writer)) {
        result = false;
        return true;
      }
      return false;
    });

    return result;
  }

  void Part::place(Vec2I pos, Set<Vec2I> const& places, DungeonGeneratorWriter* writer) const {
    placePhase(pos, Phase::ClearPhase, places, writer);
    placePhase(pos, Phase::WallPhase, places, writer);
    placePhase(pos, Phase::ModsPhase, places, writer);
    placePhase(pos, Phase::ObjectPhase, places, writer);
    placePhase(pos, Phase::BiomeTreesPhase, places, writer);
    placePhase(pos, Phase::BiomeItemsPhase, places, writer);
    placePhase(pos, Phase::WirePhase, places, writer);
    placePhase(pos, Phase::ItemPhase, places, writer);
    placePhase(pos, Phase::NpcPhase, places, writer);
    placePhase(pos, Phase::DungeonIdPhase, places, writer);
  }

  void Part::forEachTile(TileCallback const& callback) const {
    m_reader->forEachTile(callback);
  }

  void Part::placePhase(Vec2I pos, Phase phase, Set<Vec2I> const& places, DungeonGeneratorWriter* writer) const {
    m_reader->forEachTile([&places, pos, phase, writer](Vec2I tilePos, Tile const& tile) -> bool {
      Vec2I position = pos + tilePos;
      if (tile.collidesWithPlaces() || !places.contains(position)) {
        try {
          tile.place(position, phase, writer);
        } catch (std::exception const&) {
          Logger::error("Error at map position {}:", tilePos);
          throw;
        }
      }
      return false;
    });
  }

  bool Part::tileUsesPlaces(Vec2I pos) const {
    bool result = false;
    m_reader->forEachTileAt(pos,
        [&result](Vec2I, Tile const& tile) -> bool {
          if (tile.usesPlaces()) {
            result = true;
            return true;
          }
          return false;
        });
    return result;
  }

  Direction Part::pickByEdge(Vec2I position, Vec2U size) const {
    int dxa = position[0];
    int dxb = size[0] - position[0];
    int dya = position[1];
    int dyb = size[1] - position[1];

    int m = min(min(dxa, dxb), min(dya, dyb));
    if (dxa == m)
      return Direction::Left;
    if (dxb == m)
      return Direction::Right;
    if (dya == m)
      return Direction::Down;
    if (dyb == m)
      return Direction::Up;
    throw DungeonException("Ambiguous direction");
  }

  Direction Part::pickByNeighbours(Vec2I pos) const {
    int x = pos.x(), y = pos.y();

    // if on a border use that, corners use the left/right direction
    if (x == 0)
      return Direction::Left;
    if (x == (int)size().x() - 1)
      return Direction::Right;
    if (y == 0)
      return Direction::Down;
    if (y == (int)size().y() - 1)
      return Direction::Up;

    // scans around the connector, the direction where it finds a solid is where
    // it assume the
    // connection comes from

    if (tileUsesPlaces({x + 1, y}) && !tileUsesPlaces({x - 1, y}))
      return Direction::Left;

    if (tileUsesPlaces({x - 1, y}) && !tileUsesPlaces({x + 1, y}))
      return Direction::Right;

    if (tileUsesPlaces({x, y + 1}) && !tileUsesPlaces({x, y - 1}))
      return Direction::Down;

    if (tileUsesPlaces({x, y - 1}) && !tileUsesPlaces({x, y + 1}))
      return Direction::Up;

    return Direction::Unknown;
  }

  void Part::scanConnectors() {
    try {
      m_reader->forEachTile([this](Vec2I position, Tile const& tile) -> bool {
        if (tile.connector.isValid()) {
          auto d = tile.connector->direction;
          if (d == Direction::Unknown)
            d = pickByNeighbours(position);
          if (d == Direction::Unknown)
            d = pickByEdge(position, m_size);
          Logger::debug("Found connector on {} at {} group {} direction {}", m_name, position, tile.connector->value, (int)d);
          m_connections.append(make_shared<Connector>(this, tile.connector->value, tile.connector->forwardOnly, d, position));
        }

        return false;
      });
    } catch (std::exception& e) {
      throw DungeonException(strf("Exception {} in connector {}", outputException(e, true), m_name));
    }
  }

  void Part::scanAnchor() {
    int cx, cy, cc;
    cx = cy = cc = 0;
    int lowestAir = m_size[1];
    int highestGound = -1;
    int highestLiquid = -1;
    try {
      m_reader->forEachTile([&](Vec2I pos, Tile const& tile) -> bool {
        int x = pos.x(), y = pos.y();
        if (tile.collidesWithPlaces()) {
          cx += x;
          cy += y;
          cc++;
        }
        if (tile.requiresOpen()) {
          if ((int)y < lowestAir)
            lowestAir = y;
        }
        if (tile.requiresSolid()) {
          if ((int)y > highestGound)
            highestGound = y;
        }
        if (tile.requiresLiquid()) {
          if ((int)y > highestLiquid)
            highestLiquid = y;
        }
        return false;
      });
    } catch (std::exception& e) {
      throw DungeonException(strf("Exception {} in part {}", outputException(e, true), m_name));
    }

    highestGound = max(highestGound, highestLiquid);
    if (highestGound == -1)
      highestGound = lowestAir - 1;

    if (lowestAir == (int)m_size[1])
      lowestAir = highestGound + 1;

    if (cc == 0) {
      cx = m_size[0] / 2;
      cy = m_size[1] / 2;
    } else {
      cx /= cc;
      cy /= cc;
    }

    if (highestGound != -1)
      cy = highestGound + 1;

    m_anchorPoint = {cx, cy};
  }

  bool WorldGenMustContainSolidRule::checkTileCanPlace(Vec2I position, DungeonGeneratorWriter* writer) const {
    return writer->checkSolid(position, layer);
  }

  bool WorldGenMustContainAirRule::checkTileCanPlace(Vec2I position, DungeonGeneratorWriter* writer) const {
    return writer->checkOpen(position, layer);
  }
  
  bool WorldGenMustContainLiquidRule::checkTileCanPlace(Vec2I position, DungeonGeneratorWriter * writer) const {
    return writer->checkLiquid(position);
  }

  bool WorldGenMustNotContainLiquidRule::checkTileCanPlace(Vec2I position, DungeonGeneratorWriter * writer) const {
    return !writer->checkLiquid(position);
  }

  Connector::Connector(Part* part, String value, bool forwardOnly, Direction direction, Vec2I offset)
    : m_value(value), m_forwardOnly(forwardOnly), m_direction(direction), m_offset(offset), m_part(part) {}

  bool Connector::connectsTo(ConnectorConstPtr connector) const {
    if (m_forwardOnly)
      return false;
    if (m_value != connector->m_value)
      return false;
    if (m_direction == Direction::Any || connector->m_direction == Direction::Any)
      return true;
    if (m_direction != flipDirection(connector->m_direction))
      return false;
    return true;
  }

  String Connector::value() const {
    return m_value;
  }

  Vec2I Connector::positionAdjustment() const {
    if (m_direction == Direction::Any)
      return Vec2I(0, 0);
    if (m_direction == Direction::Left)
      return Vec2I(-1, 0);
    if (m_direction == Direction::Right)
      return Vec2I(1, 0);
    if (m_direction == Direction::Up)
      return Vec2I(0, 1);
    starAssert(m_direction == Direction::Down);
    return Vec2I(0, -1);
  }

  Part* Connector::part() const {
    return m_part;
  }

  Vec2I Connector::offset() const {
    return m_offset;
  }

  DungeonGeneratorWriter::DungeonGeneratorWriter(DungeonGeneratorWorldFacadePtr facade, Maybe<int> terrainMarkingSurfaceLevel, Maybe<int> terrainSurfaceSpaceExtends)
    : m_facade(facade), m_terrainMarkingSurfaceLevel(terrainMarkingSurfaceLevel), m_terrainSurfaceSpaceExtends(terrainSurfaceSpaceExtends) {
    m_currentBounds.setMin(Vec2I{std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max()});
    m_currentBounds.setMax(Vec2I{std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min()});
  }

  Vec2I DungeonGeneratorWriter::wrapPosition(Vec2I const& pos) const {
    return m_facade->getWorldGeometry().xwrap(pos);
  }

  void DungeonGeneratorWriter::setMarkDungeonId(Maybe<DungeonId> dungeonId) {
    m_markDungeonId = dungeonId;
  }

  void DungeonGeneratorWriter::requestLiquid(Vec2I const& pos, LiquidStore const& liquid) {
    m_pendingLiquids[pos] = liquid;
  }

  void DungeonGeneratorWriter::setLiquid(Vec2I const& pos, LiquidStore const& liquid) {
    m_liquids[pos] = liquid;
    markPosition(pos);
  }

  void DungeonGeneratorWriter::setForegroundMaterial(Vec2I const& position, MaterialId material, MaterialHue hueshift, MaterialColorVariant colorVariant) {
    m_foregroundMaterial[position] = {material, hueshift, colorVariant};
    markPosition(position);
  }

  void DungeonGeneratorWriter::setBackgroundMaterial(Vec2I const& position, MaterialId material, MaterialHue hueshift, MaterialColorVariant colorVariant) {
    m_backgroundMaterial[position] = {material, hueshift, colorVariant};
    markPosition(position);
  }

  void DungeonGeneratorWriter::setForegroundMod(Vec2I const& position, ModId mod, MaterialHue hueshift) {
    m_foregroundMod[position] = {mod, hueshift};
    markPosition(position);
  }

  void DungeonGeneratorWriter::setBackgroundMod(Vec2I const& position, ModId mod, MaterialHue hueshift) {
    m_backgroundMod[position] = {mod, hueshift};
    markPosition(position);
  }

  bool DungeonGeneratorWriter::needsForegroundBiomeMod(Vec2I const& position) {
    if (!m_foregroundMaterial.contains(position))
      return false;
    if (!isBiomeMaterial(m_foregroundMaterial[position].material))
      return false;
    Vec2I abovePosition(position.x(), position.y() + 1);
    if (m_foregroundMaterial.contains(abovePosition))
      if (m_foregroundMaterial[abovePosition].material != EmptyMaterialId)
        return false;
    return true;
  }

  bool DungeonGeneratorWriter::needsBackgroundBiomeMod(Vec2I const& position) {
    if (!m_backgroundMaterial.contains(position))
      return false;
    if (!isBiomeMaterial(m_backgroundMaterial[position].material))
      return false;
    Vec2I abovePosition(position.x(), position.y() + 1);
    if (m_backgroundMaterial.contains(abovePosition))
      if (m_backgroundMaterial[abovePosition].material != EmptyMaterialId)
        return false;
    if (m_foregroundMaterial.contains(abovePosition))
      if (m_foregroundMaterial[abovePosition].material != EmptyMaterialId)
        return false;
    return true;
  }

  void DungeonGeneratorWriter::placeObject(Vec2I const& pos, String const& objectType, Star::Direction direction, Json const& parameters) {
    m_objects[pos] = {objectType, direction, parameters};
    markPosition(pos);
  }

  void DungeonGeneratorWriter::placeVehicle(Vec2F const& pos, String const& vehicleName, Json const& parameters) {
    m_vehicles[pos] = make_pair(vehicleName, parameters);
    markPosition(pos);
  }

  void DungeonGeneratorWriter::placeSurfaceBiomeItems(Vec2I const& pos) {
    m_biomeItems.insert(pos);
    markPosition(pos);
  }

  void DungeonGeneratorWriter::placeBiomeTree(Vec2I const& pos) {
    m_biomeTrees.insert(pos);
    markPosition(pos);
  }

  void DungeonGeneratorWriter::addDrop(Vec2F const& position, ItemDescriptor const& item) {
    m_drops[position] = item;
    markPosition(position);
  }

  void DungeonGeneratorWriter::requestWire(Vec2I const& position, String const& wireGroup, bool partLocal) {
    if (partLocal)
      m_openLocalWires[wireGroup].add(position);
    else
      m_globalWires[wireGroup].add(position);
  }

  void DungeonGeneratorWriter::spawnNpc(Vec2F const& position, Json const& definition) {
    m_npcs[position] = definition;
    markPosition(position);
  }

  void DungeonGeneratorWriter::spawnStagehand(Vec2F const& position, Json const& definition) {
    m_stagehands[position] = definition;
    markPosition(position);
  }

  void DungeonGeneratorWriter::setPlayerStart(Vec2F const& startPosition) {
    m_facade->setPlayerStart(startPosition);
  }

  bool DungeonGeneratorWriter::checkSolid(Vec2I position, TileLayer layer) {
    if (m_terrainMarkingSurfaceLevel)
      return position.y() < *m_terrainMarkingSurfaceLevel;
    return m_facade->checkSolid(position, layer);
  }

  bool DungeonGeneratorWriter::checkOpen(Vec2I position, TileLayer layer) {
    if (m_terrainMarkingSurfaceLevel)
      return position.y() >= *m_terrainMarkingSurfaceLevel;
    return m_facade->checkOpen(position, layer);
  }

  bool DungeonGeneratorWriter::checkLiquid(Vec2I const& position) {
    return m_facade->checkOceanLiquid(position);
  }

  bool DungeonGeneratorWriter::otherDungeonPresent(Vec2I position) {
    return m_facade->getDungeonIdAt(position) != NoDungeonId;
  }

  void DungeonGeneratorWriter::setDungeonId(Vec2I const& pos, DungeonId dungeonId) {
    m_dungeonIds[pos] = dungeonId;
  }

  void DungeonGeneratorWriter::markPosition(Vec2F const& pos) {
    markPosition(Vec2I(pos.floor()));
  }

  void DungeonGeneratorWriter::markPosition(Vec2I const& pos) {
    m_currentBounds.combine(pos);
    if (m_markDungeonId)
      m_dungeonIds[pos] = *m_markDungeonId;
  }

  void DungeonGeneratorWriter::clearTileEntities(RectI const& bounds, Set<Vec2I> const& positions, bool clearAnchoredObjects) {
    m_facade->clearTileEntities(bounds, positions, clearAnchoredObjects);
  }

  void DungeonGeneratorWriter::finishPart() {
    for (auto& entries : m_openLocalWires)
      m_localWires.append(entries.second);
    m_openLocalWires.clear();

    if (m_currentBounds.xMin() > m_currentBounds.xMax())
      return;
    m_boundingBoxes.push_back(m_currentBounds);
    m_currentBounds.setMin(Vec2I{std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max()});
    m_currentBounds.setMax(Vec2I{std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min()});
  }

  void DungeonGeneratorWriter::flushLiquid() {
    // For each liquid type, find each contiguous region of liquid, then
    // pressurize that region based on the highest position in the region

    Map<LiquidId, Set<Vec2I>> unpressurizedLiquids;
    for (auto& p : m_pendingLiquids)
      unpressurizedLiquids[p.second.liquid].add(p.first);

    for (auto& liquidPair : unpressurizedLiquids) {
      auto& unpressurized = liquidPair.second;
      while (!unpressurized.empty()) {
        // Start with the first unpressurized block as the open set.
        Vec2I firstBlock = unpressurized.takeFirst();
        List<Vec2I> openSet = {firstBlock};
        Set<Vec2I> contiguousRegion = {firstBlock};

        // For each element in the previous open set, add all connected blocks
        // in
        // the unpressurized set to the new open set and to the total contiguous
        // region, taking them from the unpressurized set.
        while (!openSet.empty()) {
          auto oldOpenSet = take(openSet);
          for (auto const& p : oldOpenSet) {
            for (auto dir : {Vec2I(1, 0), Vec2I(-1, 0), Vec2I(0, 1), Vec2I(0, -1)}) {
              Vec2I pos = p + dir;
              if (unpressurized.remove(pos)) {
                contiguousRegion.add(pos);
                openSet.append(pos);
              }
            }
          }
        }

        // Once we have found no more blocks in the unpressurized set to add to
        // the open set, then we have taken a contiguous region out of the
        // unpressurized set.  Pressurize it based on the highest point.
        int highestPoint = lowest<int>();
        for (auto const& p : contiguousRegion)
          highestPoint = max(highestPoint, p[1]);
        for (auto const& p : contiguousRegion)
          m_pendingLiquids[p].pressure = 1.0f + highestPoint - p[1];
      }
    }

    for (auto& p : m_pendingLiquids)
      setLiquid(p.first, p.second);

    m_pendingLiquids.clear();
  }

  void DungeonGeneratorWriter::flush() {
    auto geometry = m_facade->getWorldGeometry();
    auto displace = [&](Vec2I pos) -> Vec2I { return geometry.xwrap(pos); };
    auto displaceF = [&](Vec2F pos) -> Vec2F { return geometry.xwrap(pos); };

    PolyF::VertexList terrainBlendingVertexes;
    PolyF::VertexList spaceBlendingVertexes;
    for (auto bb : m_boundingBoxes) {
      m_facade->markRegion(bb);

      if (m_terrainMarkingSurfaceLevel) {
        // Mark the regions of the dungeon above the dungeon surface as needing
        // space, and the regions below the surface as needing terrain
        if (bb.yMin() < *m_terrainMarkingSurfaceLevel) {
          RectI lower = bb;
          lower.setYMax(min(lower.yMax(), *m_terrainMarkingSurfaceLevel));
          terrainBlendingVertexes.append(Vec2F(lower.xMin(), lower.yMin()));
          terrainBlendingVertexes.append(Vec2F(lower.xMax(), lower.yMin()));
          terrainBlendingVertexes.append(Vec2F(lower.xMin(), lower.yMax()));
          terrainBlendingVertexes.append(Vec2F(lower.xMax(), lower.yMax()));
        }

        if (bb.yMax() > *m_terrainMarkingSurfaceLevel) {
          RectI upper = bb;
          upper.setYMin(max(upper.yMin(), *m_terrainMarkingSurfaceLevel));
          spaceBlendingVertexes.append(Vec2F(upper.xMin(), upper.yMin()));
          spaceBlendingVertexes.append(Vec2F(upper.xMax(), upper.yMin()));
          spaceBlendingVertexes.append(Vec2F(upper.xMin(), upper.yMax() + m_terrainSurfaceSpaceExtends.value(0)));
          spaceBlendingVertexes.append(Vec2F(upper.xMax(), upper.yMax() + m_terrainSurfaceSpaceExtends.value(0)));
        }
      }
    }

    if (!terrainBlendingVertexes.empty())
      m_facade->markTerrain(PolyF::convexHull(terrainBlendingVertexes));
    if (!spaceBlendingVertexes.empty())
      m_facade->markSpace(PolyF::convexHull(spaceBlendingVertexes));

    for (auto iter = m_backgroundMaterial.begin(); iter != m_backgroundMaterial.end(); iter++)
      m_facade->setBackgroundMaterial(displace(iter->first), iter->second.material, iter->second.hueshift, iter->second.colorVariant);
    for (auto iter = m_foregroundMaterial.begin(); iter != m_foregroundMaterial.end(); iter++)
      m_facade->setForegroundMaterial(displace(iter->first), iter->second.material, iter->second.hueshift, iter->second.colorVariant);
    for (auto iter = m_foregroundMod.begin(); iter != m_foregroundMod.end(); iter++)
      m_facade->setForegroundMod(displace(iter->first), iter->second.mod, iter->second.hueshift);
    for (auto iter = m_backgroundMod.begin(); iter != m_backgroundMod.end(); iter++)
      m_facade->setBackgroundMod(displace(iter->first), iter->second.mod, iter->second.hueshift);

    List<Vec2I> sortedPositions = m_objects.keys();
    sortByComputedValue(sortedPositions, [](Vec2I pos) { return pos[1] + pos[0] / 1000.0f; });
    for (auto pos : sortedPositions) {
      auto& object = m_objects[pos];
      m_facade->placeObject(displace(pos), object.objectName, object.direction, object.parameters);
    }

    for (auto entry : m_vehicles) {
      String vehicleName;
      Json parameters;
      tie(vehicleName, parameters) = entry.second;
      m_facade->placeVehicle(displaceF(entry.first), vehicleName, parameters);
    }

    sortedPositions = List<Vec2I>::from(m_biomeTrees);
    sortByComputedValue(sortedPositions, [](Vec2I pos) { return pos[1] + pos[0] / 1000.0f; });
    for (auto pos : sortedPositions) {
      m_facade->placeBiomeTree(pos);
    }

    sortedPositions = List<Vec2I>::from(m_biomeItems);
    sortByComputedValue(sortedPositions, [](Vec2I pos) { return pos[1] + pos[0] / 1000.0f; });
    for (auto pos : sortedPositions) {
      m_facade->placeSurfaceBiomeItems(pos);
    }

    for (auto& npc : m_npcs) {
      m_facade->spawnNpc(displaceF(npc.first), npc.second);
    }

    for (auto& stagehand : m_stagehands) {
      m_facade->spawnStagehand(displaceF(stagehand.first), stagehand.second);
    }

    for (auto& wires : m_globalWires) {
      List<Vec2I> wireGroup;
      for (auto& pos : wires.second)
        wireGroup.append(displace(pos));
      m_facade->connectWireGroup(wireGroup);
    }
    for (auto& wires : m_localWires) {
      List<Vec2I> wireGroup;
      for (auto& pos : wires)
        wireGroup.append(displace(pos));
      m_facade->connectWireGroup(wireGroup);
    }

    for (auto iter = m_drops.begin(); iter != m_drops.end(); iter++)
      m_facade->addDrop(displaceF(iter->first), iter->second);

    for (auto iter = m_liquids.begin(); iter != m_liquids.end(); iter++)
      m_facade->setLiquid(displace(iter->first), iter->second);

    for (auto const& dungeonId : m_dungeonIds)
      m_facade->setDungeonIdAt(dungeonId.first, dungeonId.second);
  }

  List<RectI> DungeonGeneratorWriter::boundingBoxes() const {
    return m_boundingBoxes;
  }

  void DungeonGeneratorWriter::reset() {
    m_currentBounds.setMin(Vec2I{std::numeric_limits<int32_t>::max(), std::numeric_limits<int32_t>::max()});
    m_currentBounds.setMax(Vec2I{std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min()});

    m_pendingLiquids.clear();
    m_foregroundMaterial.clear();
    m_backgroundMaterial.clear();
    m_foregroundMod.clear();
    m_backgroundMod.clear();
    m_objects.clear();
    m_biomeTrees.clear();
    m_biomeItems.clear();
    m_drops.clear();
    m_npcs.clear();
    m_stagehands.clear();
    m_liquids.clear();
    m_globalWires.clear();
    m_localWires.clear();
    m_openLocalWires.clear();
    m_boundingBoxes.clear();
  }
}

DungeonDefinitions::DungeonDefinitions() : m_paths(), m_cacheMutex(), m_definitionCache(DefinitionsCacheSize) {
  auto assets = Root::singleton().assets();

  for (auto file : assets->scan(".dungeon")) {
    Json dungeon = assets->json(file);
    m_paths.insert(dungeon.get("metadata").getString("name"), file);
  }
}

DungeonDefinitionConstPtr DungeonDefinitions::get(String const& name) const {
  MutexLocker locker(m_cacheMutex);
  return m_definitionCache.get(name,
      [this](String const& name) -> DungeonDefinitionPtr {
        if (auto path = m_paths.maybe(name))
          return readDefinition(*path);
        throw DungeonException::format("Unknown dungeon: '{}'", name);
      });
}

JsonObject DungeonDefinitions::getMetadata(String const& name) const {
  auto definition = get(name);
  return definition->metadata();
}

DungeonDefinitionPtr DungeonDefinitions::readDefinition(String const& path) {
  try {
    auto assets = Root::singleton().assets();
    return make_shared<DungeonDefinition>(assets->json(path).toObject(), AssetPath::directory(path));
  } catch (std::exception const& e) {
    throw DungeonException::format("Error loading dungeon '{}': {}", path, outputException(e, false));
  }
}

DungeonDefinition::DungeonDefinition(JsonObject const& definition, String const& directory) {
  m_directory = directory;
  m_metadata = definition.get("metadata").toObject();
  m_name = m_metadata.get("name").toString();
  m_displayName = m_metadata.contains("displayName") ? m_metadata.get("displayName").toString() : "";
  m_species = m_metadata.get("species").toString();
  m_isProtected = m_metadata.contains("protected") ? m_metadata.get("protected").toBool() : false;
  if (m_metadata.contains("rules"))
    m_rules = Dungeon::Rule::readRules(m_metadata.get("rules"));

  m_maxRadius = m_metadata.value("maxRadius", 100).toInt();
  m_maxParts = m_metadata.value("maxParts", 100).toInt();
  m_extendSurfaceFreeSpace = m_metadata.value("extendSurfaceFreeSpace", 0).toInt();

  m_anchors = jsonToStringList(m_metadata.get("anchor"));

  auto tileset = definition.maybe("tiles").apply([](Json const& tileset) {
      return make_shared<const Dungeon::ImageTileset>(tileset);
    });

  for (auto const& partsDefMap : definition.get("parts").iterateArray()) {
    Dungeon::PartConstPtr part = parsePart(this, partsDefMap, tileset);
    if (m_parts.contains(part->name()))
      throw DungeonException::format("Duplicate dungeon part name: {}", part->name());
    m_parts.insert(part->name(), part);
  }

  if (m_metadata.contains("gravity"))
    m_gravity = m_metadata.get("gravity").toFloat();

  if (m_metadata.contains("breathable"))
    m_breathable = m_metadata.get("breathable").toBool();
}

JsonObject DungeonDefinition::metadata() const {
  return m_metadata;
}

String DungeonDefinition::directory() const {
  return m_directory;
}

String DungeonDefinition::name() const {
  return m_name;
}

String DungeonDefinition::displayName() const {
  return m_displayName;
}

bool DungeonDefinition::isProtected() const {
  return m_isProtected;
}

Maybe<float> DungeonDefinition::gravity() const {
  return m_gravity;
}

Maybe<bool> DungeonDefinition::breathable() const {
  return m_breathable;
}

StringMap<Dungeon::PartConstPtr> const& DungeonDefinition::parts() const {
  return m_parts;
}

List<String> const& DungeonDefinition::anchors() const {
  return m_anchors;
}

Maybe<Json> const& DungeonDefinition::optTileset() const {
  return m_tileset;
}

int DungeonDefinition::maxParts() const {
  return m_maxParts;
}

int DungeonDefinition::maxRadius() const {
  return m_maxRadius;
}

int DungeonDefinition::extendSurfaceFreeSpace() const {
  return m_extendSurfaceFreeSpace;
}

DungeonGenerator::DungeonGenerator(String const& dungeonName, uint64_t seed, float threatLevel, Maybe<DungeonId> dungeonId)
  : m_rand(seed), m_threatLevel(threatLevel), m_dungeonId(dungeonId) {
  m_def = Root::singleton().dungeonDefinitions()->get(dungeonName);
}

Maybe<pair<List<RectI>, Set<Vec2I>>> DungeonGenerator::generate(DungeonGeneratorWorldFacadePtr facade, Vec2I position, bool markSurfaceAndTerrain, bool forcePlacement) {
  try {
    Dungeon::DungeonGeneratorWriter writer(facade, markSurfaceAndTerrain ? position[1] : Maybe<int>(), m_def->extendSurfaceFreeSpace());

    Logger::debug(forcePlacement ? "Forcing generation of dungeon {}" : "Generating dungeon {}", m_def->name());

    Dungeon::PartConstPtr anchor = pickAnchor();
    if (!anchor) {
      Logger::error("No valid anchor piece found for dungeon at {}", position);
      return {};
    }

    auto pos = position + Vec2I(0, -anchor->placementLevelConstraint());
    if (forcePlacement || anchor->canPlace(pos, &writer)) {
      Logger::info("Placing dungeon at {}", position);
      return buildDungeon(anchor, pos, &writer, forcePlacement);
    } else {
      Logger::debug("Failed to place a dungeon at {}", position);
      return {};
    }
  } catch (std::exception const& e) {
    throw DungeonException(strf("Error generating dungeon named '{}'", m_def->name()), e);
  }
}

pair<List<RectI>, Set<Vec2I>> DungeonGenerator::buildDungeon(Dungeon::PartConstPtr anchor, Vec2I basePos, Dungeon::DungeonGeneratorWriter* writer, bool forcePlacement) {
  writer->reset();

  Deque<std::pair<Dungeon::Part const*, Vec2I>> openSet;
  StringMap<int> placementCounter;
  Set<Vec2I> modifiedTiles;
  Set<Vec2I> preserveTiles;
  int piecesPlaced = 0;

  Logger::debug("Placing dungeon entrance at {}", basePos);

  auto placePart = [&](Dungeon::Part const* part, Vec2I const& placePos) {
      Set<Vec2I> clearTileEntityPositions;
      part->forEachTile([&](Vec2I tilePos, Dungeon::Tile const& tile) -> bool {
          if (tile.modifiesPlaces())
            clearTileEntityPositions.insert(writer->wrapPosition(placePos + tilePos));
          return false;
        });
      auto partBounds = RectI::withSize(placePos, Vec2I(part->size()));
      writer->clearTileEntities(partBounds, clearTileEntityPositions, part->clearAnchoredObjects());

      if (part->markDungeonId())
        writer->setMarkDungeonId(m_dungeonId);
      else
        writer->setMarkDungeonId();

      part->place(placePos, preserveTiles, writer);
      writer->finishPart();

      part->forEachTile([&](Vec2I tilePos, Dungeon::Tile const& tile) -> bool {
          if (tile.usesPlaces())
            preserveTiles.insert(placePos + tilePos);
          if (tile.modifiesPlaces())
            modifiedTiles.insert(placePos + tilePos);
          return false;
        });

      openSet.append({part, placePos});

      placementCounter[part->name()]++;
      piecesPlaced++;

      Logger::debug("placed {}", part->name());
    };

  placePart(anchor.get(), basePos);

  Vec2I origin = basePos + Vec2I(anchor->size()) / 2;

  Set<Vec2I> closedConnectors;
  while (openSet.size()) {
    Dungeon::Part const* parentPart = openSet.first().first;
    Vec2I parentPos = openSet.first().second;
    openSet.takeFirst();
    Logger::debug("Trying to add part {} at {} connectors: {}", parentPart->name(), parentPos, parentPart->connections().size());
    for (size_t i = 0; i < parentPart->connections().size(); i++) {
      auto connector = parentPart->connections()[i];
      Vec2I connectorPos = parentPos + connector->offset();
      if (closedConnectors.contains(connectorPos))
        continue;
      List<Dungeon::ConnectorConstPtr> options = findConnectablePart(connector);
      while (options.size()) {
        Dungeon::ConnectorConstPtr option = chooseOption(options, m_rand);
        Logger::debug("Trying part {}", option->part()->name());
        Vec2I partPos = connectorPos - option->offset() + option->positionAdjustment();
        Vec2I optionPos = connectorPos + option->positionAdjustment();
        if (!option->part()->ignoresPartMaximum()) {
          if (piecesPlaced >= m_def->maxParts())
            continue;

          if ((partPos - origin).magnitude() > m_def->maxRadius()) {
            Logger::debug("out of range. {} ... {}", partPos, origin);
            continue;
          }
        }
        if (!option->part()->allowsPlacement(placementCounter[option->part()->name()])) {
          Logger::debug("part failed in allowsPlacement");
          continue;
        }
        if (!option->part()->checkPartCombinationsAllowed(placementCounter)) {
          Logger::debug("part failed in checkPartCombinationsAllowed");
          continue;
        }
        if (option->part()->collidesWithPlaces(partPos, preserveTiles)) {
          Logger::debug("part failed in collidesWithPlaces");
          continue;
        }
        if (option->part()->minimumThreatLevel() && m_threatLevel < *option->part()->minimumThreatLevel()) {
          Logger::debug("part failed in minimumThreatLevel");
          continue;
        }
        if (option->part()->maximumThreatLevel() && m_threatLevel > *option->part()->maximumThreatLevel()) {
          Logger::debug("part failed in maximumThreatLevel");
          continue;
        }
        if (forcePlacement || option->part()->canPlace(partPos, writer)) {
          placePart(option->part(), partPos);
          closedConnectors.add(connectorPos);
          closedConnectors.add(optionPos);
          break;
        } else {
          Logger::debug("part failed in canPlace");
        }
      }
    }
  }
  Logger::debug("Settling dungeon water.");
  writer->flushLiquid();
  Logger::debug("Flushing dungeon into the worldgen.");
  writer->flush();

  return {writer->boundingBoxes(), modifiedTiles};
}

Dungeon::PartConstPtr DungeonGenerator::pickAnchor() {
  auto validAnchors = m_def->anchors().filtered([this](String const& anchorName) {
    auto anchorPart = m_def->parts().get(anchorName);
    return (!anchorPart->minimumThreatLevel() || m_threatLevel >= *anchorPart->minimumThreatLevel())
        && (!anchorPart->maximumThreatLevel() || m_threatLevel <= *anchorPart->maximumThreatLevel());
  });

  if (validAnchors.empty())
    return {};

  return m_def->parts().get(m_rand.randFrom(validAnchors));
}

List<Dungeon::ConnectorConstPtr> DungeonGenerator::findConnectablePart(Dungeon::ConnectorConstPtr connector) const {
  List<Dungeon::ConnectorConstPtr> result;
  for (auto const& partPair : m_def->parts()) {
    if (partPair.second->doesNotConnectTo(connector->part()))
      continue;
    for (auto const& connection : partPair.second->connections()) {
      if (connection->connectsTo(connector))
        result.append(connection);
    }
  }
  return result;
}

DungeonDefinitionConstPtr DungeonGenerator::definition() const {
  return m_def;
}

}

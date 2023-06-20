#include "StarAssets.hpp"
#include "StarCasting.hpp"
#include "StarRoot.hpp"
#include "StarTilesetDatabase.hpp"

namespace Star {

namespace Tiled {
  using namespace Dungeon;

  EnumMap<TileLayer> const LayerNames{{TileLayer::Foreground, "front"}, {TileLayer::Background, "back"}};

  Properties::Properties() : m_properties(JsonObject{}) {}

  Properties::Properties(Json const& json) : m_properties(json) {}

  Json Properties::toJson() const {
    return m_properties;
  }

  Properties Properties::inherit(Json const& properties) const {
    return jsonMerge(properties, m_properties);
  }

  Properties Properties::inherit(Properties const& properties) const {
    return jsonMerge(properties.m_properties, m_properties);
  }

  bool Properties::contains(String const& name) const {
    return m_properties.contains(name);
  }

  Maybe<BrushConstPtr> getClearBrush(bool value, Tiled::Properties&) {
    if (value)
      return Maybe<BrushConstPtr>(as<Brush>(make_shared<const ClearBrush>()));
    return {};
  }

  BrushConstPtr getFrontBrush(String const& materialName, Tiled::Properties& properties) {
    Maybe<float> hueshift = properties.opt<float>("hueshift");
    Maybe<MaterialColorVariant> colorVariant = properties.opt<size_t>("colorVariant");
    Maybe<String> mod = properties.opt<String>("mod");
    Maybe<float> modhueshift = properties.opt<float>("modhueshift");

    return make_shared<const FrontBrush>(materialName, mod, hueshift, modhueshift, colorVariant);
  }

  BrushConstPtr getBackBrush(String const& materialName, Tiled::Properties& properties) {
    Maybe<float> hueshift = properties.opt<float>("hueshift");
    Maybe<MaterialColorVariant> colorVariant = properties.opt<size_t>("colorVariant");
    Maybe<String> mod = properties.opt<String>("mod");
    Maybe<float> modhueshift = properties.opt<float>("modhueshift");

    return make_shared<const BackBrush>(materialName, mod, hueshift, modhueshift, colorVariant);
  }

  BrushConstPtr getMaterialBrush(String const& materialName, Tiled::Properties& properties) {
    TileLayer layer = LayerNames.getLeft(properties.get<String>("layer"));

    if (layer == TileLayer::Background) {
      return getBackBrush(materialName, properties);
    } else {
      starAssert(layer == TileLayer::Foreground);
      return getFrontBrush(materialName, properties);
    }
  }

  BrushConstPtr getPlayerStartBrush(String const&, Tiled::Properties&) {
    return make_shared<PlayerStartBrush>();
  }

  BrushConstPtr getObjectBrush(String const& objectName, Tiled::Properties& properties) {
    Star::Direction direction = Star::Direction::Right;
    Json parameters;

    if (properties.contains("tilesetDirection"))
      direction = DirectionNames.getLeft(properties.get<String>("tilesetDirection"));
    if (properties.contains("flipX"))
      direction = -direction;

    if (properties.contains("parameters")) {
      parameters = properties.get<Json>("parameters");
    }

    parameters = parameters.opt().value(JsonObject{});

    return make_shared<const ObjectBrush>(objectName, direction, parameters);
  }

  BrushConstPtr getVehicleBrush(String const& vehicleName, Tiled::Properties& properties) {
    Json parameters = JsonObject{};
    if (properties.contains("parameters")) {
      parameters = properties.get<Json>("parameters");
    }
    return make_shared<const VehicleBrush>(vehicleName, parameters);
  }

  BrushConstPtr getWireBrush(String const& group, Tiled::Properties& properties) {
    bool local = properties.opt<bool>("local").value(true);

    return make_shared<const WireBrush>(group, local);
  }

  Json getSeed(Tiled::Properties& properties) {
    String seed = properties.get<String>("seed");
    if (seed == "stable")
      return seed;
    return lexicalCast<uint64_t>(seed);
  }

  BrushConstPtr getNpcBrush(String const& species, Tiled::Properties& properties) {
    JsonObject brush;
    brush["kind"] = "npc";
    brush["species"] = species; // this may be a single species or a comma
    // separated list to be parsed later
    if (properties.contains("seed")) {
      brush["seed"] = getSeed(properties);
    }
    if (properties.contains("typeName"))
      brush["typeName"] = properties.get<String>("typeName");
    brush["parameters"] = properties.opt<Json>("parameters").value(JsonObject{});
    return make_shared<const NpcBrush>(brush);
  }

  BrushConstPtr getMonsterBrush(String const& typeName, Tiled::Properties& properties) {
    JsonObject brush;
    brush["kind"] = "monster";
    brush["typeName"] = typeName;
    if (properties.contains("seed")) {
      brush["seed"] = getSeed(properties);
    }
    brush["parameters"] = properties.opt<Json>("parameters").value(JsonObject{});
    return make_shared<const NpcBrush>(brush);
  }

  BrushConstPtr getStagehandBrush(String const& typeName, Tiled::Properties& properties) {
    JsonObject brush;
    brush["type"] = typeName;
    brush["parameters"] = properties.opt<Json>("parameters").value(JsonObject{});
    if (properties.contains("broadcastArea"))
      brush["parameters"] = brush["parameters"].set("broadcastArea", properties.get<Json>("broadcastArea"));
    if (typeName == "radiomessage" && properties.contains("radioMessage"))
      brush["parameters"] = brush["parameters"].set("radioMessage", properties.get<Json>("radioMessage"));
    return make_shared<const StagehandBrush>(brush);
  }

  BrushConstPtr getDungeonIdBrush(String const& dungeonId, Tiled::Properties&) {
    return make_shared<const DungeonIdBrush>(maybeLexicalCast<DungeonId>(dungeonId).value(NoDungeonId));
  }

  BrushConstPtr getBiomeItemsBrush(String const&, Tiled::Properties&) {
    return make_shared<const BiomeItemsBrush>();
  }

  BrushConstPtr getBiomeTreeBrush(String const&, Tiled::Properties&) {
    return make_shared<const BiomeTreeBrush>();
  }

  BrushConstPtr getItemBrush(String const& itemName, Tiled::Properties& properties) {
    size_t count = properties.opt<size_t>("count").value(1);
    Json parameters = properties.opt<Json>("parameters").value(JsonObject{});
    ItemDescriptor item(itemName, count, parameters);
    return make_shared<const ItemBrush>(item);
  }

  BrushConstPtr getSurfaceBrush(String const& variantStr, Tiled::Properties& properties) {
    TileLayer layer = LayerNames.getLeft(properties.get<String>("layer"));
    Maybe<int> variant = maybeLexicalCast<int>(variantStr);
    Maybe<String> mod = properties.opt<String>("mod");

    if (layer == TileLayer::Background)
      return make_shared<const SurfaceBackgroundBrush>(variant, mod);
    return make_shared<const SurfaceBrush>(variant, mod);
  }

  BrushConstPtr getLiquidBrush(String const& liquidName, Tiled::Properties& properties) {
    float quantity = properties.opt<float>("quantity").value(1.0f);
    bool source = properties.opt<bool>("source").value(false);
    return make_shared<const LiquidBrush>(liquidName, quantity, source);
  }

  Maybe<BrushConstPtr> getInvalidBrush(bool invalidValue, Tiled::Properties& properties) {
    if (!invalidValue)
      return {};

    return as<const Brush>(make_shared<const InvalidBrush>(properties.opt<String>("//name")));
  }

  RuleConstPtr getAirRule(String const&, Tiled::Properties& properties) {
    TileLayer layer = LayerNames.getLeft(properties.get<String>("layer"));
    return make_shared<const WorldGenMustContainAirRule>(layer);
  }

  RuleConstPtr getSolidRule(String const&, Tiled::Properties& properties) {
    TileLayer layer = LayerNames.getLeft(properties.get<String>("layer"));
    return make_shared<const WorldGenMustContainSolidRule>(layer);
  }

  RuleConstPtr getLiquidRule(String const&, Tiled::Properties&) {
    return make_shared<const WorldGenMustContainLiquidRule>();
  }

  RuleConstPtr getNotLiquidRule(String const&, Tiled::Properties&) {
    return make_shared<const WorldGenMustNotContainLiquidRule>();
  }

  RuleConstPtr getAllowOverdrawingRule(String const&, Tiled::Properties&) {
    return make_shared<const AllowOverdrawingRule>();
  }

  template <typename T>
  class PropertyReader {
  public:
    template <typename PropertyType,
        typename GetterReturn = T,
        typename Getter = function<GetterReturn(PropertyType, Tiled::Properties&)>>
    void optRead(List<T>& list, String const& propertyName, Getter getter, Tiled::Properties& properties) {
      auto appendFn = bind(&List<T>::append, &list, _1);
      read<PropertyType, Getter>(propertyName, getter, properties).exec(appendFn);
    }

  private:
    template <typename PropertyType, typename Getter>
    Maybe<T> read(String const& propertyName, Getter getter, Tiled::Properties& properties) {
      Maybe<PropertyType> propertyValue = properties.opt<PropertyType>(propertyName);
      if (propertyValue.isValid())
        return getter(*propertyValue, properties);
      return {};
    }
  };

  Tile::Tile(Properties const& tileProperties, TileLayer layer, bool flipX)
    : Dungeon::Tile(), properties(tileProperties) {
    JsonObject computedProperties;
    if (!properties.contains("layer")) {
      computedProperties["layer"] = LayerNames.getRight(layer);
    } else {
      layer = LayerNames.getLeft(properties.get<String>("layer"));
    }

    if (flipX)
      computedProperties["flipX"] = "true";

    if (layer == TileLayer::Background && !properties.contains("clear"))
      // The magic pink tile/brush has the clear property set to "false". All
      // other tiles default to clear="true".
      computedProperties["clear"] = "true";

    properties = properties.inherit(computedProperties);

    PropertyReader<BrushConstPtr> br;
    br.optRead<bool, Maybe<BrushConstPtr>>(brushes, "clear", getClearBrush, properties);
    br.optRead<String>(brushes, "material", getMaterialBrush, properties);
    br.optRead<String>(brushes, "front", getFrontBrush, properties);
    br.optRead<String>(brushes, "back", getBackBrush, properties);
    br.optRead<String>(brushes, "playerstart", getPlayerStartBrush, properties);
    br.optRead<String>(brushes, "object", getObjectBrush, properties);
    br.optRead<String>(brushes, "vehicle", getVehicleBrush, properties);
    br.optRead<String>(brushes, "wire", getWireBrush, properties);
    br.optRead<String>(brushes, "npc", getNpcBrush, properties);
    br.optRead<String>(brushes, "monster", getMonsterBrush, properties);
    br.optRead<String>(brushes, "stagehand", getStagehandBrush, properties);
    br.optRead<String>(brushes, "dungeonid", getDungeonIdBrush, properties);
    br.optRead<String>(brushes, "biomeitems", getBiomeItemsBrush, properties);
    br.optRead<String>(brushes, "biometree", getBiomeTreeBrush, properties);
    br.optRead<String>(brushes, "item", getItemBrush, properties);
    br.optRead<String>(brushes, "surface", getSurfaceBrush, properties);
    br.optRead<String>(brushes, "liquid", getLiquidBrush, properties);
    br.optRead<bool, Maybe<BrushConstPtr>>(brushes, "invalid", getInvalidBrush, properties);

    PropertyReader<RuleConstPtr> rr;
    rr.optRead<String>(rules, "worldGenMustContainAir", getAirRule, properties);
    rr.optRead<String>(rules, "worldGenMustContainSolid", getSolidRule, properties);
    rr.optRead<String>(rules, "worldGenMustContainLiquid", getLiquidRule, properties);
    rr.optRead<String>(rules, "worldGenMustNotContainLiquid", getNotLiquidRule, properties);
    rr.optRead<String>(rules, "allowOverdrawing", getAllowOverdrawingRule, properties);

    if (auto connectorName = properties.opt<String>("connector")) {
      auto newConnector = TileConnector();

      newConnector.value = *connectorName;

      auto connectForwardOnly = properties.opt<bool>("connectForwardOnly");
      newConnector.forwardOnly = connectForwardOnly.value(false);

      if (auto connectDirection = properties.opt<String>("connectDirection"))
        newConnector.direction = DungeonDirectionNames.getLeft(*connectDirection);

      connector = newConnector;
    }
  }

  Tileset::Tileset(Json const& json) {
    Properties tilesetProperties(json.opt("properties").value(JsonObject{}));
    Json tileProperties = json.opt("tileproperties").value(JsonObject{});

    m_tilesBack.resize(json.getInt("tilecount"));
    m_tilesFront.resize(json.getInt("tilecount"));

    for (auto const& entry : tileProperties.iterateObject()) {
      size_t index = lexicalCast<size_t>(entry.first);
      Properties properties = Properties(entry.second).inherit(tilesetProperties);

      m_tilesBack[index] = make_shared<Tile>(properties, TileLayer::Background);
      m_tilesFront[index] = make_shared<Tile>(properties, TileLayer::Foreground);
    }
  }

  TileConstPtr const& Tileset::getTile(size_t id, TileLayer layer) const {
    List<TileConstPtr> const& tileset = tiles(layer);
    return tileset[id];
  }

  size_t Tileset::size() const {
    starAssert(m_tilesBack.size() == m_tilesFront.size());
    return m_tilesBack.size();
  }

  List<TileConstPtr> const& Tileset::tiles(TileLayer layer) const {
    if (layer == TileLayer::Background)
      return m_tilesBack;
    starAssert(layer == TileLayer::Foreground);
    return m_tilesFront;
  }
}

TilesetDatabase::TilesetDatabase() : m_cacheMutex(), m_tilesetCache() {}

Tiled::TilesetConstPtr TilesetDatabase::get(String const& path) const {
  MutexLocker locker(m_cacheMutex);
  return m_tilesetCache.get(path, TilesetDatabase::readTileset);
}

Tiled::TilesetConstPtr TilesetDatabase::readTileset(String const& path) {
  auto assets = Root::singleton().assets();
  return make_shared<Tiled::Tileset>(assets->json(path));
}

namespace Tiled {
  Json PropertyConverter<Json>::to(String const& propertyValue) {
    try {
      return Json::parseJson(propertyValue);
    } catch (JsonParsingException const& e) {
      throw StarException::format("Error parsing Tiled property as Json: %s", outputException(e, false));
    }
  }

  String PropertyConverter<Json>::from(Json const& propertyValue) {
    return propertyValue.repr();
  }

  String PropertyConverter<String>::to(String const& propertyValue) {
    return propertyValue;
  }

  String PropertyConverter<String>::from(String const& propertyValue) {
    return propertyValue;
  }
}

}

#include "StarParallax.hpp"
#include "StarLexicalCast.hpp"
#include "StarJsonExtra.hpp"
#include "StarRoot.hpp"
#include "StarImageMetadataDatabase.hpp"
#include "StarRandom.hpp"
#include "StarAssets.hpp"
#include "StarDataStreamExtra.hpp"

namespace Star {

ParallaxLayer::ParallaxLayer() {
  timeOfDayCorrelation = "";
  zLevel = 0;
  verticalOrigin = 0;
  speed = 0;
  unlit = false;
  lightMapped = false;
  fadePercent = 0;
  directives = "";
  alpha = 1.0f;
}

ParallaxLayer::ParallaxLayer(Json const& store) : ParallaxLayer() {
  textures = jsonToStringList(store.get("textures"));
  directives = store.getString("directives");
  parallaxValue = jsonToVec2F(store.get("parallaxValue"));
  repeat = jsonToVec2B(store.get("repeat"));
  tileLimitTop = store.optFloat("tileLimitTop");
  tileLimitBottom = store.optFloat("tileLimitBottom");
  verticalOrigin = store.getFloat("verticalOrigin");
  zLevel = store.getFloat("zLevel");
  parallaxOffset = jsonToVec2F(store.get("parallaxOffset"));
  timeOfDayCorrelation = store.getString("timeOfDayCorrelation");
  speed = store.getFloat("speed");
  unlit = store.getBool("unlit");
  lightMapped = store.getBool("lightMapped");
  fadePercent = store.getFloat("fadePercent");
}

Json ParallaxLayer::store() const {
  return JsonObject{{"textures", jsonFromStringList(textures)},
      {"directives", directives},
      {"parallaxValue", jsonFromVec2F(parallaxValue)},
      {"repeat", jsonFromVec2B(repeat)},
      {"tileLimitTop", jsonFromMaybe(tileLimitTop)},
      {"tileLimitBottom", jsonFromMaybe(tileLimitBottom)},
      {"verticalOrigin", verticalOrigin},
      {"zLevel", zLevel},
      {"parallaxOffset", jsonFromVec2F(parallaxOffset)},
      {"timeOfDayCorrelation", timeOfDayCorrelation},
      {"speed", speed},
      {"unlit", unlit},
      {"lightMapped", lightMapped},
      {"fadePercent", fadePercent}};
}

void ParallaxLayer::addImageDirectives(String const& newDirectives) {
  if (!newDirectives.empty())
    directives = String::joinWith("?", directives, newDirectives);
}

void ParallaxLayer::fadeToSkyColor(Color skyColor) {
  if (fadePercent > 0) {
    String fade = "fade=" + skyColor.toHex().slice(0, 6) + "=" + toString(fadePercent);
    addImageDirectives(fade);
  }
}

DataStream& operator>>(DataStream& ds, ParallaxLayer& parallaxLayer) {
  ds.read(parallaxLayer.textures);
  ds.read(parallaxLayer.directives);
  ds.read(parallaxLayer.alpha);
  ds.read(parallaxLayer.parallaxValue);
  ds.read(parallaxLayer.repeat);
  ds.read(parallaxLayer.tileLimitTop);
  ds.read(parallaxLayer.tileLimitBottom);
  ds.read(parallaxLayer.verticalOrigin);
  ds.read(parallaxLayer.zLevel);
  ds.read(parallaxLayer.parallaxOffset);
  ds.read(parallaxLayer.timeOfDayCorrelation);
  ds.read(parallaxLayer.speed);
  ds.read(parallaxLayer.unlit);
  ds.read(parallaxLayer.lightMapped);
  ds.read(parallaxLayer.fadePercent);
  return ds;
}

DataStream& operator<<(DataStream& ds, ParallaxLayer const& parallaxLayer) {
  ds.write(parallaxLayer.textures);
  ds.write(parallaxLayer.directives);
  ds.write(parallaxLayer.alpha);
  ds.write(parallaxLayer.parallaxValue);
  ds.write(parallaxLayer.repeat);
  ds.write(parallaxLayer.tileLimitTop);
  ds.write(parallaxLayer.tileLimitBottom);
  ds.write(parallaxLayer.verticalOrigin);
  ds.write(parallaxLayer.zLevel);
  ds.write(parallaxLayer.parallaxOffset);
  ds.write(parallaxLayer.timeOfDayCorrelation);
  ds.write(parallaxLayer.speed);
  ds.write(parallaxLayer.unlit);
  ds.write(parallaxLayer.lightMapped);
  ds.write(parallaxLayer.fadePercent);
  return ds;
}

Parallax::Parallax(String const& assetFile,
    uint64_t seed,
    float verticalOrigin,
    float hueShift,
    Maybe<TreeVariant> parallaxTreeVariant) {
  m_seed = seed;
  m_verticalOrigin = verticalOrigin;
  m_parallaxTreeVariant = parallaxTreeVariant;
  m_hueShift = hueShift;
  m_imageDirectory = "/parallax/images/";

  auto assets = Root::singleton().assets();

  Json config = assets->json(assetFile);

  m_verticalOrigin += config.getFloat("verticalOrigin", 0);

  RandomSource rnd(m_seed);

  for (auto layerSettings : config.getArray("layers")) {
    String kind = layerSettings.getString("kind");

    float frequency = layerSettings.getFloat("frequency", 1.0);
    if (rnd.randf() > frequency)
      continue;

    buildLayer(layerSettings, kind);
  }

  // sort with highest Z level first
  stableSort(m_layers, [](ParallaxLayer const& a, ParallaxLayer const& b) { return a.zLevel > b.zLevel; });
}

Parallax::Parallax(Json const& store) {
  m_seed = store.getUInt("seed");
  m_verticalOrigin = store.getFloat("verticalOrigin");
  if (auto treeVariant = store.opt("parallaxTreeVariant"))
    m_parallaxTreeVariant = TreeVariant(*treeVariant);
  m_hueShift = store.getFloat("hueShift");
  m_imageDirectory = store.getString("imageDirectory");
  m_layers = store.getArray("layers").transformed(construct<ParallaxLayer>());

  stableSort(m_layers, [](ParallaxLayer const& a, ParallaxLayer const& b) { return a.zLevel > b.zLevel; });
}

Json Parallax::store() const {
  return JsonObject{{"seed", m_seed},
      {"verticalOrigin", m_verticalOrigin},
      {"parallaxTreeVariant", m_parallaxTreeVariant ? m_parallaxTreeVariant->toJson() : Json()},
      {"hueShift", m_hueShift},
      {"imageDirectory", m_imageDirectory},
      {"layers", m_layers.transformed(mem_fn(&ParallaxLayer::store))}};
}

void Parallax::fadeToSkyColor(Color const& skyColor) {
  for (auto& layer : m_layers) {
    layer.fadeToSkyColor(skyColor);
  }
}

ParallaxLayers const& Parallax::layers() const {
  return m_layers;
}

void Parallax::buildLayer(Json const& layerSettings, String const& kind) {
  bool isFoliage = kind.beginsWith("foliage/");
  bool isStem = kind.beginsWith("stem/");

  String texPath = m_imageDirectory + kind + "/";
  if (isFoliage) {
    // If our tree type has no foliage, don't construct this layer
    if (!m_parallaxTreeVariant || !m_parallaxTreeVariant->foliageSettings.getBool("parallaxFoliage", false))
      return;

    texPath = m_parallaxTreeVariant->foliageDirectory + "parallax/" + kind.replace("foliage/", "") + "/";
  } else if (isStem) {
    if (!m_parallaxTreeVariant)
      return;

    texPath = m_parallaxTreeVariant->stemDirectory + "parallax/" + kind.replace("stem/", "") + "/";
  }

  ParallaxLayer layer;
  RandomSource rnd(m_seed + m_layers.size());
  auto imgMetadata = Root::singleton().imageMetadataDatabase();

  int baseCount = layerSettings.getInt("baseCount", 1);
  int base = rnd.randInt(baseCount - 1) + 1;
  layer.textures.append(AssetPath::relativeTo(texPath, "base/" + toString<int>(base) + ".png"));

  int modCount = layerSettings.getInt("modCount", 0);
  int mod = rnd.randInt(modCount);
  if (mod != 0)
    layer.textures.append(AssetPath::relativeTo(texPath, "mod/" + toString<int>(mod) + ".png"));

  if (layerSettings.get("parallax").type() == Json::Type::Array)
    layer.parallaxValue = jsonToVec2F(layerSettings.get("parallax"));
  else
    layer.parallaxValue = Vec2F::filled(layerSettings.getFloat("parallax"));

  bool repeatY = layerSettings.getBool("repeatY", false);
  layer.repeat = {true, repeatY};
  if (repeatY) {
    layer.tileLimitTop = layerSettings.optFloat("tileLimitTop");
    layer.tileLimitBottom = layerSettings.optFloat("tileLimitBottom");
  }

  layer.verticalOrigin = m_verticalOrigin;
  layer.zLevel = layer.parallaxValue.sum();
  layer.parallaxOffset = {layerSettings.getArray("offset", {0, 0})[0].toFloat(),
      layerSettings.getArray("offset", {0, 0})[1].toFloat()}; // shift from bottom left to horizon level in the image
  if (!layerSettings.getBool("noRandomOffset", false))
    layer.parallaxOffset[0] += rnd.randInt(imgMetadata->imageSize(layer.textures[0])[0]);
  layer.timeOfDayCorrelation = layerSettings.getString("timeOfDayCorrelation", "");
  layer.speed = rnd.randf(layerSettings.getFloat("minSpeed", 0), layerSettings.getFloat("maxSpeed", 0));
  layer.unlit = layerSettings.getBool("unlit", false);
  layer.lightMapped = layerSettings.getBool("lightMapped", false);

  layer.addImageDirectives(layerSettings.getString("directives", ""));
  if (isFoliage)
    layer.addImageDirectives(strf("hueshift=%s", m_parallaxTreeVariant->foliageHueShift));
  else if (isStem)
    layer.addImageDirectives(strf("hueshift=%s", m_parallaxTreeVariant->stemHueShift));
  else if (!layerSettings.getBool("nohueshift", false))
    layer.addImageDirectives(strf("hueshift=%s", m_hueShift));

  layer.fadePercent = layerSettings.getFloat("fadePercent", 0);

  m_layers.append(layer);
}

}

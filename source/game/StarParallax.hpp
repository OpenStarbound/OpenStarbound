#ifndef STAR_PARALLAX_HPP
#define STAR_PARALLAX_HPP

#include "StarMaybe.hpp"
#include "StarColor.hpp"
#include "StarPlantDatabase.hpp"
#include "StarDirectives.hpp"

namespace Star {

STAR_CLASS(Parallax);
STAR_STRUCT(ParallaxLayer);

struct ParallaxLayer {
  ParallaxLayer();
  ParallaxLayer(Json const& store);

  Json store() const;

  void addImageDirectives(Directives const& newDirectives);
  void fadeToSkyColor(Color skyColor);

  List<String> textures;
  Directives directives;
  float alpha;
  Vec2F parallaxValue;
  Vec2B repeat;
  Maybe<float> tileLimitTop;
  Maybe<float> tileLimitBottom;
  float verticalOrigin;
  float zLevel;
  Vec2F parallaxOffset;
  String timeOfDayCorrelation;
  float speed;
  bool unlit;
  bool lightMapped;
  float fadePercent;
};
typedef List<ParallaxLayer> ParallaxLayers;

DataStream& operator>>(DataStream& ds, ParallaxLayer& parallaxLayer);
DataStream& operator<<(DataStream& ds, ParallaxLayer const& parallaxLayer);

// Object managing and rendering the parallax for a World
class Parallax {
public:
  Parallax(String const& assetFile,
      uint64_t seed,
      float verticalOrigin,
      float hueShift,
      Maybe<TreeVariant> parallaxTreeVariant = {});
  Parallax(Json const& store);

  Json store() const;

  void fadeToSkyColor(Color const& skyColor);

  ParallaxLayers const& layers() const;

private:
  void buildLayer(Json const& layerSettings, String const& kind);

  uint64_t m_seed;
  float m_verticalOrigin;
  Maybe<TreeVariant> m_parallaxTreeVariant;
  float m_hueShift;

  String m_imageDirectory;

  ParallaxLayers m_layers;
};

}

#endif

#include "StarFile.hpp"
#include "StarLexicalCast.hpp"
#include "StarImage.hpp"
#include "StarRootLoader.hpp"
#include "StarTerrainDatabase.hpp"
#include "StarJson.hpp"
#include "StarRandom.hpp"
#include "StarColor.hpp"
#include "StarMultiArray.hpp"

using namespace Star;

int main(int argc, char** argv) {
  try {
    RootLoader rootLoader({{}, {}, {}, LogLevel::Error, false, {}});

    rootLoader.setSummary("Generate a heatmap image visualizing the output of a given terrain selector");

    rootLoader.addParameter("selector", "selector", OptionParser::Required, "name of the terrain selector to be rendered");
    rootLoader.addParameter("size", "size", OptionParser::Required, "x,y size of the region to be rendered");
    rootLoader.addParameter("seed", "seed", OptionParser::Optional, "seed value for the selector");
    rootLoader.addParameter("commonality", "commonality", OptionParser::Optional, "commonality value for the selector (default 1)");
    rootLoader.addParameter("scale", "scale", OptionParser::Optional, "maximum distance from 0 for color range");
    rootLoader.addParameter("mode", "mode", OptionParser::Optional, "color mode: heatmap, terrain");

    RootUPtr root;
    OptionParser::Options options;
    tie(root, options) = rootLoader.commandInitOrDie(argc, argv);

    auto size = Vec2U(lexicalCast<unsigned>(options.parameters["size"].first().split(",")[0]), lexicalCast<unsigned>(options.parameters["size"].first().split(",")[1]));

    auto seed = Random::randu64();
    if (!options.parameters["seed"].empty())
      seed = lexicalCast<uint64_t>(options.parameters["seed"].first());

    float commonality = 1.0f;
    if (!options.parameters["commonality"].empty())
      commonality = lexicalCast<float>(options.parameters["commonality"].first());

    float scale = 1.0f;
    bool autoScale = true;
    if (!options.parameters["scale"].empty()) {
      autoScale = false;
      scale = lexicalCast<float>(options.parameters["scale"].first());
    }

    String mode = "heatmap";
    if (!options.parameters["mode"].empty())
      mode = options.parameters["mode"].first();

    auto selectorParameters = TerrainSelectorParameters(JsonObject{
        {"worldWidth", size[0]},
        {"baseHeight", size[1] / 2},
        {"seed", seed},
        {"commonality", commonality}
      });

    auto selector = Root::singleton().terrainDatabase()->createNamedSelector(options.parameters["selector"].first(), selectorParameters);

    MultiArray<float, 2> terrainResult({size[0], size[1]}, 0.0f);
    for (size_t x = 0; x < size[0]; ++x) {
      for (size_t y = 0; y < size[1]; ++y) {
        auto value = selector->get(x, y);
        terrainResult(x, y) = value;
        if (autoScale)
          scale = max(scale, abs(value));
      }
    }

    coutf("Generating {} size image for selector with scale {}\n", size, scale);
    auto outputImage = make_shared<Image>(size, PixelFormat::RGB24);

    for (size_t x = 0; x < size[0]; ++x) {
      for (size_t y = 0; y < size[1]; ++y) {
        // Image y = 0 is the top, so reverse it for the world position
        auto value = terrainResult(x, y) / scale;
        if (mode == "heatmap") {
          Color color = Color::rgb(255, 0, 0);
          color.setHue(clamp(value / 2 + 0.5f, 0.0f, 1.0f));
          outputImage->set(x, y, color.toRgb());
        } else if (mode == "terrain") {
          if (value > 0)
            outputImage->set(x, y, Vec3B(0, 100 + floor(155 * value), floor(255 * value)));
          else
            outputImage->set(x, y, Vec3B(floor(255 * -value), 0, 0));
        }
      }
    }

    outputImage->writePng(File::open("terrain.png", IOMode::Write));
    return 0;
  } catch (std::exception const& e) {
    cerrf("exception caught: {}\n", outputException(e, true));
    return 1;
  }
}

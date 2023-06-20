#include "StarFile.hpp"
#include "StarLexicalCast.hpp"
#include "StarImage.hpp"
#include "StarRootLoader.hpp"
#include "StarCelestialDatabase.hpp"
#include "StarWorldTemplate.hpp"

using namespace Star;

int main(int argc, char** argv) {
  try {
    RootLoader rootLoader({{}, {}, {}, LogLevel::Error, false, {}});

    rootLoader.setSummary("Generate a WorldTemplate and output the data in it to an image");

    rootLoader.addParameter("coordinate", "coordinate", OptionParser::Optional, "coordinate for the celestial world");
    rootLoader.addParameter("coordseed", "seed", OptionParser::Optional, "seed to use when selecting a random celestial world coordinate");
    rootLoader.addParameter("size", "size", OptionParser::Optional, "x,y size of the region to be rendered");
    rootLoader.addSwitch("weighting", "Output instead the region weighting at each point");
    rootLoader.addSwitch("weightingblocknoise", "apply layout block noise before outputting weighting");
    rootLoader.addSwitch("transition", "show biome transition regions");

    RootUPtr root;
    OptionParser::Options options;
    tie(root, options) = rootLoader.commandInitOrDie(argc, argv);

    CelestialMasterDatabasePtr celestialDatabase = make_shared<CelestialMasterDatabase>();

    Maybe<CelestialCoordinate> coordinate;
    if (!options.parameters["coordinate"].empty())
      coordinate = CelestialCoordinate(options.parameters["coordinate"].first());
    else if (!options.parameters["coordseed"].empty())
      coordinate = celestialDatabase->findRandomWorld(
          10, 50, {}, lexicalCast<uint64_t>(options.parameters["coordseed"].first()));
    else
      coordinate = celestialDatabase->findRandomWorld();

    if (!coordinate)
      throw StarException("Could not find world to generate, try again");

    coutf("Generating world with coordinate %s\n", *coordinate);

    WorldTemplate worldTemplate(*coordinate, celestialDatabase);
    auto size = worldTemplate.size();

    if (!options.parameters["size"].empty()) {
      auto regionSize = Vec2U(lexicalCast<unsigned>(options.parameters["size"].first().split(",")[0]),
          lexicalCast<unsigned>(options.parameters["size"].first().split(",")[1]));
      size = regionSize.piecewiseClamp(Vec2U(0, 0), size);
    } else if (size[0] > 1000) {
      size[0] = 1000;
    }

    coutf("Generating %s size image for world of type '%s'\n", size, worldTemplate.worldParameters()->typeName);
    auto outputImage = make_shared<Image>(size, PixelFormat::RGB24);

    Color groundColor = Color::rgb(255, 0, 0);
    Color caveColor = Color::rgb(128, 0, 0);
    Color blankColor = Color::rgb(0, 0, 0);

    for (size_t x = 0; x < size[0]; ++x) {
      for (size_t y = 0; y < size[1]; ++y) {
        if (options.switches.contains("weighting")) {
          auto layout = worldTemplate.worldLayout();
          Color color = Color::Black;
          Vec2I pos(x, y);
          if (options.switches.contains("weightingblocknoise")) {
            if (auto blockNoise = layout->blockNoise())
              pos = blockNoise->apply(pos, size);
          }
          auto weightings = layout->getWeighting(pos[0], pos[1]);
          for (auto const& weighting : weightings) {
            Color mixColor = Color::rgb(128, 0, 0);
            mixColor.setHue(staticRandomFloat((uint64_t)weighting.region));
            color = Color::rgbaf(color.toRgbaF() + mixColor.toRgbaF() * weighting.weight);
          }
          outputImage->set(x, y, color.toRgb());
        } else if (options.switches.contains("transition")) {
          auto blockInfo = worldTemplate.blockInfo(x, y);
          if (isRealMaterial(blockInfo.foreground)) {
            Color color = groundColor;
            color.setHue(blockInfo.biomeTransition ? 0 : 0.5f);
            outputImage->set(x, y, color.toRgb());
          } else if (isRealMaterial(blockInfo.background)) {
            Color color = caveColor;
            color.setHue(blockInfo.biomeTransition ? 0 : 0.5f);
            outputImage->set(x, y, color.toRgb());
          } else {
            outputImage->set(x, y, blankColor.toRgb());
          }
        } else {
          // Image y = 0 is the top, so reverse it for the world tile
          auto blockInfo = worldTemplate.blockInfo(x, y);
          if (isRealMaterial(blockInfo.foreground)) {
            Color color = groundColor;
            color.setHue(staticRandomFloat(blockInfo.foreground));
            color.setSaturation(staticRandomFloat(blockInfo.foregroundMod));
            outputImage->set(x, y, color.toRgb());
          } else if (isRealMaterial(blockInfo.background)) {
            Color color = caveColor;
            color.setHue(staticRandomFloat(blockInfo.background));
            color.setSaturation(staticRandomFloat(blockInfo.backgroundMod));
            outputImage->set(x, y, color.toRgb());
          } else {
            outputImage->set(x, y, blankColor.toRgb());
          }
        }
      }
    }

    outputImage->writePng(File::open("mapgen.png", IOMode::Write));
    return 0;
  } catch (std::exception const& e) {
    cerrf("exception caught: %s\n", outputException(e, true));
    return 1;
  }
}

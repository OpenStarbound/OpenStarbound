#include "StarRootLoader.hpp"
#include "StarCelestialDatabase.hpp"
#include "StarWorldTemplate.hpp"
#include "StarWorldServer.hpp"

using namespace Star;

int main(int argc, char** argv) {
  try {
    RootLoader rootLoader({{}, {}, {}, LogLevel::Error, false, {}});
    rootLoader.addParameter("coordinate", "coordinate", OptionParser::Optional, "world coordinate to test");
    rootLoader.addParameter("regions", "regions", OptionParser::Optional, "number of regions to generate, default 1000");
    rootLoader.addParameter("regionsize", "size", OptionParser::Optional, "width / height of each generation region, default 10");
    rootLoader.addParameter("reportevery", "report regions", OptionParser::Optional, "number of generation regions before each progress report, default 20");

    RootUPtr root;
    OptionParser::Options options;
    tie(root, options) = rootLoader.commandInitOrDie(argc, argv);

    coutf("Fully loading root...");
    root->fullyLoad();
    coutf(" done\n");

    CelestialMasterDatabase celestialDatabase;

    CelestialCoordinate coordinate;
    if (auto coordinateOption = options.parameters.maybe("coordinate")) {
      coordinate = CelestialCoordinate(coordinateOption->first());
    } else {
      coordinate = celestialDatabase.findRandomWorld(100, 50, [&](CelestialCoordinate const& coord) {
          return celestialDatabase.parameters(coord)->isVisitable();
        }).take();
    }

    unsigned regionsToGenerate = 1000;
    if (auto regionsOption = options.parameters.maybe("regions"))
      regionsToGenerate = lexicalCast<unsigned>(regionsOption->first());

    unsigned regionSize = 10;
    if (auto regionSizeOption = options.parameters.maybe("regionsize"))
      regionSize = lexicalCast<unsigned>(regionSizeOption->first());

    unsigned reportEvery = 20;
    if (auto reportEveryOption = options.parameters.maybe("reportevery"))
      reportEvery = lexicalCast<unsigned>(reportEveryOption->first());

    coutf("testing generation on coordinate {}\n", coordinate);

    auto worldParameters = celestialDatabase.parameters(coordinate).take();
    auto worldTemplate = make_shared<WorldTemplate>(worldParameters.visitableParameters(), SkyParameters(), worldParameters.seed());

    auto rand = RandomSource(worldTemplate->worldSeed());

    WorldServer worldServer(std::move(worldTemplate), File::ephemeralFile());
    Vec2U worldSize = worldServer.geometry().size();

    double start = Time::monotonicTime();
    double lastReport = Time::monotonicTime();

    coutf("Starting world generation for {} regions\n", regionsToGenerate);

    for (unsigned i = 0; i < regionsToGenerate; ++i) {
      if (i != 0 && i % reportEvery == 0) {
        float gps = reportEvery / (Time::monotonicTime() - lastReport);
        lastReport = Time::monotonicTime();
        coutf("[{}] {}s | Generatons Per Second: {}\n", i, Time::monotonicTime() - start, gps);
      }

      RectI region = RectI::withCenter(Vec2I(rand.randInt(0, worldSize[0]), rand.randInt(0, worldSize[1])), Vec2I::filled(regionSize));
      worldServer.generateRegion(region);
    }

    coutf("Finished generating {} regions with size {}x{} in world '{}' in {} seconds", regionsToGenerate, regionSize, regionSize, coordinate, Time::monotonicTime() - start);

    return 0;

  } catch (std::exception const& e) {
    cerrf("Exception caught: {}\n", outputException(e, true));
    return 1;
  }
}

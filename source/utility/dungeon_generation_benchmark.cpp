#include "StarRootLoader.hpp"
#include "StarCelestialDatabase.hpp"
#include "StarWorldTemplate.hpp"
#include "StarWorldServer.hpp"

using namespace Star;

int main(int argc, char** argv) {
  try {
    unsigned repetitions = 5;
    unsigned reportEvery = 1;
    String dungeonWorldName = "outpost";

    RootLoader rootLoader({{}, {}, {}, LogLevel::Error, false, {}});
    rootLoader.addParameter("dungeonWorld", "dungeonWorld", OptionParser::Optional, strf("dungeonWorld to test, default is %s", dungeonWorldName));
    rootLoader.addParameter("repetitions", "repetitions", OptionParser::Optional, strf("number of times to generate, default %s", repetitions));
    rootLoader.addParameter("reportevery", "report repetitions", OptionParser::Optional, strf("number of repetitions before each progress report, default %s", reportEvery));

    RootUPtr root;
    OptionParser::Options options;
    tie(root, options) = rootLoader.commandInitOrDie(argc, argv);

    coutf("Fully loading root...");
    root->fullyLoad();
    coutf(" done\n");

    if (auto repetitionsOption = options.parameters.maybe("repetitions"))
      repetitions = lexicalCast<unsigned>(repetitionsOption->first());

    if (auto reportEveryOption = options.parameters.maybe("reportevery"))
      reportEvery = lexicalCast<unsigned>(reportEveryOption->first());

    if (auto dungeonWorldOption = options.parameters.maybe("dungeonWorld"))
      dungeonWorldName = dungeonWorldOption->first();

    double start = Time::monotonicTime();
    double lastReport = Time::monotonicTime();

    coutf("testing %s generations of dungeonWorld %s\n", repetitions, dungeonWorldName);

    for (unsigned i = 0; i < repetitions; ++i) {
      if (i > 0 && i % reportEvery == 0) {
        float gps = reportEvery / (Time::monotonicTime() - lastReport);
        lastReport = Time::monotonicTime();
        coutf("[%s] %ss | Generations Per Second: %s\n", i, Time::monotonicTime() - start, gps);
      }

      VisitableWorldParametersPtr worldParameters = generateFloatingDungeonWorldParameters(dungeonWorldName);
      auto worldTemplate = make_shared<WorldTemplate>(worldParameters, SkyParameters(), 1234);
      WorldServer worldServer(move(worldTemplate), File::ephemeralFile());
    }

    coutf("Finished %s generations of dungeonWorld %s in %s seconds", repetitions, dungeonWorldName, Time::monotonicTime() - start);

    return 0;

  } catch (std::exception const& e) {
    cerrf("Exception caught: %s\n", outputException(e, true));
    return 1;
  }
}

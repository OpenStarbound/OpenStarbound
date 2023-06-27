#include "StarLexicalCast.hpp"
#include "StarLogging.hpp"
#include "StarRootLoader.hpp"
#include "StarWorldServer.hpp"
#include "StarWorldTemplate.hpp"

using namespace Star;

int main(int argc, char** argv) {
  try {
    RootLoader rootLoader({{}, {}, {}, LogLevel::Error, false, {}});
    rootLoader.addArgument("dungeon", OptionParser::Required, "name of the dungeon to spawn in the world to benchmark");
    rootLoader.addParameter("seed", "seed", OptionParser::Optional, "world seed used to create the WorldTemplate");
    rootLoader.addParameter("steps", "steps", OptionParser::Optional, "number of steps to run the world for, defaults to 5,000");
    rootLoader.addParameter("times", "times", OptionParser::Optional, "how many times to perform the run, defaults to once");
    rootLoader.addParameter("signalevery", "signal steps", OptionParser::Optional, "number of steps to wait between scanning and signaling all entities to stay alive, default 120");
    rootLoader.addParameter("reportevery", "report steps", OptionParser::Optional, "number of steps between each progress report, default 0 (do not report progress)");
    rootLoader.addParameter("fidelity", "server fidelity", OptionParser::Optional, "fidelity to run the server with, default high");
    rootLoader.addSwitch("profiling", "whether to use lua profiling, prints the profile with info logging");
    rootLoader.addSwitch("unsafe", "enables unsafe lua libraries");
    RootUPtr root;
    OptionParser::Options options;
    tie(root, options) = rootLoader.commandInitOrDie(argc, argv);

    coutf("Fully loading root...");
    root->fullyLoad();
    coutf(" done\n");

    String dungeon = options.arguments.first();
    VisitableWorldParametersPtr worldParameters = generateFloatingDungeonWorldParameters(dungeon);
    uint64_t worldSeed = Random::randu64();
    if (options.parameters.contains("seed"))
      worldSeed = lexicalCast<uint64_t>(options.parameters.get("seed").first());
    auto worldTemplate = make_shared<WorldTemplate>(worldParameters, SkyParameters(), worldSeed);

    auto fidelity = options.parameters.maybe("fidelity").apply([](StringList p) { return p.maybeFirst(); }).value({});
    root->configuration()->set("serverFidelity", fidelity.value("high"));

    if (options.switches.contains("unsafe"))
      root->configuration()->set("safeScripts", false);
    if (options.switches.contains("profiling")) {
      root->configuration()->set("scriptProfilingEnabled", true);
      root->configuration()->set("scriptInstructionMeasureInterval", 100);
    }

    uint64_t times = 1;
    if (options.parameters.contains("times"))
      times = lexicalCast<uint64_t>(options.parameters.get("times").first());

    uint64_t steps = 5000;
    if (options.parameters.contains("steps"))
      steps = lexicalCast<uint64_t>(options.parameters.get("steps").first());

    uint64_t signalEvery = 120;
    if (options.parameters.contains("signalevery"))
      signalEvery = lexicalCast<uint64_t>(options.parameters.get("signalevery").first());

    uint64_t reportEvery = 0;
    if (options.parameters.contains("reportevery"))
      reportEvery = lexicalCast<uint64_t>(options.parameters.get("reportevery").first());

    double sumTime = 0.0;
    for (uint64_t i = 0; i < times; ++i) {
      WorldServer worldServer(worldTemplate, File::ephemeralFile());

      coutf("Starting world simulation for {} steps\n", steps);
      double start = Time::monotonicTime();
      double lastReport = Time::monotonicTime();
      uint64_t entityCount = 0;
      for (uint64_t j = 0; j < steps; ++j) {
        if (j % signalEvery == 0) {
          entityCount = 0;
          worldServer.forEachEntity(RectF(Vec2F(), Vec2F(worldServer.geometry().size())), [&](auto const& entity) {
              ++entityCount;
              worldServer.signalRegion(RectI::integral(entity->metaBoundBox().translated(entity->position())));
            });
        }

        if (reportEvery != 0 && j % reportEvery == 0) {
          float fps = reportEvery / (Time::monotonicTime() - lastReport);
          lastReport = Time::monotonicTime();
          coutf("[{}] {}s | FPS: {} | Entities: {}\n", j, Time::monotonicTime() - start, fps, entityCount);
        }
        worldServer.update();
      }
      double totalTime = Time::monotonicTime() - start;
      coutf("Finished run of running dungeon world '{}' with seed {} for {} steps in {} seconds, average FPS: {}\n",
            dungeon, worldSeed, steps, totalTime, steps / totalTime);
      sumTime += totalTime;
    }

    if (times != 1) {
      coutf("Average of all runs - time: {}, FPS: {}\n", sumTime / times, steps / (sumTime / times));
    }

    return 0;
  } catch (std::exception const& e) {
    cerrf("Exception caught: {}\n", outputException(e, true));
    return 1;
  }
}

#include "StarVersionOptionParser.hpp"
#include "StarFile.hpp"

namespace Star {

void VersionOptionParser::printVersion(std::ostream& os) {
  format(os, "Starbound Version {} ({})\n", StarVersionString, StarArchitectureString);
  format(os, "Source Identifier - {}\n", StarSourceIdentifierString);
}

VersionOptionParser::VersionOptionParser() {
  addSwitch("help", "Show help text");
  addSwitch("version", "Print version info");
}

VersionOptionParser::Options VersionOptionParser::parseOrDie(StringList const& cmdLineArguments) const {
  Options options;
  StringList errors;
  tie(options, errors) = OptionParser::parseOptions(cmdLineArguments);

  if (options.switches.contains("version"))
    printVersion(std::cout);

  if (options.switches.contains("help"))
    printHelp(std::cout);

  if (options.switches.contains("version") || options.switches.contains("help"))
    std::exit(0);

  if (!errors.empty()) {
    for (auto const& err : errors)
      coutf("Error: {}\n", err);
    coutf("\n");
    printHelp(std::cout);
    std::exit(1);
  }

  return options;
}

VersionOptionParser::Options VersionOptionParser::commandParseOrDie(int argc, char** argv) {
  setCommandName(File::baseName(argv[0]));
  return parseOrDie(StringList(argc - 1, argv + 1));
}

}

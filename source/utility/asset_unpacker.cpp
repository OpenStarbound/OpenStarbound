#include "StarPackedAssetSource.hpp"
#include "StarTime.hpp"
#include "StarJsonExtra.hpp"
#include "StarFile.hpp"
#include "StarVersionOptionParser.hpp"

using namespace Star;

int main(int argc, char** argv) {
  try {
    double startTime = Time::monotonicTime();

    VersionOptionParser optParse;
    optParse.setVersionName("Asset Unpacker");
    optParse.setSummary("Unpacks a starbound .pak file into an asset folder");
    optParse.addSwitch("v", "Verbose, list each file extracted");
    optParse.addArgument("input pak path", OptionParser::Required, "Path to the .pak file to unpack");
    optParse.addArgument("output folder", OptionParser::Required, "Output folder");

    auto opts = optParse.commandParseOrDie(argc, argv);

    String inputFile = opts.arguments.at(0);
    String outputFolderPath = opts.arguments.at(1);

    bool verbose = opts.switches.contains("v");

    PackedAssetSource assetsPack(inputFile);

    if (!File::isDirectory(outputFolderPath))
      File::makeDirectory(outputFolderPath);

    File::changeDirectory(outputFolderPath);

    auto allFiles = assetsPack.assetPaths();

    for (auto file : allFiles) {
      try {
        auto fileData = assetsPack.read(file);
        auto relativePath = "." + file;
        auto relativeDir = File::dirName(relativePath);
        File::makeDirectoryRecursive(relativeDir);
        File::writeFile(fileData, relativePath);
        if (verbose)
          coutf("Extracting file '{}' to the output directory as '{}'\n", file, relativePath);
      } catch (AssetSourceException const& e) {
        cerrf("Could not open file: {}\n", file);
        cerrf("Reason: {}\n", outputException(e, false));
      }
    }

    auto metadata = assetsPack.metadata();
    if (!metadata.empty())
      File::writeFile(Json(std::move(metadata)).printJson(2), "_metadata");

    coutf("Unpacked assets to {} in {}s\n", outputFolderPath, Time::monotonicTime() - startTime);
    return 0;
  } catch (std::exception const& e) {
    cerrf("Exception caught: {}\n", outputException(e, true));
    return 1;
  }
}

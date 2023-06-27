#include "StarPackedAssetSource.hpp"
#include "StarTime.hpp"
#include "StarJsonExtra.hpp"
#include "StarFile.hpp"

using namespace Star;

int main(int argc, char** argv) {
  try {
    double startTime = Time::monotonicTime();

    if (argc != 3) {
      cerrf("Usage: {} <assets pak path> <target output directory>\n", argv[0]);
      cerrf("If the target output directory does not exist it will be created\n");
      return 1;
    }

    String inputFile = argv[1];
    String outputFolderPath = argv[2];

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
      } catch (AssetSourceException const& e) {
        cerrf("Could not open file: {}\n", file);
        cerrf("Reason: {}\n", outputException(e, false));
      }
    }

    auto metadata = assetsPack.metadata();
    if (!metadata.empty())
      File::writeFile(Json(move(metadata)).printJson(2), "_metadata");

    coutf("Unpacked assets to {} in {}s\n", outputFolderPath, Time::monotonicTime() - startTime);
    return 0;
  } catch (std::exception const& e) {
    cerrf("Exception caught: {}\n", outputException(e, true));
    return 1;
  }
}

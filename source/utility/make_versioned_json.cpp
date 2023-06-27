#include "StarFile.hpp"
#include "StarVersioningDatabase.hpp"

using namespace Star;

int main(int argc, char** argv) {
  try {
    if (argc != 3) {
      coutf("Usage, {} <versioned_json_json> <versioned_json_binary>\n", argv[0]);
      return -1;
    }

    auto versionedJson = VersionedJson::fromJson(Json::parse(File::readFileString(argv[1])));
    VersionedJson::writeFile(versionedJson, argv[2]);
    return 0;
  } catch (std::exception const& e) {
    coutf("Error! Caught exception {}\n", outputException(e, true));
    return 1;
  }
}

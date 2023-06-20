#include "StarRootLoader.hpp"
#include "StarRootLuaBindings.hpp"
#include "StarUtilityLuaBindings.hpp"
#include "StarRootLuaBindings.hpp"

using namespace Star;

int main(int argc, char** argv) {
  RootLoader rootLoader({{}, {}, {}, LogLevel::Error, false, {}});
  RootUPtr root;
  OptionParser::Options options;
  tie(root, options) = rootLoader.commandInitOrDie(argc, argv);

  auto engine = LuaEngine::create(true);
  auto context = engine->createContext();
  context.setCallbacks("sb", LuaBindings::makeUtilityCallbacks());
  context.setCallbacks("root", LuaBindings::makeRootCallbacks());

  String code;
  bool continuation = false;
  while (!std::cin.eof()) {
    auto getline = [](std::istream& stream) -> String {
      std::string line;
      std::getline(stream, line);
      return String(move(line));
    };

    if (continuation) {
      std::cout << ">> ";
      std::cout.flush();
      code += getline(std::cin);
      code += '\n';
    } else {
      std::cout << "> ";
      std::cout.flush();
      code = getline(std::cin);
      code += '\n';
    }

    try {
      auto result = context.eval<LuaVariadic<LuaValue>>(code);
      for (auto r : result)
        coutf("%s\n", r);
      continuation = false;
    } catch (LuaIncompleteStatementException const&) {
      continuation = true;
    } catch (std::exception const& e) {
      coutf("Error: %s\n", outputException(e, false));
      continuation = false;
    }
  }
  return 0;
}

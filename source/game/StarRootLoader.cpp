#include "StarRootLoader.hpp"
#include "StarLexicalCast.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

Json const BaseAssetsSettings = Json::parseJson(R"JSON(
    {
      "assetTimeToLive" : 30,

      // In seconds, audio less than this long will be decompressed in memory.
      "audioDecompressLimit" : 4.0,

      "workerPoolSize" : 2,

      "pathIgnore" : [
        "/\\.",
        "/~",
        "thumbs\\.db$",
        "\\.bak$",
        "\\.tmp$",
        "\\.zip$",
        "\\.orig$",
        "\\.fail$",
        "\\.psd$",
        "\\.tmx$"
      ],

      "digestIgnore" : [
        "\\.ogg$",
        "\\.wav$",
        "\\.abc$"
      ]
    }
  )JSON");

Json const BaseDefaultConfiguration = Json::parseJson(R"JSON(
    {
      "configurationVersion" : {
        "basic" : 2
      },

      "gameServerPort" : 21025,
)JSON"
#ifdef STAR_SYSTEM_WINDOWS
                                                      R"JSON(
      "gameServerBind" : "*",
      "queryServerBind" : "*",
      "rconServerBind" : "*",
)JSON"
#else
                                                      R"JSON(
      "gameServerBind" : "::",
)JSON"
#endif
R"JSON(
      "serverUsers" : {},
      "allowAnonymousConnections" : true,

      "bannedUuids" : [],
      "bannedIPs" : [],

      "serverName" : "A Starbound Server",
      "maxPlayers" : 8,
      "maxTeamSize" : 4,
      "serverFidelity" : "automatic",

      "checkAssetsDigest" : false,

      "safeScripts" : true,
      "scriptRecursionLimit" : 100,
      "scriptInstructionLimit" : 10000000,
      "scriptProfilingEnabled" : false,
      "scriptInstructionMeasureInterval" : 10000,

      "allowAdminCommands" : true,
      "allowAdminCommandsFromAnyone" : false,
      "anonymousConnectionsAreAdmin" : false,

      "clientP2PJoinable" : true,
      "clientIPJoinable" : false,

      "clearUniverseFiles" : false,
      "clearPlayerFiles" : false,
      "playerBackupFileCount" : 3,

      "tutorialMessages" : true,

      "interactiveHighlight" : true,

      "monochromeLighting" : false,

      "crafting" : {
        "filterHaveMaterials" : false
      },

      "inventory" : {
        "pickupToActionBar" : true
      }
    }
  )JSON");

RootLoader::RootLoader(Defaults defaults) {
  String baseConfigFile;
  Maybe<String> userConfigFile;

  addParameter("bootconfig", "bootconfig", Optional,
      strf("Boot time configuration file, defaults to sbinit.config"));
  addParameter("logfile", "logfile", Optional,
      strf("Log to the given logfile relative to the root directory, defaults to {}",
        defaults.logFile ? *defaults.logFile : "no log file"));
  addParameter("loglevel", "level", Optional,
      strf("Sets the logging level (debug|info|warn|error), defaults to {}",
        LogLevelNames.getRight(defaults.logLevel)));
  addSwitch("quiet", strf("Do not log to stdout, defaults to {}", defaults.quiet));
  addSwitch("verbose", strf("Log to stdout, defaults to {}", !defaults.quiet));
  addSwitch("runtimeconfig",
      strf("Sets the path to the runtime configuration storage file relative to root directory, defauts to {}",
        defaults.runtimeConfigFile ? *defaults.runtimeConfigFile : "no storage file"));
  m_defaults = std::move(defaults);
}

pair<Root::Settings, RootLoader::Options> RootLoader::parseOrDie(
    StringList const& cmdLineArguments) const {
  auto options = VersionOptionParser::parseOrDie(cmdLineArguments);
  return {rootSettingsForOptions(options), options};
}

pair<RootUPtr, RootLoader::Options> RootLoader::initOrDie(StringList const& cmdLineArguments) const {
  auto p = parseOrDie(cmdLineArguments);
  auto root = make_unique<Root>(p.first);
  return {std::move(root), p.second};
}

pair<Root::Settings, RootLoader::Options> RootLoader::commandParseOrDie(int argc, char** argv) {
  auto options = VersionOptionParser::commandParseOrDie(argc, argv);
  return {rootSettingsForOptions(options), options};
}

pair<RootUPtr, RootLoader::Options> RootLoader::commandInitOrDie(int argc, char** argv) {
  auto p = commandParseOrDie(argc, argv);
  auto root = make_unique<Root>(p.first);
  return {std::move(root), p.second};
}

Root::Settings RootLoader::rootSettingsForOptions(Options const& options) const {
  try {
    String bootConfigFile = options.parameters.value("bootconfig").maybeFirst().value("sbinit.config");
    Json bootConfig = Json::parseJson(File::readFileString(bootConfigFile));

    Json assetsSettings = jsonMerge(
        BaseAssetsSettings,
        m_defaults.additionalAssetsSettings,
        bootConfig.get("assetsSettings", {})
      );

    Root::Settings rootSettings;
    rootSettings.assetsSettings.assetTimeToLive = assetsSettings.getInt("assetTimeToLive");
    rootSettings.assetsSettings.audioDecompressLimit = assetsSettings.getFloat("audioDecompressLimit");
    rootSettings.assetsSettings.workerPoolSize = assetsSettings.getUInt("workerPoolSize");
    rootSettings.assetsSettings.missingImage = assetsSettings.optString("missingImage");
    rootSettings.assetsSettings.missingAudio = assetsSettings.optString("missingAudio");
    rootSettings.assetsSettings.pathIgnore = jsonToStringList(assetsSettings.get("pathIgnore"));
    rootSettings.assetsSettings.digestIgnore = jsonToStringList(assetsSettings.get("digestIgnore"));

    rootSettings.assetDirectories = jsonToStringList(bootConfig.get("assetDirectories", JsonArray()));
    rootSettings.assetSources     = jsonToStringList(bootConfig.get("assetSources",     JsonArray()));

    rootSettings.defaultConfiguration = jsonMerge(
        BaseDefaultConfiguration,
        m_defaults.additionalDefaultConfiguration,
        bootConfig.get("defaultConfiguration", {})
      );

    rootSettings.storageDirectory = bootConfig.getString("storageDirectory");
    rootSettings.logDirectory = bootConfig.optString("logDirectory");
    rootSettings.logFile = options.parameters.value("logfile").maybeFirst().orMaybe(m_defaults.logFile);
    rootSettings.logFileBackups = bootConfig.getUInt("logFileBackups", 10);

    if (auto ll = options.parameters.value("loglevel").maybeFirst())
      rootSettings.logLevel = LogLevelNames.getLeft(*ll);
    else
      rootSettings.logLevel = m_defaults.logLevel;

    if (options.switches.contains("quiet"))
      rootSettings.quiet = true;
    else if (options.switches.contains("verbose"))
      rootSettings.quiet = false;
    else
      rootSettings.quiet = m_defaults.quiet;

    if (auto rc = options.parameters.value("runtimeconfig").maybeFirst())
      rootSettings.runtimeConfigFile = *rc;
    else
      rootSettings.runtimeConfigFile = m_defaults.runtimeConfigFile;

    return rootSettings;

  } catch (std::exception const& e) {
    throw StarException("Could not perform initial Root load", e);
  }
}

}

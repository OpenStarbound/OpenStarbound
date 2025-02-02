#include "StarClientCommandProcessor.hpp"
#include "StarItem.hpp"
#include "StarAssets.hpp"
#include "StarItemDatabase.hpp"
#include "StarPlayer.hpp"
#include "StarPlayerTech.hpp"
#include "StarPlayerInventory.hpp"
#include "StarPlayerLog.hpp"
#include "StarWorldClient.hpp"
#include "StarAiInterface.hpp"
#include "StarQuestInterface.hpp"
#include "StarStatistics.hpp"
#include "StarInterfaceLuaBindings.hpp"

namespace Star {

ClientCommandProcessor::ClientCommandProcessor(UniverseClientPtr universeClient, CinematicPtr cinematicOverlay,
  MainInterfacePaneManager* paneManager, StringMap<StringList> macroCommands)
  : m_universeClient(std::move(universeClient)), m_cinematicOverlay(std::move(cinematicOverlay)),
  m_paneManager(paneManager), m_macroCommands(std::move(macroCommands)) {
  m_builtinCommands = {
    {"reload", bind(&ClientCommandProcessor::reload, this)},
    {"hotReload", bind(&ClientCommandProcessor::hotReload, this)},
    {"whoami", bind(&ClientCommandProcessor::whoami, this)},
    {"gravity", bind(&ClientCommandProcessor::gravity, this)},
    {"debug", bind(&ClientCommandProcessor::debug, this, _1)},
    {"boxes", bind(&ClientCommandProcessor::boxes, this)},
    {"fullbright", bind(&ClientCommandProcessor::fullbright, this)},
    {"asyncLighting", bind(&ClientCommandProcessor::asyncLighting, this)},
    {"setGravity", bind(&ClientCommandProcessor::setGravity, this, _1)},
    {"resetGravity", bind(&ClientCommandProcessor::resetGravity, this)},
    {"fixedCamera", bind(&ClientCommandProcessor::fixedCamera, this)},
    {"monochromeLighting", bind(&ClientCommandProcessor::monochromeLighting, this)},
    {"radioMessage", bind(&ClientCommandProcessor::radioMessage, this, _1)},
    {"clearRadioMessages", bind(&ClientCommandProcessor::clearRadioMessages, this)},
    {"clearCinematics", bind(&ClientCommandProcessor::clearCinematics, this)},
    {"startQuest", bind(&ClientCommandProcessor::startQuest, this, _1)},
    {"completeQuest", bind(&ClientCommandProcessor::completeQuest, this, _1)},
    {"failQuest", bind(&ClientCommandProcessor::failQuest, this, _1)},
    {"previewNewQuest", bind(&ClientCommandProcessor::previewNewQuest, this, _1)},
    {"previewQuestComplete", bind(&ClientCommandProcessor::previewQuestComplete, this, _1)},
    {"previewQuestFailed", bind(&ClientCommandProcessor::previewQuestFailed, this, _1)},
    {"clearScannedObjects", bind(&ClientCommandProcessor::clearScannedObjects, this)},
    {"played", bind(&ClientCommandProcessor::playTime, this)},
    {"deaths", bind(&ClientCommandProcessor::deathCount, this)},
    {"cinema", bind(&ClientCommandProcessor::cinema, this, _1)},
    {"suicide", bind(&ClientCommandProcessor::suicide, this)},
    {"naked", bind(&ClientCommandProcessor::naked, this)},
    {"resetAchievements", bind(&ClientCommandProcessor::resetAchievements, this)},
    {"statistic", bind(&ClientCommandProcessor::statistic, this, _1)},
    {"giveessentialitem", bind(&ClientCommandProcessor::giveEssentialItem, this, _1)},
    {"maketechavailable", bind(&ClientCommandProcessor::makeTechAvailable, this, _1)},
    {"enabletech", bind(&ClientCommandProcessor::enableTech, this, _1)},
    {"upgradeship", bind(&ClientCommandProcessor::upgradeShip, this, _1)},
    {"swap", bind(&ClientCommandProcessor::swap, this, _1)},
    {"respawnInWorld", bind(&ClientCommandProcessor::respawnInWorld, this, _1)},
    {"render", bind(&ClientCommandProcessor::render, this, _1)}
  };
}

bool ClientCommandProcessor::adminCommandAllowed() const {
  return Root::singleton().configuration()->get("allowAdminCommandsFromAnyone").toBool() ||
    m_universeClient->mainPlayer()->isAdmin();
}

String ClientCommandProcessor::previewQuestPane(StringList const& arguments, function<PanePtr(QuestPtr)> createPane) {
  Maybe<String> templateId = {};
  templateId = arguments[0];
  if (auto quest = createPreviewQuest(*templateId, arguments.at(1), arguments.at(2), m_universeClient->mainPlayer().get())) {
    auto pane = createPane(quest);
    m_paneManager->displayPane(PaneLayer::ModalWindow, pane);
    return "Previewed quest";
  }
  return "No such quest";
}

StringList ClientCommandProcessor::handleCommand(String const& commandLine) {
  try {
    if (!commandLine.beginsWith("/"))
      throw StarException("ClientCommandProcessor expected command, does not start with '/'");

    String allArguments = commandLine.substr(1);
    String command = allArguments.extract();

    StringList result;
    if (auto builtinCommand = m_builtinCommands.maybe(command)) {
      result.append((*builtinCommand)(allArguments));
    } else if (auto macroCommand = m_macroCommands.maybe(command)) {
      for (auto const& c : *macroCommand) {
        if (c.beginsWith("/"))
          result.appendAll(handleCommand(c));
        else
          result.append(c);
      }
    } else {
      auto player = m_universeClient->mainPlayer();
      if (auto messageResult = player->receiveMessage(connectionForEntity(player->entityId()), "/" + command, {allArguments})) {
        if (messageResult->isType(Json::Type::String))
          result.append(*messageResult->stringPtr());
        else if (!messageResult->isNull())
          result.append(messageResult->repr(1, true));
      } else
        m_universeClient->sendChat(commandLine, ChatSendMode::Broadcast);
    }
    return result;
  } catch (ShellParsingException const& e) {
    Logger::error("Shell parsing exception: {}", outputException(e, false));
    return {"Shell parsing exception"};
  } catch (std::exception const& e) {
    Logger::error("Exception caught handling client command {}: {}", commandLine, outputException(e, true));
    return {strf("Exception caught handling client command {}", commandLine)};
  }
}

bool ClientCommandProcessor::debugDisplayEnabled() const {
  return m_debugDisplayEnabled;
}

bool ClientCommandProcessor::debugHudEnabled() const {
  return m_debugHudEnabled;
}

bool ClientCommandProcessor::fixedCameraEnabled() const {
  return m_fixedCameraEnabled;
}

String ClientCommandProcessor::reload() {
  Root::singleton().reload();
  return "Client Star::Root reloaded";
}

String ClientCommandProcessor::hotReload() {
  Root::singleton().hotReload();
  return "Hot-reloaded assets";
}

String ClientCommandProcessor::whoami() {
  return strf("Client: You are {}. You are {}an Admin.",
              m_universeClient->mainPlayer()->name(), m_universeClient->mainPlayer()->isAdmin() ? "" : "not ");
}

String ClientCommandProcessor::gravity() {
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  return toString(m_universeClient->worldClient()->gravity(m_universeClient->mainPlayer()->position()));
}

String ClientCommandProcessor::debug(String const& argumentsString) {
  auto arguments = m_parser.tokenizeToStringList(argumentsString);
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  if (!arguments.empty() && arguments.at(0).equalsIgnoreCase("hud")) {
    m_debugHudEnabled = !m_debugHudEnabled;
    return strf("Debug HUD {}", m_debugHudEnabled ? "enabled" : "disabled");
  }
  else {
    m_debugDisplayEnabled = !m_debugDisplayEnabled;
    return strf("Debug display {}", m_debugDisplayEnabled ? "enabled" : "disabled");
  }
}

String ClientCommandProcessor::boxes() {
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  auto worldClient = m_universeClient->worldClient();
  bool state = !worldClient->collisionDebug();
  worldClient->setCollisionDebug(state);
  return strf("Geometry debug display {}", state ? "enabled" : "disabled");
}

String ClientCommandProcessor::fullbright() {
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  auto worldClient = m_universeClient->worldClient();
  bool state = !worldClient->fullBright();
  worldClient->setFullBright(state);
  return strf("Fullbright render lighting {}", state ? "enabled" : "disabled");
}

String ClientCommandProcessor::asyncLighting() {
  auto worldClient = m_universeClient->worldClient();
  bool state = !worldClient->asyncLighting();
  worldClient->setAsyncLighting(state);
  return strf("Asynchronous render lighting {}", state ? "enabled" : "disabled");
}

String ClientCommandProcessor::setGravity(String const& argumentsString) {
  auto arguments = m_parser.tokenizeToStringList(argumentsString);
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  m_universeClient->worldClient()->overrideGravity(lexicalCast<float>(arguments.at(0)));
  return strf("Gravity set to {} (This is client-side!)", arguments.at(0));
}

String ClientCommandProcessor::resetGravity() {
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  m_universeClient->worldClient()->resetGravity();
  return "Gravity reset";
}

String ClientCommandProcessor::fixedCamera() {
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  m_fixedCameraEnabled = !m_fixedCameraEnabled;
  return strf("Fixed camera {}", m_fixedCameraEnabled ? "enabled" : "disabled");
}

String ClientCommandProcessor::monochromeLighting() {
  bool monochrome = !Root::singleton().configuration()->get("monochromeLighting").toBool();
  Root::singleton().configuration()->set("monochromeLighting", monochrome);
  return strf("Monochrome lighting {}", monochrome ? "enabled" : "disabled");
}

String ClientCommandProcessor::radioMessage(String const& argumentsString) {
  auto arguments = m_parser.tokenizeToStringList(argumentsString);
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  if (arguments.size() != 1)
    return "Must provide one argument";

  m_universeClient->mainPlayer()->queueRadioMessage(arguments.at(0));
  return "Queued radio message";
}

String ClientCommandProcessor::clearRadioMessages() {
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  m_universeClient->mainPlayer()->log()->clearRadioMessages();
  return "Player radio message records cleared!";
}

String ClientCommandProcessor::clearCinematics() {
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  m_universeClient->mainPlayer()->log()->clearCinematics();
  return "Player cinematic records cleared!";
}

String ClientCommandProcessor::startQuest(String const& argumentsString) {
  auto arguments = m_parser.tokenizeToStringList(argumentsString);
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  auto questArc = QuestArcDescriptor::fromJson(Json::parseSequence(arguments.at(0)).get(0));
  m_universeClient->questManager()->offer(make_shared<Quest>(questArc, 0, m_universeClient->mainPlayer().get()));
  return "Quest started";
}

String ClientCommandProcessor::completeQuest(String const& argumentsString) {
  auto arguments = m_parser.tokenizeToStringList(argumentsString);
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  m_universeClient->questManager()->getQuest(arguments.at(0))->complete();
  return strf("Quest {} complete", arguments.at(0));
}

String ClientCommandProcessor::failQuest(String const& argumentsString) {
  auto arguments = m_parser.tokenizeToStringList(argumentsString);
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  m_universeClient->questManager()->getQuest(arguments.at(0))->fail();
  return strf("Quest {} failed", arguments.at(0));
}

String ClientCommandProcessor::previewNewQuest(String const& argumentsString) {
  auto arguments = m_parser.tokenizeToStringList(argumentsString);
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  return previewQuestPane(arguments, [this](QuestPtr const& quest) {
    return make_shared<NewQuestInterface>(m_universeClient->questManager(), quest, m_universeClient->mainPlayer());
  });
}

String ClientCommandProcessor::previewQuestComplete(String const& argumentsString) {
  auto arguments = m_parser.tokenizeToStringList(argumentsString);
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  return previewQuestPane(arguments, [this](QuestPtr const& quest) {
    return make_shared<QuestCompleteInterface>(quest, m_universeClient->mainPlayer(), CinematicPtr{});
  });
}

String ClientCommandProcessor::previewQuestFailed(String const& argumentsString) {
  auto arguments = m_parser.tokenizeToStringList(argumentsString);
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  return previewQuestPane(arguments, [this](QuestPtr const& quest) {
    return make_shared<QuestFailedInterface>(quest, m_universeClient->mainPlayer());
  });
}

String ClientCommandProcessor::clearScannedObjects() {
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  m_universeClient->mainPlayer()->log()->clearScannedObjects();
  return "Player scanned objects cleared!";
}

String ClientCommandProcessor::playTime() {
  return strf("Total play time: {}", Time::printDuration(m_universeClient->mainPlayer()->log()->playTime()));
}

String ClientCommandProcessor::deathCount() {
  auto deaths = m_universeClient->mainPlayer()->log()->deathCount();
  return deaths ? strf("Total deaths: {}", deaths) : "Total deaths: 0. Well done!";
}

String ClientCommandProcessor::cinema(String const& argumentsString) {
  auto arguments = m_parser.tokenizeToStringList(argumentsString);
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  m_cinematicOverlay->load(Root::singleton().assets()->json(arguments.at(0)));
  if (arguments.size() > 1)
    m_cinematicOverlay->setTime(lexicalCast<float>(arguments.at(1)));
  return strf("Started cinematic {} at {}", arguments.at(0), arguments.size() > 1 ? arguments.at(1) : "beginning");
}

String ClientCommandProcessor::suicide() {
  m_universeClient->mainPlayer()->kill();
  return "You are now dead";
}

String ClientCommandProcessor::naked() {
  auto playerInventory = m_universeClient->mainPlayer()->inventory();
  for (auto slot : EquipmentSlotNames.leftValues())
    playerInventory->addItems(playerInventory->addToBags(playerInventory->takeSlot(slot)));
  return "You are now naked";
}

String ClientCommandProcessor::resetAchievements() {
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  if (m_universeClient->statistics()->reset()) {
    return "Achievements reset";
  }
  return "Unable to reset achievements";
}

String ClientCommandProcessor::statistic(String const& argumentsString) {
  auto arguments = m_parser.tokenizeToStringList(argumentsString);
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  StringList values;
  for (String const& statName : arguments) {
    values.append(strf("{} = {}", statName, m_universeClient->statistics()->stat(statName)));
  }
  return values.join("\n");
}

String ClientCommandProcessor::giveEssentialItem(String const& argumentsString) {
  auto arguments = m_parser.tokenizeToStringList(argumentsString);
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  if (arguments.size() < 2)
    return "Not enough arguments to /giveessentialitem";

  try {
    auto item = Root::singleton().itemDatabase()->item(ItemDescriptor(arguments.at(0)));
    auto slot = EssentialItemNames.getLeft(arguments.at(1));
    m_universeClient->mainPlayer()->inventory()->setEssentialItem(slot, item);
    return strf("Put {} in player slot {}", item->name(), arguments.at(1));
  } catch (MapException const& e) {
    return strf("Invalid essential item slot {}.", arguments.at(1));
  }
}

String ClientCommandProcessor::makeTechAvailable(String const& argumentsString) {
  auto arguments = m_parser.tokenizeToStringList(argumentsString);
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  if (arguments.size() == 0)
    return "Not enough arguments to /maketechavailable";

  m_universeClient->mainPlayer()->techs()->makeAvailable(arguments.at(0));
  return strf("Added {} to player's visible techs", arguments.at(0));
}

String ClientCommandProcessor::enableTech(String const& argumentsString) {
  auto arguments = m_parser.tokenizeToStringList(argumentsString);
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  if (arguments.size() == 0)
    return "Not enough arguments to /enabletech";

  m_universeClient->mainPlayer()->techs()->makeAvailable(arguments.at(0));
  m_universeClient->mainPlayer()->techs()->enable(arguments.at(0));
  return strf("Player tech {} enabled", arguments.at(0));
}

String ClientCommandProcessor::upgradeShip(String const& argumentsString) {
  auto arguments = m_parser.tokenizeToStringList(argumentsString);
  if (!adminCommandAllowed())
    return "You must be an admin to use this command.";

  if (arguments.size() == 0)
    return "Not enough arguments to /upgradeship";

  auto shipUpgrades = Json::parseJson(arguments.at(0));
  m_universeClient->rpcInterface()->invokeRemote("ship.applyShipUpgrades", shipUpgrades);
  return strf("Upgraded ship");
}

String ClientCommandProcessor::swap(String const& argumentsString) {
  auto arguments = m_parser.tokenizeToStringList(argumentsString);

  if (arguments.size() == 0)
    return "Not enough arguments to /swap";

  if (m_universeClient->switchPlayer(arguments[0]))
    return "Successfully swapped player";
  else
    return "Failed to swap player";
}

String ClientCommandProcessor::respawnInWorld(String const& argumentsString) {
  auto arguments = m_parser.tokenizeToStringList(argumentsString);
  auto worldClient = m_universeClient->worldClient();
  
  if (arguments.size() == 0)
    return strf("Respawn in this world is currently {}", worldClient->respawnInWorld() ? "true" : "false");

  bool respawnInWorld = Json::parse(arguments.at(0)).toBool();
  worldClient->setRespawnInWorld(respawnInWorld);
  return strf("Respawn in this world set to {} (This is client-side!)", respawnInWorld ? "true" : "false");
}

// Hardcoded render command, future version will write to the clipboard and possibly be implemented in Lua
String ClientCommandProcessor::render(String const& path) {
  if (path.empty()) {
    return "Specify a path to render an image, or for worn armor: "
           "^cyan;hat^reset;/^cyan;chest^reset;/^cyan;legs^reset;/^cyan;back^reset; "
           "or for body parts: "
           "^cyan;head^reset;/^cyan;body^reset;/^cyan;hair^reset;/^cyan;facialhair^reset;/"
           "^cyan;facialmask^reset;/^cyan;frontarm^reset;/^cyan;backarm^reset;/^cyan;emote^reset;";
  }
  AssetPath assetPath;
  bool outputSheet = false;
  String outputName = "render";
  auto player = m_universeClient->mainPlayer();
  if (player && path.utf8Size() < 100) {
    auto args = m_parser.tokenizeToStringList(path);
    auto first = args.maybeFirst().value().toLower();
    auto humanoid = player->humanoid();
    auto& identity = humanoid->identity();
    auto species = identity.imagePath.value(identity.species);
    outputSheet = true;
    outputName = first;
    if (first.equals("hat")) {
      assetPath.basePath = humanoid->headArmorFrameset();
      assetPath.directives += humanoid->headArmorDirectives();
    } else if (first.equals("chest")) {
      if (args.size() <= 1) {
        return "Chest armors have multiple spritesheets. Do: "
               "^white;/chest torso ^cyan;front^reset;/^cyan;torso^reset;/^cyan;back^reset;. "
               "To repair old generated clothes, then also specify ^cyan;old^reset;.";
      }
      String sheet = args[1].toLower();
      outputName += " " + sheet;
      if (sheet == "torso") {
        assetPath.basePath = humanoid->chestArmorFrameset();
        assetPath.directives += humanoid->chestArmorDirectives();
      } else if (sheet == "front") {
        assetPath.basePath = humanoid->frontSleeveFrameset();
        assetPath.directives += humanoid->chestArmorDirectives();
      } else if (sheet == "back") {
        assetPath.basePath = humanoid->backSleeveFrameset();
        assetPath.directives += humanoid->chestArmorDirectives();
      } else {
        return strf("^red;Invalid chest sheet type '{}'^reset;", sheet);
      }
      // recovery for custom chests made by a very old generator
      if (args.size() >= 2 && args[2].toLower() == "old" && assetPath.basePath.beginsWith("/items/armors/avian/avian-tier6separator/"))
          assetPath.basePath = "/items/armors/avian/avian-tier6separator/old/" + assetPath.basePath.substr(41);
    } else if (first.equals("legs")) {
      assetPath.basePath = humanoid->legsArmorFrameset();
      assetPath.directives += humanoid->legsArmorDirectives();
    } else if (first.equals("back")) {
      assetPath.basePath = humanoid->backArmorFrameset();
      assetPath.directives += humanoid->backArmorDirectives();
    } else if (first.equals("body")) {
      assetPath.basePath = humanoid->getBodyFromIdentity();
      assetPath.directives += identity.bodyDirectives;
    } else if (first.equals("head")) {
      outputSheet = false;
      assetPath.basePath = humanoid->getHeadFromIdentity();
      assetPath.directives += identity.bodyDirectives;
    } else if (first.equals("hair")) {
      outputSheet = false;
      assetPath.basePath = humanoid->getHairFromIdentity();
      assetPath.directives += identity.hairDirectives;
    } else if (first.equals("facialhair")) {
      outputSheet = false;
      assetPath.basePath = humanoid->getFacialHairFromIdentity();
      assetPath.directives += identity.facialHairDirectives;
    } else if (first.equals("facialmask")) {
      outputSheet = false;
      assetPath.basePath = humanoid->getFacialMaskFromIdentity();
      assetPath.directives += identity.facialMaskDirectives;
    } else if (first.equals("frontarm")) {
      assetPath.basePath = humanoid->getFrontArmFromIdentity();
      assetPath.directives += identity.bodyDirectives;
    } else if (first.equals("backarm")) {
      assetPath.basePath = humanoid->getBackArmFromIdentity();
      assetPath.directives += identity.bodyDirectives;
    } else if (first.equals("emote")) {
      assetPath.basePath = humanoid->getFacialEmotesFromIdentity();
      assetPath.directives += identity.emoteDirectives;
    } else {
      outputName = "render";
    }

    if (!outputSheet)
      assetPath.subPath = String("normal");
  }
  if (assetPath == AssetPath()) {
    assetPath = AssetPath::split(path);
    if (!assetPath.basePath.beginsWith("/"))
      assetPath.basePath = "/assetmissing.png" + assetPath.basePath;
  }
  auto assets = Root::singleton().assets();
  ImageConstPtr image;
  if (outputSheet) {
    auto sheet = make_shared<Image>(*assets->image(assetPath.basePath));
    sheet->convert(PixelFormat::RGBA32);
    AssetPath framePath = assetPath;

    StringMap<pair<RectU, ImageConstPtr>> frames;
    auto imageFrames = assets->imageFrames(assetPath.basePath);
    for (auto& pair : imageFrames->frames)
      frames[pair.first] = make_pair(pair.second, ImageConstPtr());

    if (frames.empty())
      return "^red;Failed to save image^reset;";

    for (auto& entry : frames) {
      framePath.subPath = entry.first;
      entry.second.second = assets->image(framePath);
    }

    Vec2U frameSize = frames.begin()->second.first.size();
    Vec2U imageSize = frames.begin()->second.second->size().piecewiseMin(Vec2U{256, 256});
    if (imageSize.min() == 0)
      return "^red;Resulting image is empty^reset;";

    for (auto& frame : frames) {
      RectU& box = frame.second.first;
      box.setXMin((box.xMin() / frameSize[0]) * imageSize[0]);
      box.setYMin(((sheet->height() - box.yMin() - box.height()) / frameSize[1]) * imageSize[1]);
      box.setXMax(box.xMin() + imageSize[0]);
      box.setYMax(box.yMin() + imageSize[1]);
    }

    if (frameSize != imageSize) {
      unsigned sheetWidth  = (sheet->width()  / frameSize[0]) * imageSize[0];
      unsigned sheetHeight = (sheet->height() / frameSize[0]) * imageSize[0];
      sheet->reset(sheetWidth, sheetHeight, PixelFormat::RGBA32);
    }

    for (auto& entry : frames)
      sheet->copyInto(entry.second.first.min(), *entry.second.second);

    image = std::move(sheet);
  } else {
    image = assets->image(assetPath);
  }
  if (image->size().min() == 0)
    return "^red;Resulting image is empty^reset;";
  auto outputDirectory = Root::singleton().toStoragePath("output");
  auto outputPath = File::relativeTo(outputDirectory, strf("{}.png", outputName));
  if (!File::isDirectory(outputDirectory))
    File::makeDirectory(outputDirectory);
  image->writePng(File::open(outputPath, IOMode::Write | IOMode::Truncate));
  return strf("Saved {}x{} image to {}.png", image->width(), image->height(), outputName);
}


}
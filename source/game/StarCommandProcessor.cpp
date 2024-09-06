#include "StarCommandProcessor.hpp"
#include "StarLexicalCast.hpp"
#include "StarJsonExtra.hpp"
#include "StarNpc.hpp"
#include "StarWorldServer.hpp"
#include "StarUniverseServer.hpp"
#include "StarUniverseSettings.hpp"
#include "StarRoot.hpp"
#include "StarItemDatabase.hpp"
#include "StarConfiguration.hpp"
#include "StarItemDrop.hpp"
#include "StarTreasure.hpp"
#include "StarLogging.hpp"
#include "StarPlayer.hpp"
#include "StarMonster.hpp"
#include "StarStagehand.hpp"
#include "StarVehicleDatabase.hpp"
#include "StarStagehandDatabase.hpp"
#include "StarLiquidsDatabase.hpp"
#include "StarChatProcessor.hpp"
#include "StarAssets.hpp"
#include "StarWorldLuaBindings.hpp"
#include "StarUniverseServerLuaBindings.hpp"

namespace Star {

CommandProcessor::CommandProcessor(UniverseServer* universe)
  : m_universe(universe) {
  auto assets = Root::singleton().assets();
  m_scriptComponent.addCallbacks("universe", LuaBindings::makeUniverseServerCallbacks(m_universe));
  m_scriptComponent.addCallbacks("CommandProcessor", makeCommandCallbacks());
  m_scriptComponent.setScripts(jsonToStringList(assets->json("/universe_server.config:commandProcessorScripts")));
  auto luaRoot = make_shared<LuaRoot>();
  luaRoot->luaEngine().setNullTerminated(false);
  m_scriptComponent.setLuaRoot(luaRoot);
  m_scriptComponent.init();
}

String CommandProcessor::adminCommand(String const& command, String const& argumentString) {
  MutexLocker locker(m_mutex);
  return handleCommand(ServerConnectionId, command, argumentString);
}

String CommandProcessor::userCommand(ConnectionId connectionId, String const& command, String const& argumentString) {
  MutexLocker locker(m_mutex);
  if (connectionId == ServerConnectionId)
    throw StarException("CommandProcessor::userCommand called with ServerConnectionId");
  return handleCommand(connectionId, command, argumentString);
}

String CommandProcessor::help(ConnectionId connectionId, String const& argumentString) {
  auto arguments = m_parser.tokenizeToStringList(argumentString);

  auto assets = Root::singleton().assets();
  auto basicCommands = assets->json("/help.config:basicCommands");
  auto openSbCommands = assets->json("/help.config:openSbCommands");
  auto adminCommands = assets->json("/help.config:adminCommands");
  auto debugCommands = assets->json("/help.config:debugCommands");
  auto openSbDebugCommands = assets->json("/help.config:openSbDebugCommands");

  if (arguments.size()) {
    if (arguments.size() >= 1) {
      if (auto helpText = basicCommands.optString(arguments[0]).orMaybe(openSbCommands.optString(arguments[0])).orMaybe(adminCommands.optString(arguments[0])).orMaybe(debugCommands.optString(arguments[0])).orMaybe(openSbDebugCommands.optString(arguments[0])))
        return *helpText;
    }
  }

  String res = "";

  auto commandDescriptions = [&](Json const& commandConfig) {
      StringList commandList = commandConfig.toObject().keys();
      sort(commandList);
      return "/" + commandList.join(", /");
    };

  String basicHelpFormat = assets->json("/help.config:basicHelpText").toString();
  res = res + strf(basicHelpFormat.utf8Ptr(), commandDescriptions(basicCommands));

  String openSbHelpFormat = assets->json("/help.config:openSbHelpText").toString();
  res = res + "\n" + strf(openSbHelpFormat.utf8Ptr(), commandDescriptions(openSbCommands));

  if (!adminCheck(connectionId, "")) {
    String adminHelpFormat = assets->json("/help.config:adminHelpText").toString();
    res = res + "\n" + strf(adminHelpFormat.utf8Ptr(), commandDescriptions(adminCommands));

    String debugHelpFormat = assets->json("/help.config:debugHelpText").toString();
    res = res + "\n" + strf(debugHelpFormat.utf8Ptr(), commandDescriptions(debugCommands));

    String openSbDebugHelpFormat = assets->json("/help.config:openSbDebugHelpText").toString();
    res = res + "\n" + strf(openSbDebugHelpFormat.utf8Ptr(), commandDescriptions(openSbDebugCommands));
  }

  res = res + "\n" + basicCommands.getString("help");

  return res;
}

String CommandProcessor::admin(ConnectionId connectionId, String const&) {
  auto config = Root::singleton().configuration();
  if (m_universe->canBecomeAdmin(connectionId)) {
    if (connectionId == ServerConnectionId)
      return "Invalid client state";

    if (!config->get("allowAdminCommands").toBool())
      return "Admin commands disabled on this server.";

    bool wasAdmin = m_universe->isAdmin(connectionId);
    m_universe->setAdmin(connectionId, !wasAdmin);

    if (!wasAdmin)
      return strf("Admin privileges now given to player {}", m_universe->clientNick(connectionId));
    else
      return strf("Admin privileges taken away from {}", m_universe->clientNick(connectionId));
  } else {
    return "Insufficient privileges to make self admin.";
  }
}

String CommandProcessor::pvp(ConnectionId connectionId, String const&) {
  if (!m_universe->isPvp(connectionId)) {
    m_universe->setPvp(connectionId, true);
    if (m_universe->isPvp(connectionId))
      m_universe->adminBroadcast(strf("Player {} is now PVP", m_universe->clientNick(connectionId)));
  } else {
    m_universe->setPvp(connectionId, false);
    if (!m_universe->isPvp(connectionId))
      m_universe->adminBroadcast(strf("Player {} is a big wimp and is no longer PVP", m_universe->clientNick(connectionId)));
  }

  if (m_universe->isPvp(connectionId))
    return "PVP active";
  else
    return "PVP inactive";
}

String CommandProcessor::whoami(ConnectionId connectionId, String const&) {
  return strf("Server: You are {}. You are {}an Admin",
      m_universe->clientNick(connectionId),
      m_universe->isAdmin(connectionId) ? "" : "not ");
}

String CommandProcessor::warp(ConnectionId connectionId, String const& argumentString) {
  if (auto errorMsg = adminCheck(connectionId, "do the space warp again"))
    return *errorMsg;

  try {
    m_universe->clientWarpPlayer(connectionId, parseWarpAction(argumentString));
    return "Lets do the space warp again";
  } catch (StarException const& e) {
    Logger::warn("Could not parse warp target: {}", outputException(e, false));
    return strf("Could not parse the argument {} as a warp target", argumentString);
  }
}

String CommandProcessor::warpRandom(ConnectionId connectionId, String const& typeName) {
  if (auto errorMsg = adminCheck(connectionId, "warp to random world"))
    return *errorMsg;

	Vec2I size = {2, 2};
	auto& celestialDatabase = m_universe->celestialDatabase();
	Maybe<CelestialCoordinate> target = {};

	auto validPlanet = [&celestialDatabase, &typeName](CelestialCoordinate const& p) {
			if (auto celestialParams = celestialDatabase.parameters(p)) {
				if (auto visitableParams = celestialParams->visitableParameters()) {
					if (visitableParams->typeName == typeName)
						return true;
				}
			}
			return false;
		};

	while (target.isNothing()) {
		RectI region = RectI::withSize(Vec2I(Random::randi32(), Random::randi32()), size);

		while (!celestialDatabase.scanRegionFullyLoaded(region)) {
			celestialDatabase.scanSystems(region);
		}
		auto systems = celestialDatabase.scanSystems(region);
		for (auto s : systems) {
			for (auto planet : celestialDatabase.children(s)) {
				if (validPlanet(planet))
					target = planet;
				if (target.isNothing()) {
					for (auto moon : celestialDatabase.children(planet)) {
						if (validPlanet(moon)) {
							target = moon;
							break;
						}
					}
				}
			}
		}

		if (size.magnitude() > 1024)
			return "could not find a matching world";
		size *= 2;
	}

	m_universe->clientWarpPlayer(connectionId, WarpToWorld(CelestialWorldId(*target)));
	return strf("warping to {}", *target);
}

String CommandProcessor::timewarp(ConnectionId connectionId, String const& argumentsString) {
  if (auto errorMsg = adminCheck(connectionId, "do the time warp again"))
    return *errorMsg;

  auto arguments = m_parser.tokenizeToStringList(argumentsString);
  if (arguments.empty())
    return "Not enough arguments to /timewarp";

  try {
    auto time = lexicalCast<double>(arguments.at(0));
    if (time == 0.0)
      return "You suck at time travel.";
    else if (time < 0.0 && (arguments.size() < 2 || arguments[1] != "please"))
      return "Great Scott! We can't go back in time!";

    m_universe->universeClock()->adjustTime(time);
    return time > 0.0 ? "It's just a jump to the left..." : "And then a step to the right...";
  } catch (BadLexicalCast const&) {
    return strf("Could not parse the argument {} as a time adjustment", arguments[0]);
  }
}

String CommandProcessor::timescale(ConnectionId connectionId, String const& argumentsString) {
  if (auto errorMsg = adminCheck(connectionId, "mess with time"))
    return *errorMsg;

  auto arguments = m_parser.tokenizeToStringList(argumentsString);

  if (arguments.empty())
    return strf("Current timescale is {:6.6f}x", GlobalTimescale);

  float timescale = clamp(lexicalCast<float>(arguments[0]), 0.001f, 32.0f);
  m_universe->setTimescale(timescale);
  return strf("Set timescale to {:6.6f}x", timescale);
}

String CommandProcessor::tickrate(ConnectionId connectionId, String const& argumentsString) {
  if (auto errorMsg = adminCheck(connectionId, "change the tick rate"))
    return *errorMsg;

  auto arguments = m_parser.tokenizeToStringList(argumentsString);

  if (arguments.empty())
    return strf("Current tick rate is {:4.2f}Hz", 1.0f / ServerGlobalTimestep);

  float tickRate = clamp(lexicalCast<float>(arguments[0]), 5.f, 500.f);
  m_universe->setTickRate(tickRate);
  return strf("Set tick rate to {:4.2f}Hz", tickRate);
}

String CommandProcessor::setTileProtection(ConnectionId connectionId, String const& argumentString) {
  if (auto errorMsg = adminCheck(connectionId, "modify world properties")) {
    return *errorMsg;
  }

  auto arguments = m_parser.tokenizeToStringList(argumentString);

  if (arguments.size() < 2)
    return "Not enough arguments to /settileprotection. Use /settileprotection <dungeonId> <protected>";

  try {
    DungeonId dungeonId = lexicalCast<DungeonId>(arguments.at(0));
    bool isProtected = lexicalCast<bool>(arguments.at(1));

    bool done = m_universe->executeForClient(connectionId, [dungeonId, isProtected](WorldServer* world, PlayerPtr const&) {
        world->setTileProtection(dungeonId, isProtected);
      });

    return done ? "" : "Failed to set block protection.";
  } catch (BadLexicalCast const&) {
    return strf("Could not parse /settileprotection parameters. Use /settileprotection <dungeonId> <protected>", argumentString);
  }
}

String CommandProcessor::setDungeonId(ConnectionId connectionId, String const& argumentString) {
  if (auto errorMsg = adminCheck(connectionId, "set dungeon id")) {
    return *errorMsg;
  }

  auto arguments = m_parser.tokenizeToStringList(argumentString);
  if (arguments.size() < 1)
    return "Not enough arguments to /setdungeonid. Use /setdungeonid <dungeonId>";

  try {
    DungeonId dungeonId = lexicalCast<DungeonId>(arguments.at(0));

    bool done = m_universe->executeForClient(connectionId, [dungeonId](WorldServer* world, PlayerPtr const& player) {
        world->setDungeonId(RectI::withSize(Vec2I(player->aimPosition()), Vec2I(1, 1)), dungeonId);
      });

    return done ? "" : "Failed to set dungeon id.";
  } catch (BadLexicalCast const&) {
    return strf("Could not parse /setdungeonid parameters. Use /setdungeonid <dungeonId>!", argumentString);
  }
}

String CommandProcessor::setPlayerStart(ConnectionId connectionId, String const&) {
  if (auto errorMsg = adminCheck(connectionId, "modify world properties"))
    return *errorMsg;

  m_universe->executeForClient(connectionId, [](WorldServer* world, PlayerPtr const& player) {
      world->setPlayerStart(player->position() + player->feetOffset());
    });

  return "";
}

String CommandProcessor::spawnItem(ConnectionId connectionId, String const& argumentString) {
  if (auto errorMsg = adminCheck(connectionId, "spawn items"))
    return *errorMsg;

  auto arguments = m_parser.tokenizeToStringList(argumentString);

  if (arguments.empty())
    return "Not enough arguments to /spawnitem";

  try {
    String kind = arguments.at(0);
    Json parameters = JsonObject();
    unsigned amount = 1;
    Maybe<float> level;
    Maybe<uint64_t> seed;

    if (arguments.size() >= 2)
      amount = lexicalCast<unsigned>(arguments.at(1));

    if (arguments.size() >= 3)
      parameters = Json::parse(arguments.at(2));

    if (arguments.size() >= 4)
      level = lexicalCast<float>(arguments.at(3));

    if (arguments.size() >= 5)
      seed = lexicalCast<uint64_t>(arguments.at(4));

    bool done = m_universe->executeForClient(connectionId, [&](WorldServer* world, PlayerPtr const& player) {
        auto itemDatabase = Root::singleton().itemDatabase();
        world->addEntity(ItemDrop::createRandomizedDrop(itemDatabase->item(ItemDescriptor(kind, amount, parameters), level, seed, true), player->aimPosition()));
      });

    return done ? "" : "Invalid client state";
  } catch (JsonParsingException const& exception) {
    Logger::warn("Error while processing /spawnitem '{}' command. Json parse problem: {}", arguments.at(0), outputException(exception, false));
    return "Could not parse item parameters";
  } catch (ItemException const& exception) {
    Logger::warn("Error while processing /spawnitem '{}' command. Item instantiation problem: {}", arguments.at(0), outputException(exception, false));
    return strf("Could not load item '{}'", arguments.at(0));
  } catch (BadLexicalCast const& exception) {
    Logger::warn("Error while processing /spawnitem command. Number expected. Got something else: {}", outputException(exception, false));
    return strf("Could not load item '{}'", arguments.at(0));
  } catch (StarException const& exception) {
    Logger::warn("Error while processing /spawnitem command '{}', exception caught: {}", argumentString, outputException(exception, false));
    return strf("Could not load item '{}'", arguments.at(0));
  }
}

String CommandProcessor::spawnTreasure(ConnectionId connectionId, String const& argumentString) {
  if (auto errorMsg = adminCheck(connectionId, "spawn items"))
    return *errorMsg;

  auto arguments = m_parser.tokenizeToStringList(argumentString);

  if (arguments.empty())
    return "Not enough arguments to /spawntreasure";

  try {
    String treasurePool = arguments.at(0);
    unsigned level = 1;

    if (arguments.size() >= 2)
      level = lexicalCast<unsigned>(arguments.at(1));

    bool done = m_universe->executeForClient(connectionId, [&](WorldServer* world, PlayerPtr const& player) {
        auto treasureDatabase = Root::singleton().treasureDatabase();
        for (auto const& treasureItem : treasureDatabase->createTreasure(treasurePool, level, Random::randu64()))
          world->addEntity(ItemDrop::createRandomizedDrop(treasureItem, player->aimPosition()));
      });

    return done ? "" : "Invalid client state";
  } catch (JsonParsingException const& exception) {
    Logger::warn("Error while processing /spawntreasure '{}' command. Json parse problem: {}", arguments.at(0), outputException(exception, false));
    return "Could not parse item parameters";
  } catch (ItemException const& exception) {
    Logger::warn("Error while processing /spawntreasure '{}' command. Item instantiation problem: {}", arguments.at(0), outputException(exception, false));
    return strf("Could not load item '{}'", arguments.at(0));
  } catch (BadLexicalCast const& exception) {
    Logger::warn("Error while processing /spawntreasure command. Number expected. Got something else: {}", outputException(exception, false));
    return strf("Could not load item '{}'", arguments.at(0));
  } catch (StarException const& exception) {
    Logger::warn("Error while processing /spawntreasure command '{}', exception caught: {}", argumentString, outputException(exception, false));
    return strf("Could not load item '{}'", arguments.at(0));
  }
}

String CommandProcessor::spawnMonster(ConnectionId connectionId, String const& argumentString) {
  if (auto errorMsg = adminCheck(connectionId, "spawn monsters"))
    return *errorMsg;

  try {
    auto arguments = m_parser.tokenizeToStringList(argumentString);

    auto monsterDatabase = Root::singleton().monsterDatabase();
    MonsterPtr monster;

    float level = 1;
    if (arguments.size() >= 2)
      level = lexicalCast<float>(arguments.at(1));

    Json parameters = JsonObject();
    if (arguments.size() >= 3)
      parameters = parameters.setAll(Json::parse(arguments.at(2)).toObject());

    monster = monsterDatabase->createMonster(monsterDatabase->randomMonster(arguments.at(0), parameters.toObject()), level);
    bool done = m_universe->executeForClient(connectionId,
        [&](WorldServer* world, PlayerPtr const& player) {
          monster->setPosition(player->aimPosition());
          world->addEntity(monster);
        });

    return done ? "" : "Invalid client state";
  } catch (StarException const& exception) {
    Logger::warn("Could not spawn Monster of type '{}', exception caught: {}", argumentString, outputException(exception, false));
    return strf("Could not spawn Monster of type '{}'", argumentString);
  }
}

String CommandProcessor::spawnNpc(ConnectionId connectionId, String const& argumentString) {
  if (auto errorMsg = adminCheck(connectionId, "spawn NPCs"))
    return *errorMsg;

  auto arguments = m_parser.tokenizeToStringList(argumentString);

  try {
    auto npcDatabase = Root::singleton().npcDatabase();
    float npcLevel = 1;
    uint64_t seed = Random::randu64();
    Json overrides;

    if (arguments.size() < 2)
      return "You must specify a species and NPC type to spawn.";

    if (arguments.size() >= 3)
      npcLevel = lexicalCast<float>(arguments.at(2));
    if (arguments.size() >= 4)
      seed = lexicalCast<uint64_t>(arguments.at(3));
    if (arguments.size() >= 5)
      overrides = Json::parse(arguments.at(4)).toObject();

    auto npc = npcDatabase->createNpc(npcDatabase->generateNpcVariant(arguments.at(0), arguments.at(1), npcLevel, seed, overrides));
    bool done = m_universe->executeForClient(connectionId, [&](WorldServer* world, PlayerPtr const& player) {
        npc->setPosition(player->aimPosition());
        world->addEntity(npc);
      });

    return done ? "" : "Invalid client state";
  } catch (StarException const& exception) {
    Logger::warn("Could not spawn NPC of species '{}', exception caught: {}", argumentString, outputException(exception, true));
    return strf("Could not spawn NPC of species '{}'", argumentString);
  }
}

String CommandProcessor::spawnVehicle(ConnectionId connectionId, String const& argumentString) {
  if (auto errorMsg = adminCheck(connectionId, "spawn vehicles"))
    return *errorMsg;

  try {
    auto vehicleDatabase = Root::singleton().vehicleDatabase();
    auto arguments = m_parser.tokenizeToStringList(argumentString);

    VehiclePtr vehicle;

    String name = arguments.at(0);

    Json parameters = JsonObject();
    if (arguments.size() >= 2)
      parameters = Json::parse(arguments.at(1)).toObject();

    vehicle = vehicleDatabase->create(name, parameters);
    bool done = m_universe->executeForClient(connectionId,
        [&](WorldServer* world, PlayerPtr const& player) {
          vehicle->setPosition(player->aimPosition());
          world->addEntity(std::move(vehicle));
        });

    return done ? "" : "Invalid client state";
  } catch (StarException const& exception) {
    Logger::warn("Could not spawn vehicle, exception caught: {}", outputException(exception, false));
    return strf("Could not spawn vehicle");
  }
}

String CommandProcessor::spawnStagehand(ConnectionId connectionId, String const& argumentString) {
  if (auto errorMsg = adminCheck(connectionId, "spawn stagehands"))
    return *errorMsg;

  try {
    auto arguments = m_parser.tokenizeToStringList(argumentString);

    auto stagehandDatabase = Root::singleton().stagehandDatabase();

    Json parameters = JsonObject();
    if (arguments.size() >= 2)
      parameters = Json::parse(arguments.at(1)).toObject();

    auto stagehand = stagehandDatabase->createStagehand(arguments.at(0), parameters);
    bool done = m_universe->executeForClient(connectionId, [&](WorldServer* world, PlayerPtr player) {
        stagehand->setPosition(player->aimPosition());
        world->addEntity(stagehand);
      });

    return done ? "" : "Invalid client state";
  } catch (StarException const& exception) {
    Logger::warn("Could not spawn Stagehand of type '{}', exception caught: {}", argumentString, outputException(exception, false));
    return strf("Could not spawn Stagehand of type '{}'", argumentString);
  }
}

String CommandProcessor::clearStagehand(ConnectionId connectionId, String const&) {
  if (auto errorMsg = adminCheck(connectionId, "remove stagehands"))
    return *errorMsg;

  unsigned removed = 0;
  bool done = m_universe->executeForClient(connectionId,
      [&](WorldServer* world, PlayerPtr player) {
        auto queryRect = RectF::withCenter(player->aimPosition(), Vec2F{2, 2});
        for (auto stagehand : world->query<Stagehand>(queryRect)) {
          world->removeEntity(stagehand->entityId(), true);
          ++removed;
        }
      });
  return done ? strf("Removed {} stagehands", removed) : "Invalid client state";
}

String CommandProcessor::spawnLiquid(ConnectionId connectionId, String const& argumentString) {
  if (auto errorMsg = adminCheck(connectionId, "spawn liquid"))
    return *errorMsg;

  try {
    auto arguments = m_parser.tokenizeToStringList(argumentString);

    auto liquidsDatabase = Root::singleton().liquidsDatabase();

    if (!liquidsDatabase->isLiquidName(arguments.at(0)))
      return strf("No such liquid {}", arguments.at(0));

    LiquidId liquid = liquidsDatabase->liquidId(arguments.at(0));

    float quantity = 1.0f;
    if (arguments.size() > 1) {
      if (auto maybeQuantity = maybeLexicalCast<float>(arguments.at(1)))
        quantity = *maybeQuantity;
      else
        return strf("Could not parse quantity value '{}'", arguments.at(1));
    }

    bool done = m_universe->executeForClient(connectionId, [&](WorldServer* world, PlayerPtr const& player) {
        world->modifyTile(Vec2I(player->aimPosition().floor()), PlaceLiquid{liquid, quantity}, true);
      });
    return done ? "" : "Invalid client state";

  } catch (StarException const& exception) {
    Logger::warn(
        "Could not spawn liquid '{}', exception caught: {}", argumentString, outputException(exception, false));
    return "Could not spawn liquid.";
  }
}

String CommandProcessor::kick(ConnectionId connectionId, String const& argumentString) {
  if (auto errorMsg = adminCheck(connectionId, "kick a user"))
    return *errorMsg;

  auto arguments = m_parser.tokenizeToStringList(argumentString);

  if (arguments.empty())
    return "No player specified";

  auto toKick = playerCidFromCommand(arguments[0], m_universe);
  if (!toKick)
    return strf("No user with specifier {} found.", arguments[0]);

  // Like IRC, if only the nick is passed then the nick is used as the reason
  if (arguments.size() == 1)
    arguments.append(m_universe->clientNick(*toKick));

  m_universe->disconnectClient(*toKick, arguments[1]);

  return strf("Successfully kicked user with specifier {}. ConnectionId: {}. Reason given: {}",
      arguments[0],
      toKick,
      arguments[1]);
}

String CommandProcessor::ban(ConnectionId connectionId, String const& argumentString) {
  if (auto errorMsg = adminCheck(connectionId, "ban a user"))
    return *errorMsg;

  auto arguments = m_parser.tokenizeToStringList(argumentString);

  if (arguments.empty())
    return "No player specified";

  auto toKick = playerCidFromCommand(arguments[0], m_universe);
  if (!toKick)
    return strf("No user with specifier {} found.", arguments[0]);

  String reason = arguments[0];
  if (arguments.size() < 2)
    reason = m_universe->clientNick(*toKick);
  else
    reason = arguments[1];

  pair<bool, bool> type = {true, true};

  if (arguments.size() >= 3) {
    if (arguments[2] == "ip") {
      type = {true, false};
    } else if (arguments[2] == "uuid") {
      type = {false, true};
    } else if (arguments[2] == "both") {
      type = {true, true};
    } else {
      return strf("Invalid argument {} passed as ban type to /ban.  Options are ip, uuid, or both.", arguments[2]);
    }
  }

  Maybe<int> banTime;
  if (arguments.size() == 4) {
    try {
      banTime = lexicalCast<int>(arguments[3]);
    } catch (BadLexicalCast const&) {
      return strf("Invalid argument {} passed as ban time to /ban.", arguments[3]);
    }
  }

  m_universe->banUser(*toKick, reason, type, banTime);

  return strf("Successfully kicked user with specifier {}. ConnectionId: {}. Reason given: {}",
      arguments[0], toKick, reason);
}

String CommandProcessor::unbanIp(ConnectionId connectionId, String const& argumentString) {
  if (auto errorMsg = adminCheck(connectionId, "unban a user"))
    return *errorMsg;

  auto arguments = m_parser.tokenizeToStringList(argumentString);

  if (arguments.empty())
    return "No IP specified";

  bool success = m_universe->unbanIp(arguments[0]);

  if (success)
    return strf("Successfully removed IP {} from ban list", arguments[0]);
  else
    return strf("'{}' is not a valid IP or was not found in the bans list", arguments[0]);
}

String CommandProcessor::unbanUuid(ConnectionId connectionId, String const& argumentString) {
  if (auto errorMsg = adminCheck(connectionId, "unban a user"))
    return *errorMsg;

  auto arguments = m_parser.tokenizeToStringList(argumentString);

  if (arguments.empty())
    return "No UUID specified";

  bool success = m_universe->unbanUuid(arguments[0]);

  if (success)
    return strf("Successfully removed UUID {} from ban list", arguments[0]);
  else
    return strf("'{}' is not a valid UUID or was not found in the bans list", arguments[0]);
}

String CommandProcessor::list(ConnectionId connectionId, String const&) {
  if (auto errorMsg = adminCheck(connectionId, "list clients"))
    return *errorMsg;

  StringList res;

  auto assets = Root::singleton().assets();
  for (auto cid : m_universe->clientIds())
    res.append(strf("${} : {} : $${}", cid, m_universe->clientNick(cid), m_universe->uuidForClient(cid)->hex()));

  return res.join("\n");
}

String CommandProcessor::clientCoordinate(ConnectionId connectionId, String const& argumentString) {
  ConnectionId targetClientId = connectionId;
  String targetLabel = "Your";
  auto arguments = m_parser.tokenizeToStringList(argumentString);
  if (!adminCheck(connectionId, "find other players")) {
    if (arguments.size() > 0) {
      auto cid = playerCidFromCommand(arguments[0], m_universe);
      if (!cid)
        return strf("No user with specifier {} found.", arguments[0]);
      targetClientId = *cid;
      targetLabel = strf("Client {}'s", arguments[0]);
    }
  }

  if (targetClientId) {
    auto worldId = m_universe->clientWorld(targetClientId);
    return strf("{} current location is {}", targetLabel, worldId);
  } else {
    return "";
  }
}

String CommandProcessor::serverReload(ConnectionId connectionId, String const&) {
  if (auto errorMsg = adminCheck(connectionId, "trigger root reload"))
    return *errorMsg;

  auto& root = Root::singleton();
  root.reload();
  root.fullyLoad();
  return "";
}

String CommandProcessor::eval(ConnectionId connectionId, String const& lua) {
  if (auto errorMsg = localCheck(connectionId, "execute server script"))
    return *errorMsg;

  if (auto errorMsg = adminCheck(connectionId, "execute server script"))
    return *errorMsg;

  return toString(m_scriptComponent.context()->eval(lua));
}

String CommandProcessor::entityEval(ConnectionId connectionId, String const& lua) {
  if (auto errorMsg = localCheck(connectionId, "execute server entity script"))
    return *errorMsg;

  if (auto errorMsg = adminCheck(connectionId, "execute server entity script"))
    return *errorMsg;

  String message;
  bool done = m_universe->executeForClient(connectionId,
      [&lua, &message](WorldServer* world, PlayerPtr const& player) {
        auto queryRect = RectF::withCenter(player->aimPosition(), Vec2F{2, 2});
        auto entities = world->query<ScriptedEntity>(queryRect);
        if (entities.empty()) {
          message = "Could not find scripted entity at cursor";
          return;
        }

        ScriptedEntityPtr targetEntity;
        for (auto const& entity : entities) {
          if (!targetEntity
              || vmagSquared(entity->position() - player->aimPosition())
                  < vmagSquared(targetEntity->position() - player->aimPosition()))
            targetEntity = entity;
        }

        if (auto res = targetEntity->evalScript(lua))
          message = toString(*res);
        else
          message = "Error evaluating script in entity context, check log";
      });

  return done ? message : "failed to do entity eval";
}

String CommandProcessor::enableSpawning(ConnectionId connectionId, String const&) {
  if (auto errorMsg = adminCheck(connectionId, "enable world spawning"))
    return *errorMsg;

  bool done = m_universe->executeForClient(
      connectionId, [](WorldServer* world, PlayerPtr const&) { world->setSpawningEnabled(true); });
  return done ? "enabled monster spawning" : "enabling monster spawning failed";
}

String CommandProcessor::disableSpawning(ConnectionId connectionId, String const&) {
  if (auto errorMsg = adminCheck(connectionId, "disable world spawning"))
    return *errorMsg;

  bool done = m_universe->executeForClient(
      connectionId, [](WorldServer* world, PlayerPtr const&) { world->setSpawningEnabled(false); });
  return done ? "disabled monster spawning" : "disabling monster spawning failed";
}

String CommandProcessor::placeDungeon(ConnectionId connectionId, String const& argumentString) {
  if (auto errorMsg = adminCheck(connectionId, "place dungeons"))
    return *errorMsg;

  auto arguments = m_parser.tokenizeToStringList(argumentString);
  String dungeonName = arguments.at(0);

  Maybe<Vec2I> targetPosition;
  if (arguments.size() > 1) {
    auto pos = arguments.at(1).split(",", 1);
    targetPosition = Vec2I(lexicalCast<int>(pos.at(0)), lexicalCast<int>(pos.at(1)));
  }

  bool done = m_universe->executeForClient(connectionId,
      [dungeonName, targetPosition](WorldServer* world, PlayerPtr const& player) {
        world->placeDungeon(dungeonName, targetPosition.value(Vec2I::floor(player->aimPosition())), true);
      });

  return done ? "" : "Unable to place dungeon " + dungeonName;
}

String CommandProcessor::setUniverseFlag(ConnectionId connectionId, String const& argumentString) {
  if (auto errorMsg = adminCheck(connectionId, "set universe flags"))
    return *errorMsg;

  auto arguments = m_parser.tokenizeToStringList(argumentString);
  String flag = arguments.at(0);
  m_universe->universeSettings()->setFlag(flag);

  return "set universe flag " + flag;
}

String CommandProcessor::resetUniverseFlags(ConnectionId connectionId, String const&) {
  if (auto errorMsg = adminCheck(connectionId, "reset universe flags"))
    return *errorMsg;

  m_universe->universeSettings()->resetFlags();
  return "universe flags reset!";
}

String CommandProcessor::addBiomeRegion(ConnectionId connectionId, String const& argumentString) {
  if (auto errorMsg = adminCheck(connectionId, "add biome regions"))
    return *errorMsg;

  auto arguments = m_parser.tokenizeToStringList(argumentString);

  String biomeName = arguments.at(0);
  int width = lexicalCast<int>(arguments.at(1));

  String subBlockSelector = "largeClumps";
  if (arguments.size() > 2)
    subBlockSelector = arguments.at(2);

  bool done = m_universe->executeForClient(connectionId,
      [biomeName, width, subBlockSelector](WorldServer* world, PlayerPtr const& player) {
        world->addBiomeRegion(Vec2I::floor(player->aimPosition()), biomeName, subBlockSelector, width);
      });

  return done ? strf("added region of biome {} with width {}", biomeName, width) : "failed to add biome region";
}

String CommandProcessor::expandBiomeRegion(ConnectionId connectionId, String const& argumentString) {
  if (auto errorMsg = adminCheck(connectionId, "expand biome regions"))
    return *errorMsg;

  auto arguments = m_parser.tokenizeToStringList(argumentString);

  int newWidth = lexicalCast<int>(arguments.at(0));

  bool done = m_universe->executeForClient(connectionId,
      [newWidth](WorldServer* world, PlayerPtr const& player) {
        world->expandBiomeRegion(Vec2I::floor(player->aimPosition()), newWidth);
      });

  return done ? strf("expanded region to width {}", newWidth) : "failed to expand biome region";
}

String CommandProcessor::updatePlanetType(ConnectionId connectionId, String const& argumentString) {
  if (auto errorMsg = adminCheck(connectionId, "update planet type"))
    return *errorMsg;

  auto arguments = m_parser.tokenizeToStringList(argumentString);

  auto coordinate = CelestialCoordinate(arguments.at(0));
  auto newType = arguments.at(1);
  auto weatherBiome = arguments.at(2);

  bool done = m_universe->updatePlanetType(coordinate, newType, weatherBiome);

  return done ? strf("set planet at {} to type {} weatherBiome {}", coordinate, newType, weatherBiome) : "failed to update planet type";
}

String CommandProcessor::setEnvironmentBiome(ConnectionId connectionId, String const&) {
  if (auto errorMsg = adminCheck(connectionId, "update layer environment biome"))
    return *errorMsg;

  bool done = m_universe->executeForClient(connectionId,
      [](WorldServer* world, PlayerPtr const& player) {
        world->setLayerEnvironmentBiome(Vec2I::floor(player->aimPosition()));
      });

  return done ? "set environment biome for world layer" : "failed to set environment biome";
}

Maybe<ConnectionId> CommandProcessor::playerCidFromCommand(String const& player, UniverseServer* universe) {
  char const* const UsernamePrefix = "@";
  char const* const CidPrefix = "$";
  char const* const UUIDPrefix = "$$";

  if (player.beginsWith(UsernamePrefix)) {
    return universe->findNick(player.substr(strlen(UsernamePrefix)));
  } else if (player.beginsWith(UUIDPrefix)) {
    try {
      auto uuidString = player.substr(strlen(UUIDPrefix));
      return universe->clientForUuid(Uuid(uuidString));
    } catch (UuidException const&) {
      // pass to base case
    }
  } else if (player.beginsWith(CidPrefix)) {
    auto cidString = player.substr(strlen(CidPrefix));
    auto cid = maybeLexicalCast<ConnectionId>(cidString).value(ServerConnectionId);
    if (universe->isConnectedClient(cid))
      return cid;
  }

  return universe->findNick(player);
}

//wow, wtf. TODO: replace with hashmap
String CommandProcessor::handleCommand(ConnectionId connectionId, String const& command, String const& argumentString) {
  if (command == "admin") {
    return admin(connectionId, argumentString);
  } else if (command == "timewarp") {
    return timewarp(connectionId, argumentString);
  } else if (command == "timescale") {
    return timescale(connectionId, argumentString);
  } else if (command == "tickrate") {
    return tickrate(connectionId, argumentString);
  } else if (command == "settileprotection") {
    return setTileProtection(connectionId, argumentString);
  } else if (command == "setdungeonid") {
    return setDungeonId(connectionId, argumentString);
  } else if (command == "setspawnpoint") {
    return setPlayerStart(connectionId, argumentString);
  } else if (command == "spawnitem") {
    return spawnItem(connectionId, argumentString);
  } else if (command == "spawntreasure") {
    return spawnTreasure(connectionId, argumentString);
  } else if (command == "spawnmonster") {
    return spawnMonster(connectionId, argumentString);
  } else if (command == "spawnnpc") {
    return spawnNpc(connectionId, argumentString);
  } else if (command == "spawnstagehand") {
    return spawnStagehand(connectionId, argumentString);
  } else if (command == "clearstagehand") {
    return clearStagehand(connectionId, argumentString);
  } else if (command == "spawnvehicle") {
    return spawnVehicle(connectionId, argumentString);
  } else if (command == "spawnliquid") {
    return spawnLiquid(connectionId, argumentString);
  } else if (command == "pvp") {
    return pvp(connectionId, argumentString);
  } else if (command == "serverwhoami") {
    return whoami(connectionId, argumentString);
  } else if (command == "kick") {
    return kick(connectionId, argumentString);
  } else if (command == "ban") {
    return ban(connectionId, argumentString);
  } else if (command == "unbanip") {
    return unbanIp(connectionId, argumentString);
  } else if (command == "unbanuuid") {
    return unbanUuid(connectionId, argumentString);
  } else if (command == "list") {
    return list(connectionId, argumentString);
  } else if (command == "help") {
    return help(connectionId, argumentString);
  } else if (command == "warp") {
    return warp(connectionId, argumentString);
  } else if (command == "warprandom") {
    return warpRandom(connectionId, argumentString);
  } else if (command == "whereami") {
    return clientCoordinate(connectionId, argumentString);
  } else if (command == "whereis") {
    return clientCoordinate(connectionId, argumentString);
  } else if (command == "serverreload") {
    return serverReload(connectionId, argumentString);
  } else if (command == "eval") {
    return eval(connectionId, argumentString);
  } else if (command == "entityeval") {
    return entityEval(connectionId, argumentString);
  } else if (command == "enablespawning") {
    return enableSpawning(connectionId, argumentString);
  } else if (command == "disablespawning") {
    return disableSpawning(connectionId, argumentString);
  } else if (command == "placedungeon") {
    return placeDungeon(connectionId, argumentString);
  } else if (command == "setuniverseflag") {
    return setUniverseFlag(connectionId, argumentString);
  } else if (command == "resetuniverseflags") {
    return resetUniverseFlags(connectionId, argumentString);
  } else if (command == "addbiomeregion") {
    return addBiomeRegion(connectionId, argumentString);
  } else if (command == "expandbiomeregion") {
    return expandBiomeRegion(connectionId, argumentString);
  } else if (command == "updateplanettype") {
    return updatePlanetType(connectionId, argumentString);
  } else if (command == "setenvironmentbiome") {
    return setEnvironmentBiome(connectionId, argumentString);
  } else if (auto res = m_scriptComponent.invoke("command", command, connectionId, jsonFromStringList(m_parser.tokenizeToStringList(argumentString)))) {
    return toString(*res);
  } else {
    return strf("No such command {}", command);
  }
}

Maybe<String> CommandProcessor::adminCheck(ConnectionId connectionId, String const& commandDescription) const {
  if (connectionId == ServerConnectionId)
    return {};

  auto config = Root::singleton().configuration();
  if (!config->get("allowAdminCommands").toBool())
    return {"Admin commands disabled on this server."};
  if (!config->get("allowAdminCommandsFromAnyone").toBool()) {
    if (!m_universe->isAdmin(connectionId))
      return {strf("Insufficient privileges to {}.", commandDescription)};
  }

  return {};
}

Maybe<String> CommandProcessor::localCheck(ConnectionId connectionId, String const& commandDescription) const {
  if (connectionId == ServerConnectionId)
    return {};

  if (!m_universe->isLocal(connectionId))
    return {strf("The {} command can only be used locally.", commandDescription)};

  return {};
}

LuaCallbacks CommandProcessor::makeCommandCallbacks() {
  LuaCallbacks callbacks;
  callbacks.registerCallbackWithSignature<Maybe<String>, ConnectionId, String>(
      "adminCheck", bind(&CommandProcessor::adminCheck, this, _1, _2));
  return callbacks;
}

}

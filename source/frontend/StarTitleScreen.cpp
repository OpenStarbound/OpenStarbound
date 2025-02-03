#include "StarTitleScreen.hpp"
#include "StarEncode.hpp"
#include "StarGuiReader.hpp"
#include "StarRoot.hpp"
#include "StarJsonExtra.hpp"
#include "StarPlayer.hpp"
#include "StarGuiContext.hpp"
#include "StarPaneManager.hpp"
#include "StarButtonWidget.hpp"
#include "StarListWidget.hpp"
#include "StarLabelWidget.hpp"
#include "StarCharSelection.hpp"
#include "StarCharCreation.hpp"
#include "StarTextBoxWidget.hpp"
#include "StarOptionsMenu.hpp"
#include "StarModsMenu.hpp"
#include "StarAssets.hpp"
#include "StarCelestialDatabase.hpp"
#include "StarEnvironmentPainter.hpp"

namespace Star {

TitleScreen::TitleScreen(PlayerStoragePtr playerStorage, MixerPtr mixer, UniverseClientPtr client)
  : m_playerStorage(playerStorage), m_skipMultiPlayerConnection(false), m_mixer(mixer) {
  m_titleState = TitleState::Quit;

  auto assets = Root::singleton().assets();

  m_guiContext = GuiContext::singletonPtr();

  m_celestialDatabase = make_shared<CelestialMasterDatabase>();
  auto randomWorld = m_celestialDatabase->findRandomWorld(10, 50, [this](CelestialCoordinate const& coordinate) {
      return is<TerrestrialWorldParameters>(m_celestialDatabase->parameters(coordinate)->visitableParameters());
    }).take();

  if (auto name = m_celestialDatabase->name(randomWorld))
    Logger::info("Title world is {} @ CelestialWorld:{}", Text::stripEscapeCodes(*name), randomWorld);

  SkyParameters skyParameters(randomWorld, m_celestialDatabase);
  m_skyBackdrop = make_shared<Sky>(skyParameters, true);

  m_musicTrack = make_shared<AmbientNoisesDescription>(assets->json("/interface/windowconfig/title.config:music").toObject(), "/");

  initMainMenu();
  initCharSelectionMenu();
  initCharCreationMenu();
  initMultiPlayerMenu();
  initOptionsMenu(client);
  initModsMenu();

  resetState();
}

void TitleScreen::renderInit(RendererPtr renderer) {
  m_renderer = std::move(renderer);
  m_environmentPainter = make_shared<EnvironmentPainter>(m_renderer);
}

void TitleScreen::render() {
  auto assets = Root::singleton().assets();

  float pixelRatio = m_guiContext->interfaceScale();
  Vec2F screenSize = Vec2F(m_guiContext->windowSize());
  auto skyRenderData = m_skyBackdrop->renderData();

  float pixelRatioBasis = screenSize[1] / 1080.0f;
  float starAndDebrisRatio = lerp(0.0625f, pixelRatioBasis * 2.0f, pixelRatio);
  float orbiterAndPlanetRatio = lerp(0.125f, pixelRatioBasis * 3.0f, pixelRatio);

  m_environmentPainter->renderStars(starAndDebrisRatio, screenSize, skyRenderData);
  m_environmentPainter->renderDebrisFields(starAndDebrisRatio, screenSize, skyRenderData);
  m_environmentPainter->renderBackOrbiters(orbiterAndPlanetRatio, screenSize, skyRenderData);
  m_environmentPainter->renderPlanetHorizon(orbiterAndPlanetRatio, screenSize, skyRenderData);
  m_environmentPainter->renderSky(screenSize, skyRenderData);
  m_environmentPainter->renderFrontOrbiters(orbiterAndPlanetRatio, screenSize, skyRenderData);

  m_renderer->flush();

  auto skyBackdropDarken = jsonToColor(assets->json("/interface/windowconfig/title.config:skyBackdropDarken"));
  m_renderer->render(renderFlatRect(RectF(0, 0, windowWidth(), windowHeight()), skyBackdropDarken.toRgba(), 0.0f));

  m_renderer->flush();

  for (auto& backdropImage : assets->json("/interface/windowconfig/title.config:backdropImages").toArray()) {
    Vec2F offset = jsonToVec2F(backdropImage.get(0)) * interfaceScale();
    String image = backdropImage.getString(1);
    float scale = backdropImage.getFloat(2);
    Vec2F origin = jsonToVec2F(backdropImage.getArray(3, { 0.5f, 1.0f }));
    Vec2F imageSize = Vec2F(m_guiContext->textureSize(image)) * interfaceScale() * scale;

    Vec2F position = Vec2F(m_guiContext->windowSize()).piecewiseMultiply(origin);
    position += offset - imageSize.piecewiseMultiply(origin);
    RectF screenCoords(position, position + imageSize);
    m_guiContext->drawQuad(image, screenCoords);
  }

  m_renderer->flush();

  m_paneManager.render();
  renderCursor();

  m_renderer->flush();
}

bool TitleScreen::handleInputEvent(InputEvent const& event) {
  if (auto mouseMove = event.ptr<MouseMoveEvent>())
    m_cursorScreenPos = mouseMove->mousePosition;

  if (event.is<KeyDownEvent>()) {
    if (GuiContext::singleton().actions(event).contains(InterfaceAction::TitleBack)) {
      back();
      return true;
    }
  }

  return m_paneManager.sendInputEvent(event);
}

void TitleScreen::update(float dt) {
  m_cursor.update(dt);

  for (auto p : m_rightAnchoredButtons)
    p.first->setPosition(Vec2I(m_guiContext->windowWidth() / m_guiContext->interfaceScale(), 0) + p.second);
  m_mainMenu->determineSizeFromChildren();

  m_skyBackdrop->update(dt);
  m_environmentPainter->update(dt);

  m_paneManager.update(dt);

  if (!finishedState()) {
    if (auto audioSample = m_musicTrackManager.updateAmbient(m_musicTrack, m_skyBackdrop->isDayTime())) {
      m_currentMusicTrack = audioSample;
      audioSample->setMixerGroup(MixerGroup::Music);
      audioSample->setLoops(0);
      m_mixer->play(audioSample);
    }
  }
}

bool TitleScreen::textInputActive() const {
  return m_paneManager.keyboardCapturedForTextInput();
}

TitleState TitleScreen::currentState() const {
  return m_titleState;
}

bool TitleScreen::finishedState() const {
  switch (m_titleState) {
    case TitleState::StartSinglePlayer:
    case TitleState::StartMultiPlayer:
    case TitleState::Quit:
      return true;
    default:
      return false;
  }
}

void TitleScreen::resetState() {
  switchState(TitleState::Main);
  if (m_currentMusicTrack)
    m_currentMusicTrack->setVolume(1.0f, 4.0f);
}

void TitleScreen::goToMultiPlayerSelectCharacter(bool skipConnection) {
  m_skipMultiPlayerConnection = skipConnection;
  switchState(TitleState::MultiPlayerSelectCharacter);
}

void TitleScreen::stopMusic() {
  if (m_currentMusicTrack)
    m_currentMusicTrack->stop(8.0f);
}

PlayerPtr TitleScreen::currentlySelectedPlayer() const {
  return m_mainAppPlayer;
}

String TitleScreen::multiPlayerAddress() const {
  return m_connectionAddress;
}

void TitleScreen::setMultiPlayerAddress(String address) {
  m_multiPlayerMenu->fetchChild<TextBoxWidget>("address")->setText(address);
  m_connectionAddress = std::move(address);
}

String TitleScreen::multiPlayerPort() const {
  return m_connectionPort;
}

void TitleScreen::setMultiPlayerPort(String port) {
  m_multiPlayerMenu->fetchChild<TextBoxWidget>("port")->setText(port);
  m_connectionPort = std::move(port);
}

String TitleScreen::multiPlayerAccount() const {
  return m_account;
}

void TitleScreen::setMultiPlayerAccount(String account) {
  m_multiPlayerMenu->fetchChild<TextBoxWidget>("account")->setText(account);
  m_account = std::move(account);
}

String TitleScreen::multiPlayerPassword() const {
  return m_password;
}

void TitleScreen::setMultiPlayerPassword(String password) {
  m_multiPlayerMenu->fetchChild<TextBoxWidget>("password")->setText(password);
  m_password = std::move(password);
}

void TitleScreen::initMainMenu() {
  m_mainMenu = make_shared<Pane>();
  auto backMenu = make_shared<Pane>();

  auto assets = Root::singleton().assets();

  StringMap<WidgetCallbackFunc> buttonCallbacks;
  buttonCallbacks["singleplayer"] = [=](Widget*) { switchState(TitleState::SinglePlayerSelectCharacter); };
  buttonCallbacks["multiplayer"] = [=](Widget*) { switchState(TitleState::MultiPlayerSelectCharacter); };
  buttonCallbacks["options"] = [=](Widget*) { switchState(TitleState::Options); };
  buttonCallbacks["quit"] = [=](Widget*) { switchState(TitleState::Quit); };
  buttonCallbacks["back"] = [=](Widget*) { back(); };
  buttonCallbacks["mods"] = [=](Widget*) { switchState(TitleState::Mods); };

  for (auto buttonConfig : assets->json("/interface/windowconfig/title.config:mainMenuButtons").toArray()) {
    String key = buttonConfig.getString("key");
    String image = buttonConfig.getString("button");
    String imageHover = buttonConfig.getString("hover");
    Vec2I offset = jsonToVec2I(buttonConfig.get("offset"));
    WidgetCallbackFunc callback = buttonCallbacks.get(key);
    bool rightAnchored = buttonConfig.getBool("rightAnchored", false);

    auto button = make_shared<ButtonWidget>(callback, image, imageHover, "", "");
    button->setPosition(offset);

    if (rightAnchored)
      m_rightAnchoredButtons.append({button, offset});

    if (key == "back")
      backMenu->addChild(key, button);
    else
      m_mainMenu->addChild(key, button);
  }

  m_mainMenu->setAnchor(PaneAnchor::BottomLeft);
  m_mainMenu->lockPosition();

  backMenu->determineSizeFromChildren();
  backMenu->setAnchor(PaneAnchor::BottomLeft);
  backMenu->lockPosition();

  m_paneManager.registerPane("mainMenu", PaneLayer::Hud, m_mainMenu);
  m_paneManager.registerPane("backMenu", PaneLayer::Hud, backMenu);
}

void TitleScreen::initCharSelectionMenu() {
  auto deleteDialog = make_shared<Pane>();

  GuiReader reader;

  reader.registerCallback("delete", [=](Widget*) { deleteDialog->dismiss(); });
  reader.registerCallback("cancel", [=](Widget*) { deleteDialog->dismiss(); });

  reader.construct(Root::singleton().assets()->json("/interface/windowconfig/deletedialog.config"), deleteDialog.get());

  auto charSelectionMenu = make_shared<CharSelectionPane>(m_playerStorage, [=]() {
      if (m_titleState == TitleState::SinglePlayerSelectCharacter)
        switchState(TitleState::SinglePlayerCreateCharacter);
      else if (m_titleState == TitleState::MultiPlayerSelectCharacter)
        switchState(TitleState::MultiPlayerCreateCharacter);
    }, [=](PlayerPtr mainPlayer) {
      m_mainAppPlayer = mainPlayer;
      m_playerStorage->moveToFront(m_mainAppPlayer->uuid());
      if (m_titleState == TitleState::SinglePlayerSelectCharacter) {
        switchState(TitleState::StartSinglePlayer);
        } else if (m_titleState == TitleState::MultiPlayerSelectCharacter) {
          if (m_skipMultiPlayerConnection)
            switchState(TitleState::StartMultiPlayer);
          else
            switchState(TitleState::MultiPlayerConnect);
        }
    }, [=](Uuid playerUuid) {
      auto deleteDialog = m_paneManager.registeredPane("deleteDialog");
      deleteDialog->fetchChild<ButtonWidget>("delete")->setCallback([=](Widget*) {
        m_playerStorage->deletePlayer(playerUuid);
        deleteDialog->dismiss();
      });
      m_paneManager.displayRegisteredPane("deleteDialog");
    });
  charSelectionMenu->setAnchor(PaneAnchor::Center);
  charSelectionMenu->lockPosition();

  m_paneManager.registerPane("deleteDialog", PaneLayer::ModalWindow, deleteDialog, [=](PanePtr const&) {
      charSelectionMenu->updateCharacterPlates();
    });
  m_paneManager.registerPane("charSelectionMenu", PaneLayer::Hud, charSelectionMenu);
}

void TitleScreen::initCharCreationMenu() {
  auto charCreationMenu = make_shared<CharCreationPane>([=](PlayerPtr newPlayer) {
    if (newPlayer) {
      m_mainAppPlayer = newPlayer;
      m_playerStorage->savePlayer(m_mainAppPlayer);
      m_playerStorage->moveToFront(m_mainAppPlayer->uuid());
    }
    back();
  });
  charCreationMenu->setAnchor(PaneAnchor::Center);
  charCreationMenu->lockPosition();

  m_paneManager.registerPane("charCreationMenu", PaneLayer::Hud, charCreationMenu);
}


void TitleScreen::populateServerList(ListWidgetPtr list){
  if (!m_serverList.isNull()) {
    list->clear();
    for (auto const& server : m_serverList.iterateArray()) {
      auto listItem = list->addItem();
      listItem->fetchChild<LabelWidget>("address")->setText(server.getString("address"));
      listItem->fetchChild<LabelWidget>("account")->setText(server.get("account", "").toString());
      listItem->setData(server);
    }
  }
};

void TitleScreen::initMultiPlayerMenu() {
  m_multiPlayerMenu = make_shared<Pane>();
  m_serverSelectPane = make_shared<Pane>();

  GuiReader readerConnect;
  GuiReader readerServer;

  m_serverList = Root::singleton().configuration()->get("serverList");
  if (!m_serverList.isType(Json::Type::Array))
    m_serverList = JsonArray();

  auto assets = Root::singleton().assets();

  readerServer.registerCallback("saveServer", [=](Widget*) {
    Json serverData = JsonObject{
      {"address", multiPlayerAddress()},
      {"account", multiPlayerAccount()},
      {"port", multiPlayerPort()},
      //{"password", multiPlayerPassword()}
    };

    auto serverList = m_serverSelectPane->fetchChild<ListWidget>("serverSelectArea.serverList");
    if (auto const pos = serverList->selectedItem(); pos != NPos) { // Edit existing
      m_serverList = m_serverList.set(pos, serverData);
    } else { // Save new
      m_serverList = m_serverList.insert(0, serverData);
    }

    populateServerList(serverList);
    Root::singleton().configuration()->set("serverList", m_serverList);
  });

  readerServer.construct(assets->json("/interface/windowconfig/serverselect.config"), m_serverSelectPane.get());



  auto serverList = m_serverSelectPane->fetchChild<ListWidget>("serverSelectArea.serverList");
  
  serverList->registerMemberCallback("delete", [=](Widget* widget) {
    if (auto const pos = serverList->selectedItem(); pos != NPos) {
      m_serverList = m_serverList.eraseIndex(pos);
    }
    populateServerList(serverList);
    Root::singleton().configuration()->set("serverList", m_serverList);
  });

  serverList->setCallback([=](Widget* widget) {
    if (auto selectedItem = serverList->selectedWidget()) {
      if (selectedItem->findChild<ButtonWidget>("delete")->isHovered())
        return;
      auto& data = selectedItem->data();
      setMultiPlayerAddress(data.getString("address", ""));
      setMultiPlayerPort(data.getString("port", ""));
      setMultiPlayerAccount(data.getString("account", ""));
      setMultiPlayerPassword(data.getString("password", ""));
    }
  });

  readerConnect.registerCallback("address", [=](Widget* obj) {
      m_connectionAddress = convert<TextBoxWidget>(obj)->getText().trim();
      m_serverSelectPane->fetchChild<ButtonWidget>("save")->setVisibility(multiPlayerAddress().length() > 0);
    });

  readerConnect.registerCallback("port", [=](Widget* obj) {
      m_connectionPort = convert<TextBoxWidget>(obj)->getText().trim();
    });

  readerConnect.registerCallback("account", [=](Widget* obj) {
      m_account = convert<TextBoxWidget>(obj)->getText().trim();
    });

  readerConnect.registerCallback("password", [=](Widget* obj) {
      m_password = convert<TextBoxWidget>(obj)->getText().trim();
    });

  readerConnect.registerCallback("connect", [=](Widget*) {
    switchState(TitleState::StartMultiPlayer);
    });


  readerConnect.construct(assets->json("/interface/windowconfig/multiplayer.config"), m_multiPlayerMenu.get());

  populateServerList(serverList);

  m_paneManager.registerPane("multiplayerMenu", PaneLayer::Hud, m_multiPlayerMenu);
  m_paneManager.registerPane("serverSelect", PaneLayer::Hud, m_serverSelectPane, [=](PanePtr const&) {
    serverList->clearSelected();
  });
}

void TitleScreen::initOptionsMenu(UniverseClientPtr client) {
  auto optionsMenu = make_shared<OptionsMenu>(&m_paneManager,client);
  optionsMenu->setAnchor(PaneAnchor::Center);
  optionsMenu->lockPosition();

  m_paneManager.registerPane("optionsMenu", PaneLayer::Hud, optionsMenu, [this](PanePtr const&) {
      back();
    });
}

void TitleScreen::initModsMenu() {
  auto modsMenu = make_shared<ModsMenu>();
  modsMenu->setAnchor(PaneAnchor::Center);
  modsMenu->lockPosition();

  m_paneManager.registerPane("modsMenu", PaneLayer::Hud, modsMenu, [this](PanePtr const&) {
      back();
    });
}

void TitleScreen::switchState(TitleState titleState) {
  if (m_titleState == titleState)
    return;

  m_paneManager.dismissAllPanes();
  m_titleState = titleState;

  // Clear the 'skip multi player connection' flag if we leave the multi player
  // menus
  if (m_titleState < TitleState::MultiPlayerSelectCharacter || m_titleState > TitleState::MultiPlayerConnect)
    m_skipMultiPlayerConnection = false;

  if (titleState == TitleState::Main) {
    m_paneManager.displayRegisteredPane("mainMenu");
  } else {
    m_paneManager.displayRegisteredPane("backMenu");

    if (titleState == TitleState::Options) {
      m_paneManager.displayRegisteredPane("optionsMenu");
    } if (titleState == TitleState::Mods) {
      m_paneManager.displayRegisteredPane("modsMenu");
    } else if (titleState == TitleState::SinglePlayerSelectCharacter) {
      m_paneManager.displayRegisteredPane("charSelectionMenu");
    } else if (titleState == TitleState::SinglePlayerCreateCharacter) {
      m_paneManager.displayRegisteredPane("charCreationMenu");
    } else if (titleState == TitleState::MultiPlayerSelectCharacter) {
      m_paneManager.displayRegisteredPane("charSelectionMenu");
    } else if (titleState == TitleState::MultiPlayerCreateCharacter) {
      m_paneManager.displayRegisteredPane("charCreationMenu");
    } else if (titleState == TitleState::MultiPlayerConnect) {
      m_paneManager.displayRegisteredPane("multiplayerMenu");
      m_paneManager.displayRegisteredPane("serverSelect");
      if (auto addressWidget = m_multiPlayerMenu->fetchChild("address"))
        addressWidget->focus();
    }
  }

  if (titleState == TitleState::Quit)
    m_musicTrackManager.cancelAll();
}

void TitleScreen::back() {
  if (m_titleState == TitleState::Options)
    switchState(TitleState::Main);
  else if (m_titleState == TitleState::Mods)
    switchState(TitleState::Main);
  else if (m_titleState == TitleState::SinglePlayerSelectCharacter)
    switchState(TitleState::Main);
  else if (m_titleState == TitleState::SinglePlayerCreateCharacter)
    switchState(TitleState::SinglePlayerSelectCharacter);
  else if (m_titleState == TitleState::MultiPlayerSelectCharacter)
    switchState(TitleState::Main);
  else if (m_titleState == TitleState::MultiPlayerCreateCharacter)
    switchState(TitleState::MultiPlayerSelectCharacter);
  else if (m_titleState == TitleState::MultiPlayerConnect)
    switchState(TitleState::MultiPlayerSelectCharacter);
}

void TitleScreen::renderCursor() {
  auto assets = Root::singleton().assets();

  Vec2I cursorPos = m_cursorScreenPos;
  Vec2I cursorSize = m_cursor.size();
  Vec2I cursorOffset = m_cursor.offset();
  unsigned int cursorScale = m_cursor.scale(interfaceScale());
  Drawable cursorDrawable = m_cursor.drawable();

  cursorPos[0] -= cursorOffset[0] * cursorScale;
  cursorPos[1] -= (cursorSize[1] - cursorOffset[1]) * cursorScale;

  if (!m_guiContext->trySetCursor(cursorDrawable, cursorOffset, cursorScale))
    m_guiContext->drawDrawable(cursorDrawable, Vec2F(cursorPos), cursorScale);
}

float TitleScreen::interfaceScale() const {
  return m_guiContext->interfaceScale();
}

unsigned TitleScreen::windowHeight() const {
  return m_guiContext->windowHeight();
}

unsigned TitleScreen::windowWidth() const {
  return m_guiContext->windowWidth();
}

}

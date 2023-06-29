#include "StarCharCreation.hpp"
#include "StarJsonExtra.hpp"
#include "StarGuiReader.hpp"
#include "StarNameGenerator.hpp"
#include "StarLogging.hpp"
#include "StarRoot.hpp"
#include "StarWorldClient.hpp"
#include "StarSpeciesDatabase.hpp"
#include "StarButtonWidget.hpp"
#include "StarPortraitWidget.hpp"
#include "StarTextBoxWidget.hpp"
#include "StarLabelWidget.hpp"
#include "StarImageWidget.hpp"
#include "StarArmors.hpp"
#include "StarAssets.hpp"
#include "StarPlayerFactory.hpp"
#include "StarItemDatabase.hpp"
#include "StarPlayerInventory.hpp"
#include "StarPlayerLog.hpp"

namespace Star {

CharCreationPane::CharCreationPane(std::function<void(PlayerPtr)> requestCloseFunc) {
  auto& root = Root::singleton();

  m_speciesList = jsonToStringList(root.assets()->json("/interface/windowconfig/charcreation.config:speciesOrdering"));

  GuiReader guiReader;
  guiReader.registerCallback("cancel", [=](Widget*) { requestCloseFunc({}); });
  guiReader.registerCallback("saveChar", [=](Widget*) {
      if (fetchChild<ButtonWidget>("btnSkipIntro")->isChecked())
        m_previewPlayer->log()->setIntroComplete(true);
      requestCloseFunc(m_previewPlayer);
      createPlayer();
      randomize();
      randomizeName();
    });

  guiReader.registerCallback("mainSkinColor.up", [=](Widget*) {
      m_bodyColor++;
      changed();
    });
  guiReader.registerCallback("mainSkinColor.down", [=](Widget*) {
      m_bodyColor--;
      changed();
    });
  guiReader.registerCallback("alty.up", [=](Widget*) {
      m_alty++;
      changed();
    });
  guiReader.registerCallback("alty.down", [=](Widget*) {
      m_alty--;
      changed();
    });
  guiReader.registerCallback("hairStyle.up", [=](Widget*) {
      m_hairChoice++;
      changed();
    });
  guiReader.registerCallback("hairStyle.down", [=](Widget*) {
      m_hairChoice--;
      changed();
    });
  guiReader.registerCallback("shirt.up", [=](Widget*) {
      m_shirtChoice++;
      fetchChild<ButtonWidget>("btnToggleClothing")->setChecked(true);
      changed();
    });
  guiReader.registerCallback("shirt.down", [=](Widget*) {
      m_shirtChoice--;
      fetchChild<ButtonWidget>("btnToggleClothing")->setChecked(true);
      changed();
    });
  guiReader.registerCallback("pants.up", [=](Widget*) {
      m_pantsChoice++;
      fetchChild<ButtonWidget>("btnToggleClothing")->setChecked(true);
      changed();
    });
  guiReader.registerCallback("pants.down", [=](Widget*) {
      m_pantsChoice--;
      fetchChild<ButtonWidget>("btnToggleClothing")->setChecked(true);
      changed();
    });
  guiReader.registerCallback("heady.up", [=](Widget*) {
      m_heady++;
      changed();
    });
  guiReader.registerCallback("heady.down", [=](Widget*) {
      m_heady--;
      changed();
    });
  guiReader.registerCallback("shirtColor.up", [=](Widget*) {
      m_shirtColor++;
      fetchChild<ButtonWidget>("btnToggleClothing")->setChecked(true);
      changed();
    });
  guiReader.registerCallback("shirtColor.down", [=](Widget*) {
      m_shirtColor--;
      fetchChild<ButtonWidget>("btnToggleClothing")->setChecked(true);
      changed();
    });
  guiReader.registerCallback("pantsColor.up", [=](Widget*) {
      m_pantsColor++;
      fetchChild<ButtonWidget>("btnToggleClothing")->setChecked(true);
      changed();
    });
  guiReader.registerCallback("pantsColor.down", [=](Widget*) {
      m_pantsColor--;
      fetchChild<ButtonWidget>("btnToggleClothing")->setChecked(true);
      changed();
    });
  guiReader.registerCallback("personality.up", [=](Widget*) {
      m_personality++;
      changed();
    });
  guiReader.registerCallback("personality.down", [=](Widget*) {
      m_personality--;
      changed();
    });
  guiReader.registerCallback("toggleClothing", [=](Widget*) {
      changed();
    });

  guiReader.registerCallback("randomName", [=](Widget*) { randomizeName(); });
  guiReader.registerCallback("randomize", [=](Widget*) { randomize(); });

  guiReader.registerCallback("name", [=](Widget* object) { nameBoxCallback(object); });

  guiReader.registerCallback("species", [=](Widget* button) {
      size_t speciesChoice = convert<ButtonWidget>(button)->buttonGroupId();
      if (speciesChoice < m_speciesList.size() && speciesChoice != m_speciesChoice) {
        m_speciesChoice = speciesChoice;
        randomize();
        randomizeName();
      }
    });
  guiReader.registerCallback("gender", [=](Widget* button) {
      m_genderChoice = convert<ButtonWidget>(button)->buttonGroupId();
      changed();
    });

  guiReader.registerCallback("mode", [=](Widget* button) {
      m_modeChoice = convert<ButtonWidget>(button)->buttonGroupId();
      changed();
    });

  guiReader.construct(root.assets()->json("/interface/windowconfig/charcreation.config:paneLayout"), this);

  createPlayer();

  RandomSource random;
  m_speciesChoice = random.randu32() % m_speciesList.size();
  m_genderChoice = random.randu32();
  m_modeChoice = 1;
  randomize();
  randomizeName();
}

void CharCreationPane::createPlayer() {
  m_previewPlayer = Root::singleton().playerFactory()->create();
  try {
    auto portrait = fetchChild<PortraitWidget>("charPreview");
    if ((bool)portrait) {
      portrait->setEntity(m_previewPlayer);
    } else {
      throw CharCreationException("The charPreview portrait has the wrong type.");
    }
  } catch (CharCreationException const& e) {
    Logger::error("Character Preview portrait was not found in the json specification. {}", outputException(e, false));
  }
}

void CharCreationPane::randomize() {
  RandomSource random;
  m_bodyColor = random.randu32();
  m_hairChoice = random.randu32();
  m_alty = random.randu32();
  m_heady = random.randu32();
  m_shirtChoice = random.randu32();
  m_shirtColor = random.randu32();
  m_pantsChoice = random.randu32();
  m_pantsColor = random.randu32();
  m_personality = random.randu32();
  changed();
}

void CharCreationPane::tick() {
  Pane::tick();
  if (!active())
    return;
  if (!m_previewPlayer)
    return;
  m_previewPlayer->animatePortrait();
}

bool CharCreationPane::sendEvent(InputEvent const& event) {
  if (active() && m_previewPlayer) {
    if (event.is<KeyDownEvent>()) {
      auto actions = context()->actions(event);
      if (actions.contains(InterfaceAction::EmoteBlabbering))
        m_previewPlayer->addEmote(HumanoidEmote::Blabbering);
      if (actions.contains(InterfaceAction::EmoteShouting))
        m_previewPlayer->addEmote(HumanoidEmote::Shouting);
      if (actions.contains(InterfaceAction::EmoteHappy))
        m_previewPlayer->addEmote(HumanoidEmote::Happy);
      if (actions.contains(InterfaceAction::EmoteSad))
        m_previewPlayer->addEmote(HumanoidEmote::Sad);
      if (actions.contains(InterfaceAction::EmoteNeutral))
        m_previewPlayer->addEmote(HumanoidEmote::NEUTRAL);
      if (actions.contains(InterfaceAction::EmoteLaugh))
        m_previewPlayer->addEmote(HumanoidEmote::Laugh);
      if (actions.contains(InterfaceAction::EmoteAnnoyed))
        m_previewPlayer->addEmote(HumanoidEmote::Annoyed);
      if (actions.contains(InterfaceAction::EmoteOh))
        m_previewPlayer->addEmote(HumanoidEmote::Oh);
      if (actions.contains(InterfaceAction::EmoteOooh))
        m_previewPlayer->addEmote(HumanoidEmote::OOOH);
      if (actions.contains(InterfaceAction::EmoteBlink))
        m_previewPlayer->addEmote(HumanoidEmote::Blink);
      if (actions.contains(InterfaceAction::EmoteWink))
        m_previewPlayer->addEmote(HumanoidEmote::Wink);
      if (actions.contains(InterfaceAction::EmoteEat))
        m_previewPlayer->addEmote(HumanoidEmote::Eat);
      if (actions.contains(InterfaceAction::EmoteSleep))
        m_previewPlayer->addEmote(HumanoidEmote::Sleep);
    }
  }
  return Pane::sendEvent(event);
}

void CharCreationPane::randomizeName() {
  auto species = Root::singleton().speciesDatabase()->species(m_speciesList[m_speciesChoice]);
  auto tb = fetchChild<TextBoxWidget>("name");
  auto genderOption = species->options().genderOptions.wrap(m_genderChoice);
  int limiter = 100;
  while (!tb->setText(Root::singleton().nameGenerator()->generateName(species->nameGen(genderOption.gender)))) {
    if (limiter == 0)
      break;
    limiter--;
  }
  changed();
}

void CharCreationPane::changed() {
  auto& root = Root::singleton();

  auto textBox = fetchChild<TextBoxWidget>("name");
  auto speciesDefinition = Root::singleton().speciesDatabase()->species(m_speciesList[m_speciesChoice]);
  auto species = speciesDefinition->options();
  auto genderOptions = species.genderOptions.wrap(m_genderChoice);
  int genderIdx = pmod<int64_t>(m_genderChoice, species.genderOptions.size());

  auto labels = speciesDefinition->charGenTextLabels();

  fetchChild<LabelWidget>("labelMainSkinColor")->setText(labels[0]);
  fetchChild<LabelWidget>("labelHairStyle")->setText(labels[1]);
  fetchChild<LabelWidget>("labelShirt")->setText(labels[2]);
  fetchChild<LabelWidget>("labelPants")->setText(labels[3]);
  if (!labels[4].empty()) {
    fetchChild<LabelWidget>("labelAlty")->setText(labels[4]);
    fetchChild<LabelWidget>("labelAlty")->show();
    fetchChild<Widget>("alty")->show();
  } else {
    fetchChild<LabelWidget>("labelAlty")->hide();
    fetchChild<Widget>("alty")->hide();
  }
  fetchChild<LabelWidget>("labelHeady")->setText(labels[5]);
  fetchChild<LabelWidget>("labelShirtColor")->setText(labels[6]);
  fetchChild<LabelWidget>("labelPantsColor")->setText(labels[7]);
  fetchChild<LabelWidget>("labelPortrait")->setText(labels[8]);
  fetchChild<LabelWidget>("labelPersonality")->setText(labels[9]);

  if (auto speciesButton = fetchChild<ButtonWidget>(strf("species.{}", m_speciesChoice)))
    speciesButton->check();
  if (auto genderButton = fetchChild<ButtonWidget>(strf("gender.{}", genderIdx)))
    genderButton->check();

  auto modeButton = fetchChild<ButtonWidget>(strf("mode.{}", m_modeChoice));
  modeButton->check();
  setLabel("labelMode", modeButton->data().getString("description", "fail"));

  // Update the gender images for the new species
  for (size_t i = 0; i < species.genderOptions.size(); i++)
    if (auto button = fetchChild<ButtonWidget>(strf("gender.{}", i)))
      button->setOverlayImage(species.genderOptions[i].image);

  for (auto const& nameDefPair : root.speciesDatabase()->allSpecies()) {
    String name;
    SpeciesDefinitionPtr def;
    std::tie(name, def) = nameDefPair;
    // NOTE: Probably not hot enough to matter, but this contains and indexOf makes this loop
    // O(n^2).  This is less than ideal.
    if (m_speciesList.contains(name)) {
      if (auto bw = fetchChild<ButtonWidget>(strf("species.{}", m_speciesList.indexOf(name))))
        bw->setOverlayImage(def->options().genderOptions[genderIdx].characterImage);
    }
  }

  auto portrait = fetchChild<PortraitWidget>("charPreview");
  if (fetchChild<ButtonWidget>("btnToggleClothing")->isChecked())
    portrait->setMode(PortraitMode::Full);
  else
    portrait->setMode(PortraitMode::FullNude);

  auto gender = species.genderOptions.wrap(m_genderChoice);
  auto bodyColor = species.bodyColorDirectives.wrap(m_bodyColor);

  String altColor;

  if (species.altOptionAsUndyColor) {
    // undyColor
    altColor = species.undyColorDirectives.wrap(m_alty);
  }

  auto hair = gender.hairOptions.wrap(m_hairChoice);
  String hairColor = bodyColor;
  if (species.headOptionAsHairColor && species.altOptionAsHairColor) {
    hairColor = species.hairColorDirectives.wrap(m_heady);
    hairColor += species.undyColorDirectives.wrap(m_alty);
  } else if (species.headOptionAsHairColor) {
    hairColor = species.hairColorDirectives.wrap(m_heady);
  }

  if (species.hairColorAsBodySubColor)
    bodyColor += hairColor;

  String facialHair;
  String facialHairGroup;
  String facialHairDirective;
  if (species.headOptionAsFacialhair) {
    facialHair = gender.facialHairOptions.wrap(m_heady);
    facialHairGroup = gender.facialHairGroup;
    facialHairDirective = hairColor;
  }

  String facialMask;
  String facialMaskGroup;
  String facialMaskDirective;
  if (species.altOptionAsFacialMask) {
    facialMask = gender.facialMaskOptions.wrap(m_alty);
    facialMaskGroup = gender.facialMaskGroup;
    facialMaskDirective = "";
  }
  if (species.bodyColorAsFacialMaskSubColor)
    facialMaskDirective += bodyColor;
  if (species.altColorAsFacialMaskSubColor)
    facialMaskDirective += altColor;

  auto shirt = gender.shirtOptions.wrap(m_shirtChoice);
  auto pants = gender.pantsOptions.wrap(m_pantsChoice);

  m_previewPlayer->setModeType((PlayerMode)m_modeChoice);

  m_previewPlayer->setName(textBox->getText());

  m_previewPlayer->setSpecies(species.species);
  m_previewPlayer->setBodyDirectives(bodyColor + altColor);

  m_previewPlayer->setGender(GenderNames.getLeft(gender.name));

  m_previewPlayer->setHairGroup(gender.hairGroup);
  m_previewPlayer->setHairType(hair);
  m_previewPlayer->setHairDirectives(hairColor);

  m_previewPlayer->setEmoteDirectives(bodyColor + altColor);

  m_previewPlayer->setFacialHair(facialHairGroup, facialHair, facialHairDirective);

  m_previewPlayer->setFacialMask(facialMaskGroup, facialMask, facialMaskDirective);

  auto personality = speciesDefinition->personalities().wrap(m_personality);
  m_previewPlayer->setPersonality(personality);

  setShirt(shirt, m_shirtColor);
  setPants(pants, m_pantsColor);

  m_previewPlayer->finalizeCreation();
}

void CharCreationPane::setShirt(String const& shirt, size_t colorIndex) {
  auto& root = Root::singleton();

  while (m_previewPlayer->inventory()->chestArmor())
    m_previewPlayer->inventory()->consumeSlot(InventorySlot(EquipmentSlot::Chest));
  if (!shirt.empty()) {
    m_previewPlayer->inventory()->addItems(
        root.itemDatabase()->item({shirt, 1, JsonObject{{"colorIndex", colorIndex}}}));
  }
  m_previewPlayer->refreshEquipment();
}

void CharCreationPane::setPants(String const& pants, size_t colorIndex) {
  auto& root = Root::singleton();

  while (m_previewPlayer->inventory()->legsArmor())
    m_previewPlayer->inventory()->consumeSlot(InventorySlot(EquipmentSlot::Legs));
  if (!pants.empty()) {
    m_previewPlayer->inventory()->addItems(
        root.itemDatabase()->item({pants, 1, JsonObject{{"colorIndex", colorIndex}}}));
  }
  m_previewPlayer->refreshEquipment();
}

void CharCreationPane::nameBoxCallback(Widget* object) {
  if (as<TextBoxWidget>(object))
    changed();
  else
    throw GuiException("Invalid object type, expected TextBoxWidget.");
}

PanePtr CharCreationPane::createTooltip(Vec2I const& screenPosition) {
  // what's under my cursor
  if (WidgetPtr child = getChildAt(screenPosition)) {
    // is it a species button ?
    if (child->parent()->name() == "species") {
      // which species is it ?
      size_t speciesIndex = convert<ButtonWidget>(child)->buttonGroupId();

      // no tooltips for unassigned button indices
      if (speciesIndex >= m_speciesList.size())
        return {};

      String speciesName = m_speciesList[speciesIndex];
      Star::SpeciesDefinitionPtr speciesDefinition = Root::singleton().speciesDatabase()->species(speciesName);

      // make a tooltip from the config file
      PanePtr tooltip = make_shared<Pane>();
      tooltip->removeAllChildren();
      GuiReader reader;
      auto& root = Root::singleton();
      String tooltipKind = "/interface/tooltips/species.tooltip";
      reader.construct(root.assets()->json(tooltipKind), tooltip.get());

      // find out the gender option block from the currently selected gender
      auto genderOption = speciesDefinition->options().genderOptions.wrap(m_genderChoice);
      // makes an icon out of the default gendered character image
      WidgetPtr titleIcon = make_shared<ImageWidget>(genderOption.characterImage);

      // read the description out of the already loaded species database.
      String title = speciesDefinition->tooltip().title;
      String subTitle = speciesDefinition->tooltip().subTitle;
      tooltip->setTitle(titleIcon, title, subTitle);

      tooltip->setLabel("descriptionLabel", speciesDefinition->tooltip().description);

      return tooltip;
    }
  }

  return {};
}

}

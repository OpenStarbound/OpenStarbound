#include "StarWidgetParsing.hpp"
#include "StarRoot.hpp"
#include "StarJsonExtra.hpp"
#include "StarImageMetadataDatabase.hpp"
#include "StarPane.hpp"
#include "StarButtonGroup.hpp"
#include "StarButtonWidget.hpp"
#include "StarCanvasWidget.hpp"
#include "StarImageWidget.hpp"
#include "StarImageStretchWidget.hpp"
#include "StarPortraitWidget.hpp"
#include "StarFuelWidget.hpp"
#include "StarProgressWidget.hpp"
#include "StarLargeCharPlateWidget.hpp"
#include "StarTextBoxWidget.hpp"
#include "StarItemSlotWidget.hpp"
#include "StarItemGridWidget.hpp"
#include "StarListWidget.hpp"
#include "StarSliderBar.hpp"
#include "StarStackWidget.hpp"
#include "StarScrollArea.hpp"
#include "StarAssets.hpp"
#include "StarFlowLayout.hpp"
#include "StarVerticalLayout.hpp"
#include "StarTabSet.hpp"

namespace Star {

WidgetConstructResult::WidgetConstructResult() : zlevel() {}

WidgetConstructResult::WidgetConstructResult(WidgetPtr obj, String const& name, float zlevel)
  : obj(obj), name(name), zlevel(zlevel) {}

WidgetParser::WidgetParser() {
  // only the non-interactive ones by default
  m_constructors["widget"] = [=](String const& name, Json const& config) { return widgetHandler(name, config); };
  m_constructors["canvas"] = [=](String const& name, Json const& config) { return canvasHandler(name, config); };
  m_constructors["image"] = [=](String const& name, Json const& config) { return imageHandler(name, config); };
  m_constructors["imageStretch"] = [=](String const& name, Json const& config) { return imageStretchHandler(name, config); };
  m_constructors["label"] = [=](String const& name, Json const& config) { return labelHandler(name, config); };
  m_constructors["portrait"] = [=](String const& name, Json const& config) { return portraitHandler(name, config); };
  m_constructors["fuelGauge"] = [=](String const& name, Json const& config) { return fuelGaugeHandler(name, config); };
  m_constructors["progress"] = [=](String const& name, Json const& config) { return progressHandler(name, config); };
  m_constructors["largeCharPlate"] = [=](
      String const& name, Json const& config) { return largeCharPlateHandler(name, config); };
  m_constructors["container"] = [=](String const& name, Json const& config) { return containerHandler(name, config); };
  m_constructors["layout"] = [=](String const& name, Json const& config) { return layoutHandler(name, config); };

  m_callbacks["null"] = [](Widget*) {};
}

void WidgetParser::construct(Json const& config, Widget* widget) {
  m_pane = dynamic_cast<Pane*>(widget);
  constructImpl(config, widget);
}

void WidgetParser::constructImpl(Json const& config, Widget* widget) {
  List<WidgetConstructResult> widgets = constructor(config);

  sort(widgets,
      [&](WidgetConstructResult const& a, WidgetConstructResult const& b) -> bool {
        if (a.zlevel == b.zlevel)
          return a.obj->position() < b.obj->position();
        return a.zlevel < b.zlevel;
      });

  for (auto const& res : widgets) {
    widget->addChild(res.name, res.obj);
    if (res.obj->hasFocus()) {
      if (m_pane)
        m_pane->setFocus(res.obj.get());
    }
  }
}

WidgetPtr WidgetParser::makeSingle(String const& name, Json const& config) {
  if (!m_constructors.contains(config.getString("type"))) {
    throw WidgetParserException(strf("Unknown type in gui json. {}", config.getString("type")));
  }

  auto constructResult = m_constructors.get(config.getString("type"))(name, config);
  return constructResult.obj;
}

List<WidgetConstructResult> WidgetParser::constructor(Json const& config) {
  List<WidgetConstructResult> widgets;

  auto addWidget = [this, &widgets](Json const& memberConfig) {
    if (memberConfig.type() != Json::Type::Object || !memberConfig.contains("type") || !memberConfig.contains("name"))
      throw WidgetParserException(
          "Malformed gui json: member configuration is either not a map, or does not specify a widget name and type");
    String type = memberConfig.getString("type");
    if (type == "include") {
      widgets.appendAll(constructor(Root::singleton().assets()->json(memberConfig.getString("file"))));
    } else {
      if (!m_constructors.contains(type)) {
        throw WidgetParserException(strf("Unknown type in gui json. {}", type));
      }
      auto constructResult =
          m_constructors.get(memberConfig.getString("type"))(memberConfig.getString("name"), memberConfig);
      if (constructResult.obj)
        widgets.append(constructResult);
    }
  };

  if (config.isType(Json::Type::Object)) {
    for (auto const& kvpair : config.iterateObject())
      addWidget(kvpair.second.set("name", kvpair.first));
  } else if (config.isType(Json::Type::Array)) {
    for (auto const& elem : config.iterateArray())
      addWidget(elem);
  } else {
    throw WidgetParserException(strf("Malformed gui json, expected a Map or a List. Instead got {}", config));
  }

  return widgets;
}

void WidgetParser::registerCallback(String const& name, WidgetCallbackFunc callback) {
  m_callbacks[name] = callback;
}

WidgetConstructResult WidgetParser::buttonHandler(String const& name, Json const& config) {
  String baseImage;
  bool invisible = config.getBool("invisible", false);

  try {
    if (!invisible)
      baseImage = config.getString("base");
  } catch (MapException const& e) {
    throw WidgetParserException(
        strf("Malformed gui json, missing a required value in the map. {}", outputException(e, false)));
  }

  String hoverImage = config.getString("hover", "");
  String pressedImage = config.getString("pressed", "");
  String disabledImage = config.getString("disabledImage", "");

  String baseImageChecked = config.getString("baseImageChecked", "");
  String hoverImageChecked = config.getString("hoverImageChecked", "");
  String pressedImageChecked = config.getString("pressedImageChecked", "");
  String disabledImageChecked = config.getString("disabledImageChecked", "");

  String callback = config.getString("callback", name);

  if (!m_callbacks.contains(callback))
    throw WidgetParserException::format("Failed to find callback named: {}", callback);
  WidgetCallbackFunc callbackFunc = m_callbacks.get(callback);

  auto button = make_shared<ButtonWidget>(callbackFunc, baseImage, hoverImage, pressedImage, disabledImage);
  button->setCheckedImages(baseImageChecked, hoverImageChecked, pressedImageChecked, disabledImageChecked);
  common(button, config);

  button->setInvisible(invisible);

  if (config.contains("caption"))
    button->setText(config.getString("caption"));

  if (config.contains("pressedOffset"))
    button->setPressedOffset(jsonToVec2I(config.get("pressedOffset")));

  if (config.contains("textOffset"))
    button->setTextOffset(jsonToVec2I(config.get("textOffset")));

  if (config.contains("checkable"))
    button->setCheckable(config.getBool("checkable"));

  if (config.contains("checked"))
    button->setChecked(config.getBool("checked"));

  String hAnchor = config.getString("textAlign", "center");
  if (hAnchor == "right") {
    button->setTextAlign(HorizontalAnchor::RightAnchor);
  } else if (hAnchor == "center") {
    button->setTextAlign(HorizontalAnchor::HMidAnchor);
  } else if (hAnchor == "left") {
    button->setTextAlign(HorizontalAnchor::LeftAnchor);
  } else {
    throw WidgetParserException(strf(
        "Malformed gui json, expected textAlign to be one of \"left\", \"right\", or \"center\", got {}", hAnchor));
  }

  if (config.contains("fontSize"))
    button->setFontSize(config.getInt("fontSize"));

  if (config.contains("fontDirectives"))
    button->setFontDirectives(config.getString("fontDirectives"));

  if (config.contains("fontColor"))
    button->setFontColor(jsonToColor(config.get("fontColor")));

  if (config.contains("fontColorDisabled"))
    button->setFontColorDisabled(jsonToColor(config.get("fontColorDisabled")));

  if (config.contains("disabled"))
    button->setEnabled(!config.getBool("disabled"));

  return WidgetConstructResult(button, name, config.getFloat("zlevel", 0));
}

WidgetConstructResult WidgetParser::imageHandler(String const& name, Json const& config) {
  auto image = make_shared<ImageWidget>();
  common(image, config);

  if (config.contains("file"))
    image->setImage(config.getString("file"));

  if (config.contains("drawables"))
    image->setDrawables(config.getArray("drawables").transformed([](Json const& d) { return Drawable(d); } ));

  if (config.contains("scale"))
    image->setScale(config.getFloat("scale"));

  if (config.contains("rotation"))
    image->setRotation(config.getFloat("rotation"));

  if (config.contains("centered"))
    image->setCentered(config.getBool("centered"));

  if (config.contains("trim"))
    image->setTrim(config.getBool("trim"));
  else
    image->setTrim(image->centered()); // once upon a time in a magical kingdom, these settings were linked

  if (config.contains("offset"))
    image->setOffset(jsonToVec2I(config.get("offset")));

  if (config.contains("maxSize"))
    image->setMaxSize(jsonToVec2I(config.get("maxSize")));

  if (config.contains("minSize"))
    image->setMinSize(jsonToVec2I(config.get("minSize")));

  return WidgetConstructResult(image, name, config.getFloat("zlevel", 0));
}

WidgetConstructResult WidgetParser::imageStretchHandler(String const& name, Json const& config) {
  ImageStretchSet stretchSet = parseImageStretchSet(config.get("stretchSet"));
  GuiDirection direction = GuiDirectionNames.getLeft(config.getString("direction", "horizontal"));

  auto imageStretch = make_shared<ImageStretchWidget>(stretchSet, direction);
  common(imageStretch, config);

  return WidgetConstructResult(imageStretch, name, config.getFloat("zlevel", 0));
}

WidgetConstructResult WidgetParser::spinnerHandler(String const& name, Json const& config) {
  auto container = make_shared<Widget>();
  common(container, config);

  String callback = config.getString("callback", name);

  if (!m_callbacks.contains(callback + ".up"))
    throw WidgetParserException::format("Failed to find spinner callback named: '{}.up'", name);
  if (!m_callbacks.contains(callback + ".down"))
    throw WidgetParserException::format("Failed to find spinner callback named: '{}.down'", name);

  WidgetCallbackFunc callbackDown = m_callbacks.get(callback + ".down");
  WidgetCallbackFunc callbackUp = m_callbacks.get(callback + ".up");

  auto assets = Root::singleton().assets();
  auto imgMetadata = Root::singleton().imageMetadataDatabase();

  auto leftBase = assets->json("/interface.config:spinner.leftBase").toString();
  auto leftHover = assets->json("/interface.config:spinner.leftHover").toString();
  auto rightBase = assets->json("/interface.config:spinner.rightBase").toString();
  auto rightHover = assets->json("/interface.config:spinner.rightHover").toString();

  auto imageSize = imgMetadata->imageSize(leftBase);
  auto padding = assets->json("/interface.config:spinner.defaultPadding").toInt();

  float upOffset = config.getFloat("upOffset", (float)imageSize[0] + padding);

  auto down = make_shared<ButtonWidget>(
      callbackDown, config.getString("leftBase", leftBase), config.getString("leftHover", leftHover));
  auto up = make_shared<ButtonWidget>(
      callbackUp, config.getString("rightBase", rightBase), config.getString("rightHover", rightHover));
  up->setPosition(up->position() + Vec2I(upOffset, 0));

  container->addChild("down", down);
  container->addChild("up", up);
  container->disableScissoring();
  container->markAsContainer();
  container->determineSizeFromChildren();

  return WidgetConstructResult(container, name, config.getFloat("zlevel", 0));
}

WidgetConstructResult WidgetParser::radioGroupHandler(String const& name, Json const& config) {
  auto buttonGroup = make_shared<ButtonGroupWidget>();
  common(buttonGroup, config);
  buttonGroup->markAsContainer();
  buttonGroup->disableScissoring();

  buttonGroup->setToggle(config.getBool("toggleMode", false));

  String callback = config.getString("callback", name);

  JsonArray buttons;
  try {
    buttons = config.getArray("buttons");
  } catch (MapException const& e) {
    throw WidgetParserException(
        strf("Malformed gui json, missing a required value in the map. {}", outputException(e, false)));
  }

  if (!m_callbacks.contains(callback))
    throw WidgetParserException::format("Failed to find callback named: '{}'", callback);

  String baseImage = config.getString("baseImage", "");
  String hoverImage = config.getString("hoverImage", "");
  String baseImageChecked = config.getString("baseImageChecked", "");
  String hoverImageChecked = config.getString("hoverImageChecked", "");
  String pressedImage = config.getString("pressedImage", "");
  String disabledImage = config.getString("disabledImage", "");
  String pressedImageChecked = config.getString("pressedImageChecked", "");
  String disabledImageChecked = config.getString("disabledImageChecked", "");

  for (auto btnConfig : buttons) {
    try {
      auto overlayImage = btnConfig.getString("image", "");
      auto id = btnConfig.getInt("id", ButtonGroup::NoButton);

      auto button = make_shared<ButtonWidget>();
      button->setButtonGroup(buttonGroup, id);

      button->setImages(btnConfig.getString("baseImage", baseImage),
          btnConfig.getString("hoverImage", hoverImage),
          btnConfig.getString("pressedImage", pressedImage),
          btnConfig.getString("disabledImage", disabledImage));
      button->setCheckedImages(btnConfig.getString("baseImageChecked", baseImageChecked),
          btnConfig.getString("hoverImageChecked", hoverImageChecked),
          btnConfig.getString("pressedImageChecked", pressedImageChecked),
          btnConfig.getString("hoverImageChecked", disabledImageChecked));
      button->setOverlayImage(overlayImage);

      if (btnConfig.getBool("disabled", false))
        button->disable();

      if (btnConfig.getBool("selected", false))
        button->check();

      if (btnConfig.contains("fontSize"))
        button->setFontSize(btnConfig.getInt("fontSize"));

      if (btnConfig.contains("fontColor"))
        button->setFontColor(jsonToColor(btnConfig.get("fontColor")));

      if (btnConfig.contains("fontColorChecked"))
        button->setFontColorChecked(jsonToColor(btnConfig.get("fontColorChecked")));

      if (btnConfig.contains("fontColorDisabled"))
        button->setFontColorDisabled(jsonToColor(btnConfig.get("fontColorDisabled")));

      if (btnConfig.contains("text"))
        button->setText(btnConfig.getString("text"));

      if (btnConfig.contains("pressedOffset"))
        button->setPressedOffset(jsonToVec2I(btnConfig.get("pressedOffset")));

      common(button, btnConfig);

      buttonGroup->addChild(strf("{}", button->buttonGroupId()), button);
    } catch (MapException const& e) {
      throw WidgetParserException(
          strf("Malformed gui json, missing a required value in the map. {}", outputException(e, false)));
    }
  }

  // Set callback after all other buttons are loaded, to avoid callbacks being
  // called during reading.
  buttonGroup->setCallback(m_callbacks.get(callback));
  return WidgetConstructResult(buttonGroup, name, config.getFloat("zlevel", 0));
}

WidgetConstructResult WidgetParser::portraitHandler(String const& name, Json const& config) {
  auto portrait = make_shared<PortraitWidget>();

  if (config.contains("portraitMode"))
    portrait->setMode(PortraitModeNames.getLeft(config.getString("portraitMode")));

  portrait->setScale(config.getFloat("scale", 1));

  common(portrait, config);

  return WidgetConstructResult(portrait, name, config.getFloat("zlevel", 0));
}

WidgetConstructResult WidgetParser::textboxHandler(String const& name, Json const& config) {
  String callback = config.getString("callback", name);

  if (!m_callbacks.contains(callback))
    throw WidgetParserException::format("Failed to find textbox callback named: '{}'", name);

  WidgetCallbackFunc callbackFunc = m_callbacks.get(callback);

  String initialText = config.getString("value", "");
  String hintText = config.getString("hint", "");
  auto textbox = make_shared<TextBoxWidget>(initialText, hintText, callbackFunc);

  if (config.contains("blur"))
    textbox->setOnBlurCallback(m_callbacks.get(config.getString("blur")));
  if (config.contains("enterKey"))
    textbox->setOnEnterKeyCallback(m_callbacks.get(config.getString("enterKey")));
  if (config.contains("escapeKey"))
    textbox->setOnEscapeKeyCallback(m_callbacks.get(config.getString("escapeKey")));

  if (config.contains("nextFocus"))
    textbox->setNextFocus(config.getString("nextFocus"));
  if (config.contains("prevFocus"))
    textbox->setPrevFocus(config.getString("prevFocus"));

  String hAnchor = config.getString("textAlign", "left");
  if (hAnchor == "right") {
    textbox->setTextAlign(HorizontalAnchor::RightAnchor);
  } else if (hAnchor == "center") {
    textbox->setTextAlign(HorizontalAnchor::HMidAnchor);
  } else if (hAnchor != "left") {
    throw WidgetParserException(strf(
        "Malformed gui json, expected textAlign to be one of \"left\", \"right\", or \"center\", got {}", hAnchor));
  }

  if (config.contains("fontSize"))
    textbox->setFontSize(config.getInt("fontSize"));
  if (config.contains("color"))
    textbox->setColor(jsonToColor(config.get("color")));
  if (config.contains("directives"))
    textbox->setDirectives(config.getString("directives"));
  if (config.contains("border"))
    textbox->setDrawBorder(config.getBool("border"));
  if (config.contains("maxWidth"))
    textbox->setMaxWidth(config.getInt("maxWidth"));
  if (config.contains("regex"))
    textbox->setRegex(config.getString("regex"));
  if (config.contains("hidden"))
    textbox->setHidden(config.getBool("hidden"));

  common(textbox, config);

  return WidgetConstructResult(textbox, name, config.getFloat("zlevel", 0));
}

WidgetConstructResult WidgetParser::labelHandler(String const& name, Json const& config) {
  String text = config.getString("value", "");

  Color color = Color::White;
  if (config.contains("color"))
    color = jsonToColor(config.get("color"));
  HorizontalAnchor hAnchor = HorizontalAnchorNames.getLeft(config.getString("hAnchor", "left"));
  VerticalAnchor vAnchor = VerticalAnchorNames.getLeft(config.getString("vAnchor", "bottom"));

  auto label = make_shared<LabelWidget>(text, color, hAnchor, vAnchor);
  common(label, config);
  if (config.contains("fontSize"))
    label->setFontSize(config.getInt("fontSize"));
  if (config.contains("wrapWidth"))
    label->setWrapWidth(config.getInt("wrapWidth"));
  if (config.contains("charLimit"))
    label->setTextCharLimit(config.getInt("charLimit"));
  if (config.contains("lineSpacing"))
    label->setLineSpacing(config.getFloat("lineSpacing"));
  if (config.contains("directives"))
    label->setDirectives(config.getString("directives"));

  return WidgetConstructResult(label, name, config.getFloat("zlevel", 0));
}

WidgetConstructResult WidgetParser::itemSlotHandler(String const& name, Json const& config) {
  String backingImage = config.getString("backingImage", "");
  String callback = config.getString("callback", name);

  String rightClickCallback;
  if (callback.equals("null"))
    rightClickCallback = callback;
  else
    rightClickCallback = callback + ".right";
  rightClickCallback = config.getString("rightClickCallback", rightClickCallback);

  auto itemSlot = make_shared<ItemSlotWidget>(ItemPtr(), backingImage);

  if (!m_callbacks.contains(callback))
    throw WidgetParserException::format("Failed to find itemSlot callback named: '{}'", callback);
  itemSlot->setCallback(m_callbacks.get(callback));

  if (!m_callbacks.contains(rightClickCallback))
    throw WidgetParserException::format("Failed to find itemslot rightClickCallback named: '{}'", rightClickCallback);

  itemSlot->setRightClickCallback(m_callbacks.get(rightClickCallback));
  itemSlot->setBackingImageAffinity(config.getBool("showBackingImageWhenFull", false), config.getBool("showBackingImageWhenEmpty", true));
  itemSlot->showDurability(config.getBool("showDurability", false));
  itemSlot->showCount(config.getBool("showCount", true));
  itemSlot->showRarity(config.getBool("showRarity", true));

  common(itemSlot, config);

  return WidgetConstructResult(itemSlot, name, config.getFloat("zlevel", 0));
}

WidgetConstructResult WidgetParser::itemGridHandler(String const& name, Json const& config) {
  Vec2I dimensions;
  Vec2I rowSpacing;
  Vec2I columnSpacing;
  try {
    dimensions = jsonToVec2I(config.get("dimensions"));
    if (config.contains("spacing")) {
      auto spacing = jsonToVec2I(config.get("spacing"));
      rowSpacing = {spacing[0], 0};
      columnSpacing = {0, spacing[1]};
    } else {
      rowSpacing = jsonToVec2I(config.get("rowSpacing"));
      columnSpacing = jsonToVec2I(config.get("columnSpacing"));
    }
  } catch (MapException const& e) {
    throw WidgetParserException::format("Malformed gui json, missing a required value in the map. {}", outputException(e, false));
  }

  String backingImage = config.getString("backingImage", "");
  String callback = config.getString("callback", name);
  String rightClickCallback;
  if (callback.equals("null"))
    rightClickCallback = callback;
  else
    rightClickCallback = callback + ".right";
  rightClickCallback = config.getString("rightClickCallback", rightClickCallback);

  unsigned slotOffset = config.getInt("slotOffset", 0);

  auto itemGrid = make_shared<ItemGridWidget>(ItemBagConstPtr(), dimensions, rowSpacing, columnSpacing, backingImage, slotOffset);

  if (!m_callbacks.contains(callback))
    throw WidgetParserException::format("Failed to find itemgrid callback named: '{}'", callback);

  itemGrid->setCallback(m_callbacks.get(callback));

  itemGrid->setBackingImageAffinity(
      config.getBool("showBackingImageWhenFull", false), config.getBool("showBackingImageWhenEmpty", true));
  itemGrid->showDurability(config.getBool("showDurability", false));

  if (!m_callbacks.contains(rightClickCallback))
    throw WidgetParserException::format("Failed to find itemgrid rightClickCallback named: '{}'", rightClickCallback);

  itemGrid->setRightClickCallback(m_callbacks.get(rightClickCallback));

  common(itemGrid, config);

  return WidgetConstructResult(itemGrid, name, config.getFloat("zlevel", 0));
}

WidgetConstructResult WidgetParser::listHandler(String const& name, Json const& config) {
  Json schema;
  try {
    schema = config.get("schema");
  } catch (MapException const& e) {
    throw WidgetParserException(
        strf("Malformed gui json, missing a required value in the map. {}", outputException(e, false)));
  }

  auto list = make_shared<ListWidget>(schema);
  common(list, config);

  if (auto callback = m_callbacks.value(config.getString("callback", name)))
    list->setCallback(callback);

  list->setFillDown(config.getBool("fillDown", false));
  list->setColumns(config.getUInt("columns", 1));

  return WidgetConstructResult(list, name, config.getFloat("zlevel", 0));
}

WidgetConstructResult WidgetParser::sliderHandler(String const& name, Json const& config) {
  try {
    auto grid = config.getString("gridImage");
    auto slider = make_shared<SliderBarWidget>(grid, config.getBool("showSpinner", true));
    common(slider, config);

    if (auto callback = m_callbacks.value(config.getString("callback", name)))
      slider->setCallback(callback);

    if (config.contains("range")) {
      auto rangeConfig = config.getArray("range");
      slider->setRange(rangeConfig.get(0).toInt(), rangeConfig.get(1).toInt(), rangeConfig.get(2).toInt());
    }

    if (config.contains("jogImages")) {
      auto jogConfig = config.get("jogImages");
      slider->setJogImages(jogConfig.getString("baseImage"),
          jogConfig.getString("hoverImage", ""),
          jogConfig.getString("pressedImage", ""),
          jogConfig.getString("disabledImage", ""));
    }

    if (config.contains("disabled"))
      slider->setEnabled(!config.getBool("disabled"));

    return WidgetConstructResult(slider, name, config.getFloat("zlevel", 0));
  } catch (MapException const& e) {
    throw WidgetParserException::format(
        "Malformed gui json, missing a required value in the map. {}", outputException(e, false));
  }
}

WidgetConstructResult WidgetParser::largeCharPlateHandler(String const& name, Json const& config) {
  String callback = config.getString("callback", name);

  if (!m_callbacks.contains(callback))
    throw WidgetParserException::format("Failed to find callback named: '{}'", name);

  auto charPlate = make_shared<LargeCharPlateWidget>(m_callbacks.get(callback));
  common(charPlate, config);

  return WidgetConstructResult(charPlate, name, config.getFloat("zlevel", 0));
}

WidgetConstructResult WidgetParser::tabSetHandler(String const& name, Json const& config) {
  TabSetConfig tabSetConfig;

  tabSetConfig.tabButtonBaseImage = config.getString("tabButtonBaseImage");
  tabSetConfig.tabButtonHoverImage = config.getString("tabButtonHoverImage");
  tabSetConfig.tabButtonPressedImage = config.getString("tabButtonPressedImage", tabSetConfig.tabButtonHoverImage);

  tabSetConfig.tabButtonBaseImageSelected =
      config.getString("tabButtonBaseImageSelected", tabSetConfig.tabButtonBaseImage);
  tabSetConfig.tabButtonHoverImageSelected =
      config.getString("tabButtonHoverImageSelected", tabSetConfig.tabButtonHoverImage);
  tabSetConfig.tabButtonPressedImageSelected =
      config.getString("tabButtonPressedImageSelected", tabSetConfig.tabButtonHoverImageSelected);

  Json defaultPressedOffset = Root::singleton().assets()->json("/interface.config:buttonPressedOffset");
  tabSetConfig.tabButtonPressedOffset = jsonToVec2I(config.get("tabButtonPressedOffset", defaultPressedOffset));
  tabSetConfig.tabButtonTextOffset = config.opt("tabButtonTextOffset").apply(jsonToVec2I).value();
  tabSetConfig.tabButtonSpacing = config.opt("tabButtonSpacing").apply(jsonToVec2I).value();

  auto tabSet = make_shared<TabSetWidget>(tabSetConfig);
  common(tabSet, config);

  try {
    for (auto entry : config.get("tabs").iterateArray()) {
      auto widget = make_shared<Widget>();
      constructImpl(entry.get("children"), widget.get());
      widget->determineSizeFromChildren();
      tabSet->addTab(entry.getString("tabName"), widget, entry.getString("tabTitle"));
    }
  } catch (JsonException const& e) {
    throw WidgetParserException(strf("Malformed gui json. {}", outputException(e, false)));
  }

  return WidgetConstructResult(tabSet, name, config.getFloat("zlevel", 0));
}

WidgetConstructResult WidgetParser::widgetHandler(String const& name, Json const& config) {
  auto widget = make_shared<Widget>();
  common(widget, config);

  return WidgetConstructResult(widget, name, config.getFloat("zlevel", 0));
}

WidgetConstructResult WidgetParser::containerHandler(String const& name, Json const& config) {
  auto widget = widgetHandler(name, config);
  widget.obj->disableScissoring();
  widget.obj->markAsContainer();
  return widget;
}

WidgetConstructResult WidgetParser::layoutHandler(String const& name, Json const& config) {
  String type;
  try {
    type = config.getString("layoutType");
  } catch (JsonException const&) {
    throw WidgetParserException("Failed to find layout type.  Options are: \"basic\", \"flow\", \"vertical\".");
  }
  WidgetPtr widget;
  if (type == "flow") {
    widget = make_shared<FlowLayout>();
    auto flow = convert<FlowLayout>(widget);
    try {
      flow->setSpacing(jsonToVec2I(config.get("spacing")));
    } catch (JsonException const& e) {
      throw WidgetParserException(strf("Parameter \"spacing\" in FlowLayout specification is invalid: {}.", outputException(e, false)));
    }
  } else if (type == "vertical") {
    widget = make_shared<VerticalLayout>();
    auto vert = convert<VerticalLayout>(widget);
    vert->setHorizontalAnchor(HorizontalAnchorNames.getLeft(config.getString("hAnchor", "left")));
    vert->setVerticalAnchor(VerticalAnchorNames.getLeft(config.getString("vAnchor", "top")));
    vert->setVerticalSpacing(config.getInt("spacing", 0));
    vert->setFillDown(config.getBool("fillDown", false));
  } else if (type == "basic") {
    widget = make_shared<Layout>();
  } else {
    throw WidgetParserException(strf("Invalid layout type \"{}\".  Options are \"basic\", \"flow\", \"vertical\".", type));
  }
  common(widget, config);
  if (config.contains("children"))
    constructImpl(config.get("children"), widget.get());
  widget->update();

  return WidgetConstructResult(widget, name, config.getFloat("zlevel", 0));
}

WidgetConstructResult WidgetParser::canvasHandler(String const& name, Json const& config) {
  auto canvas = make_shared<CanvasWidget>();
  canvas->setCaptureKeyboardEvents(config.getBool("captureKeyboardEvents", false));
  canvas->setCaptureMouseEvents(config.getBool("captureMouseEvents", false));
  common(canvas, config);

  return WidgetConstructResult(canvas, name, config.getFloat("zlevel", 0));
}

WidgetConstructResult WidgetParser::fuelGaugeHandler(String const& name, Json const& config) {
  auto fuelGauge = make_shared<FuelWidget>();
  common(fuelGauge, config);

  return WidgetConstructResult(fuelGauge, name, config.getFloat("zlevel", 0));
}

WidgetConstructResult WidgetParser::progressHandler(String const& name, Json const& config) {
  String background, overlay;
  ImageStretchSet progressSet;

  background = config.get("background", "").toString();
  overlay = config.get("overlay", "").toString();
  progressSet = parseImageStretchSet(config.get("progressSet"));
  GuiDirection direction = GuiDirectionNames.getLeft(config.getString("direction", "horizontal"));

  auto progress = make_shared<ProgressWidget>(background, overlay, progressSet, direction);

  common(progress, config);

  if (config.contains("barColor"))
    progress->setColor(jsonToColor(config.get("barColor")));

  if (config.contains("max"))
    progress->setMaxProgressLevel(config.getFloat("max"));

  if (config.contains("initial"))
    progress->setCurrentProgressLevel(config.getFloat("initial"));

  return WidgetConstructResult(progress, name, config.getFloat("zlevel", 0));
}

WidgetConstructResult WidgetParser::stackHandler(String const& name, Json const& config) {
  auto stack = make_shared<StackWidget>();

  if (config.contains("stack")) {
    auto stackList = config.getArray("stack");
    for (auto widgetCfg : stackList) {
      auto widget = make_shared<Widget>();
      constructImpl(widgetCfg, widget.get());
      widget->determineSizeFromChildren();
      stack->addChild(strf("{}", stack->numChildren()), widget);
    }
  }

  stack->determineSizeFromChildren();
  common(stack, config);
  return WidgetConstructResult(stack, name, config.getFloat("zlevel", 0));
}

WidgetConstructResult WidgetParser::scrollAreaHandler(String const& name, Json const& config) {
  auto scrollArea = make_shared<ScrollArea>();

  if (config.contains("buttons"))
    scrollArea->setButtonImages(config.get("buttons"));
  if (config.contains("thumbs"))
    scrollArea->setThumbImages(config.get("thumbs"));

  if (config.contains("children"))
    constructImpl(config.get("children"), scrollArea.get());

  if (config.contains("horizontalScroll"))
    scrollArea->setHorizontalScroll(config.getBool("horizontalScroll"));
  if (config.contains("verticalScroll"))
    scrollArea->setVerticalScroll(config.getBool("verticalScroll"));

  common(scrollArea, config);
  return WidgetConstructResult(scrollArea, name, config.getFloat("zlevel", 0));
}

void WidgetParser::common(WidgetPtr widget, Json const& config) {
  if (config.contains("rect")) {
    auto rect = jsonToRectI(config.get("rect"));
    widget->setPosition(rect.min());
    widget->setSize(rect.size());
  } else {
    if (config.contains("size")) {
      widget->setSize(jsonToVec2I(config.get("size")));
    }
    if (config.contains("position")) {
      widget->setPosition(jsonToVec2I(config.get("position")));
    }
  }
  if (config.contains("visible"))
    widget->setVisibility(config.getBool("visible"));
  if (config.getBool("focus", false))
    widget->focus();
  if (config.contains("data"))
    widget->setData(config.get("data"));
  if (!config.getBool("scissoring", true))
    widget->disableScissoring();
  widget->setMouseTransparent(config.getBool("mouseTransparent", false));
}

ImageStretchSet WidgetParser::parseImageStretchSet(Json const& config) {
  ImageStretchSet res;

  res.begin = config.get("begin", "").toString();
  res.inner = config.get("inner", "").toString();
  res.end = config.get("end", "").toString();
  String type = config.get("type", "stretch").toString();

  if (type == "repeat") {
    res.type = ImageStretchSet::ImageStretchType::Repeat;
  } else if (type == "stretch") {
    res.type = ImageStretchSet::ImageStretchType::Stretch;
  } else {
    throw WidgetParserException(strf("Could not parse Image Stretch Set, unknown type: {}"));
  }

  return res;
}

}

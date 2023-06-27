#include "StarGuiReader.hpp"
#include "StarPane.hpp"
#include "StarJsonExtra.hpp"

namespace Star {

GuiReader::GuiReader() {
  m_constructors["background"] = [=](
      String const& name, Json const& config) { return backgroundHandler(name, config); };
  m_constructors["button"] = [=](String const& name, Json const& config) { return buttonHandler(name, config); };
  m_constructors["itemslot"] = [=](String const& name, Json const& config) { return itemSlotHandler(name, config); };
  m_constructors["itemgrid"] = [=](String const& name, Json const& config) { return itemGridHandler(name, config); };
  m_constructors["list"] = [=](String const& name, Json const& config) { return listHandler(name, config); };
  m_constructors["panefeature"] = [=](
      String const& name, Json const& config) { return paneFeatureHandler(name, config); };
  m_constructors["radioGroup"] = [=](
      String const& name, Json const& config) { return radioGroupHandler(name, config); };
  m_constructors["spinner"] = [=](String const& name, Json const& config) { return spinnerHandler(name, config); };
  m_constructors["slider"] = [=](String const& name, Json const& config) { return sliderHandler(name, config); };
  m_constructors["textbox"] = [=](String const& name, Json const& config) { return textboxHandler(name, config); };
  m_constructors["title"] = [=](String const& name, Json const& config) { return titleHandler(name, config); };
  m_constructors["stack"] = [=](String const& name, Json const& config) { return stackHandler(name, config); };
  m_constructors["tabSet"] = [=](String const& name, Json const& config) { return tabSetHandler(name, config); };
  m_constructors["scrollArea"] = [=](
      String const& name, Json const& config) { return scrollAreaHandler(name, config); };
}

WidgetConstructResult GuiReader::titleHandler(String const&, Json const& config) {
  if (m_pane) {
    String title = config.getString("title", "");
    String subtitle = config.getString("subtitle", "");
    Json iconConfig = config.get("icon", Json());

    if (iconConfig.isNull()) {
      m_pane->setTitleString(title, subtitle);
    } else {
      try {
        String type = iconConfig.getString("type");
        auto icon = m_constructors.get(type)("icon", iconConfig);
        if (!icon.obj)
          throw WidgetParserException(strf("Title specified incompatible icon type: {}", type));
        m_pane->setTitle(icon.obj, title, subtitle);
      } catch (JsonException const& e) {
        throw WidgetParserException(strf("Malformed icon configuration data in title. {}", outputException(e, false)));
      }
    }
  } else {
    throw StarException("Only Pane controls support the 'title' command");
  }

  return {};
}

WidgetConstructResult GuiReader::paneFeatureHandler(String const&, Json const& config) {
  if (m_pane) {
    m_pane->setAnchor(PaneAnchorNames.getLeft(config.getString("anchor", "None")));
    if (config.contains("offset"))
      m_pane->setAnchorOffset(jsonToVec2I(config.get("offset")));
    if (config.getBool("positionLocked", false))
      m_pane->lockPosition();
  } else {
    throw StarException("Only Pane controls support the 'panefeature' command");
  }

  return {};
}

WidgetConstructResult GuiReader::backgroundHandler(String const&, Json const& config) {
  if (m_pane) {
    String header, body, footer;
    try {
      header = config.getString("fileHeader", "");
      body = config.getString("fileBody", "");
      footer = config.getString("fileFooter", "");
    } catch (MapException const& e) {
      throw WidgetParserException(
          strf("Malformed gui json, missing a required value in the map. {}", outputException(e, false)));
    }

    m_pane->setBG(header, body, footer);
  } else {
    throw StarException("Only Pane controls support the 'background' command");
  }

  return {};
}

}

#include "StarPane.hpp"
#include "StarRoot.hpp"
#include "StarJsonExtra.hpp"
#include "StarAssets.hpp"
#include "StarWidgetLuaBindings.hpp"
#include "StarLuaConverters.hpp"
#include "StarImageWidget.hpp"
#include "StarItemDatabase.hpp"
#include "StarGuiReader.hpp"

namespace Star {

EnumMap<PaneAnchor> const PaneAnchorNames{
    {PaneAnchor::None, "none"},
    {PaneAnchor::BottomLeft, "bottomLeft"},
    {PaneAnchor::BottomRight, "bottomRight"},
    {PaneAnchor::TopLeft, "topLeft"},
    {PaneAnchor::TopRight, "topRight"},
    {PaneAnchor::CenterBottom, "centerBottom"},
    {PaneAnchor::CenterTop, "centerTop"},
    {PaneAnchor::CenterLeft, "centerLeft"},
    {PaneAnchor::CenterRight, "centerRight"},
    {PaneAnchor::Center, "center"},
};

Pane::Pane() {
  m_dragActive = m_lockPosition = false;
  m_dismissed = true;
  m_centerOffset = Vec2I();
  m_anchor = PaneAnchor::None;
  m_anchorOffset = Vec2I();
  m_visible = false;
  m_hasDisplayed = false;

  auto assets = Root::singleton().assets();
  m_textStyle = assets->json("/interface.config:paneTextStyle");
  m_iconOffset = jsonToVec2I(assets->json("/interface.config:paneIconOffset"));
  m_titleOffset = jsonToVec2I(assets->json("/interface.config:paneTitleOffset"));
  m_subTitleOffset = jsonToVec2I(assets->json("/interface.config:paneSubTitleOffset"));
  m_titleColor = jsonToColor(assets->json("/interface.config:paneTitleColor"));
  m_subTitleColor = jsonToColor(assets->json("/interface.config:paneSubTitleColor"));
}

void Pane::displayed() {
  m_dismissed = false;
  m_hasDisplayed = true;
  show();
}

void Pane::dismissed() {
  if (m_clickDown)
    m_clickDown->mouseOut();
  m_clickDown.reset();
  if (m_mouseOver)
    m_mouseOver->mouseOut();
  m_mouseOver.reset();
  hide();
  m_dismissed = true;
}

void Pane::dismiss() {
  m_dismissed = true;
}

bool Pane::isDismissed() const {
  return m_dismissed;
}

bool Pane::isDisplayed() const {
  return !m_dismissed;
}

bool Pane::sendEvent(InputEvent const& event) {
  if (m_visible) {
    if (event.is<MouseButtonDownEvent>() || event.is<MouseButtonUpEvent>() || event.is<MouseMoveEvent>()
        || event.is<MouseWheelEvent>()) {
      Vec2I mousePos = *m_context->mousePosition(event);
      // First, handle preliminary mouse out / click up events
      if (m_mouseOver) {
        if (!m_mouseOver->inMember(mousePos) || !m_mouseOver->active()) {
          m_mouseOver->mouseOut();
          m_mouseOver.reset();
        }
      }

      if (event.is<MouseButtonUpEvent>())
        m_clickDown.reset();

      WidgetPtr newClickDown;
      WidgetPtr newMouseOver;
      WidgetPtr newFocusWidget;

      // Then, go through widgets in highest to lowest z-order and handle mouse
      // over, focus, and capture events.
      for (auto const& widget : reverseIterate(m_members)) {
        if (widget->inMember(mousePos) && widget->active() && widget->interactive()) {
          WidgetPtr topWidget = widget;
          WidgetPtr child = getChildAt(mousePos);
          if (child->active() && child->interactive()) {
            if (event.is<MouseButtonDownEvent>()
                && (event.get<MouseButtonDownEvent>().mouseButton == MouseButton::Left
                       || event.get<MouseButtonDownEvent>().mouseButton == MouseButton::Right)) {
              if (!newClickDown)
                newClickDown = child;

              if (!newFocusWidget)
                newFocusWidget = child;
            }

            if (!newMouseOver)
              newMouseOver = child;
          }
        }
      }

      if (m_clickDown != newClickDown)
        m_clickDown = newClickDown;

      if (m_mouseOver != newMouseOver) {
        if (m_mouseOver)
          m_mouseOver->mouseOut();
        m_mouseOver = newMouseOver;
        if (m_mouseOver) {
          if (m_clickDown == m_mouseOver)
            m_mouseOver->mouseReturnStillDown();
          else
            m_mouseOver->mouseOver();
        }
      }

      if (newFocusWidget && m_focusWidget != newFocusWidget) {
        if (auto focusWidget = m_focusWidget)
          focusWidget->blur();
        m_focusWidget = newFocusWidget;
        m_focusWidget->focus();
      }

      // Finally go through widgets in highest to lowest z-order and send the
      // raw event, stopping further processing if the widget consumes it.
      for (auto const& widget : reverseIterate(m_members)) {
        if (widget->inMember(mousePos) && widget->active() && widget->interactive()) {
          if (widget->sendEvent(event))
            return true;
        }
      }
    }

    if (event.is<MouseButtonDownEvent>()) {
      Vec2I mousePos = *m_context->mousePosition(event);
      if (inDragArea(mousePos) && !m_lockPosition) {
        setDragActive(true, mousePos);
        return true;
      }
      if (inWindow(mousePos))
        return true;
    }

    if (m_focusWidget) {
      if (m_focusWidget->sendEvent(event))
        return true;
    }
  }
  return false;
}

void Pane::setFocus(Widget const* focus) {
  if (m_focusWidget.get() == focus)
    return;
  if (m_focusWidget)
    m_focusWidget->blur();
  if (auto c = childPtr(focus))
    m_focusWidget = std::move(c);
  else
    throw GuiException("Cannot set focus on a widget which is not a child of this pane");
}

void Pane::removeFocus(Widget const* focus) {
  if (m_focusWidget.get() == focus)
    m_focusWidget.reset();
}

void Pane::removeFocus() {
  m_focusWidget.reset();
}

Pane const* Pane::window() const {
  return this;
}

Pane* Pane::window() {
  return this;
}

void Pane::update(float dt) {
  if (m_visible) {
    for (auto const& widget : m_members) {
      widget->update(dt);
      if ((m_focusWidget == widget) != widget->hasFocus()) {
        m_focusWidget.reset();
        widget->blur();
      }
    }
  }
}

void Pane::tick(float) {
  m_playingSounds.filter([](pair<String, AudioInstancePtr> const& p) {
    return p.second->finished() == false;
  });
}

bool Pane::dragActive() const {
  return m_dragActive;
}

Vec2I Pane::dragMouseOrigin() const {
  return m_dragMouseOrigin;
}

void Pane::setDragActive(bool dragActive, Vec2I dragMouseOrigin) {
  m_dragActive = dragActive;
  m_dragMouseOrigin = dragMouseOrigin;
}

void Pane::drag(Vec2I mousePosition) {
  Vec2I delta = mousePosition - m_dragMouseOrigin;
  setPosition(relativePosition() + delta);
  m_dragMouseOrigin += delta;
}

bool Pane::inWindow(Vec2I const& position) const {
  return screenBoundRect().contains(position);
}

bool Pane::inDragArea(Vec2I const& position) const {
  return inWindow(position) && (position[1] < (this->position()[1] + m_footerSize[1])
                                   || position[1] > (this->position()[1] + (m_footerSize[1] + m_bodySize[1])));
}

Vec2I Pane::cursorRelativeToPane(Vec2I const& position) const {
  return position - this->position();
}

Vec2I Pane::centerOffset() const {
  return m_centerOffset;
}

void Pane::setBG(BGResult const& res) {
  setBG(res.header, res.body, res.footer);
}

void Pane::setBG(String const& header, String const& body, String const& footer) {
  m_bgHeader = header;
  m_bgBody = body;
  m_bgFooter = footer;
  if (m_bgHeader != "") {
    m_headerSize = Vec2I(m_context->textureSize(m_bgHeader));
  } else {
    m_headerSize = {};
  }
  if (m_bgBody != "") {
    m_bodySize = Vec2I(m_context->textureSize(m_bgBody));
  } else {
    m_bodySize = {};
  }
  if (m_bgFooter != "") {
    m_footerSize = Vec2I(m_context->textureSize(m_bgFooter));
  } else {
    m_footerSize = {};
  }

  setSize(Vec2I(std::max(std::max(m_headerSize[0], m_bodySize[0]), m_footerSize[0]),
      m_headerSize[1] + m_bodySize[1] + m_footerSize[1]));
}

Pane::BGResult Pane::getBG() const {
  return {m_bgHeader, m_bgBody, m_bgFooter};
}

void Pane::lockPosition() {
  m_lockPosition = true;
}

void Pane::unlockPosition() {
  m_lockPosition = false;
}

void Pane::setTitle(WidgetPtr icon, String const& title, String const& subTitle) {
  m_icon = icon;
  m_title = title;
  m_subTitle = subTitle;
  if (m_icon) {
    m_icon->setParent(this);
    m_icon->show();
  }
}

void Pane::setTitleString(String const& title, String const& subTitle) {
  m_title = title;
  m_subTitle = subTitle;
}

void Pane::setTitleIcon(WidgetPtr icon) {
  m_icon = icon;
  if (m_icon) {
    m_icon->setParent(this);
    m_icon->show();
  }
}

String Pane::title() const {
  return m_title;
}

String Pane::subTitle() const {
  return m_subTitle;
}

WidgetPtr Pane::titleIcon() const {
  return m_icon;
}

PaneAnchor Pane::anchor() {
  return m_anchor;
}

void Pane::setAnchor(PaneAnchor anchor) {
  m_anchor = anchor;
}

Vec2I Pane::anchorOffset() const {
  return m_anchorOffset;
}

void Pane::setAnchorOffset(Vec2I anchorOffset) {
  m_anchorOffset = anchorOffset;
}

bool Pane::hasDisplayed() const {
  return m_hasDisplayed;
}

PanePtr Pane::createTooltip(Vec2I const&) {
  return {};
}

Maybe<String> Pane::cursorOverride(Vec2I const&) {
  return {};
}

Maybe<ItemPtr> Pane::shiftItemFromInventory(ItemPtr const& input) {
  return {};
}

LuaCallbacks Pane::makePaneCallbacks() {
  LuaCallbacks callbacks;

  callbacks.registerCallback("toWidget", [this]() -> LuaCallbacks {
    return LuaBindings::makeWidgetCallbacks(this, reader());
  });

  callbacks.registerCallback("dismiss", [this]() { dismiss(); });

  callbacks.registerCallback("playSound",
    [this](String const& audio, Maybe<int> loops, Maybe<float> volume) {
      auto assets = Root::singleton().assets();
      auto config = Root::singleton().configuration();
      auto audioInstance = make_shared<AudioInstance>(*assets->audio(audio));
      audioInstance->setVolume(volume.value(1.0));
      audioInstance->setLoops(loops.value(0));
      auto& guiContext = GuiContext::singleton();
      guiContext.playAudio(audioInstance);
      m_playingSounds.append({audio, std::move(audioInstance)});
    });

  callbacks.registerCallback("stopAllSounds", [this](Maybe<String> const& audio) {
      m_playingSounds.filter([audio](pair<String, AudioInstancePtr> const& p) {
        if (!audio || p.first == *audio) {
          p.second->stop();
          return false;
        }
        return true;
      });
    });

  callbacks.registerCallback("setTitle", [this](String const& title, String const& subTitle) {
      setTitleString(title, subTitle);
    });

  callbacks.registerCallback("setTitleIcon", [this](String const& image) {
      if (auto icon = as<ImageWidget>(titleIcon()))
        icon->setImage(image);
    });

  callbacks.registerCallback("getPosition", [this]() -> Vec2I          { return relativePosition(); });
  callbacks.registerCallback("setPosition", [this](Vec2I const& position) { setPosition(position);  });
  callbacks.registerCallback("getSize", [this]() -> Vec2I         {  return size();  });
  callbacks.registerCallback("setSize", [this](Vec2I const& size) { setSize(size);   });

  callbacks.registerCallback("addWidget", [this](Json const& newWidgetConfig, Maybe<String> const& newWidgetName) -> LuaCallbacks {
      String name = newWidgetName.value(toString(Random::randu64()));
      WidgetPtr newWidget = reader()->makeSingle(name, newWidgetConfig);
      this->addChild(name, newWidget);
      return LuaBindings::makeWidgetCallbacks(newWidget.get(), reader());
    });

  callbacks.registerCallback("removeWidget", [this](String const& widgetName) -> bool {
      return this->removeChild(widgetName);
    });

  callbacks.registerCallback("scale", []() -> int {
      return GuiContext::singleton().interfaceScale();
    });
  
  callbacks.registerCallback("isDisplayed", [this]() {
    return isDisplayed();
  });

  callbacks.registerCallback("hasFocus", [this]() {
    hasFocus();
  });

  callbacks.registerCallback("show", [this]() {
    show();
  });

  callbacks.registerCallback("hide", [this]() {
    hide();
  });

  return callbacks;
}

GuiReaderPtr Pane::reader() {
  return make_shared<GuiReader>();
}

void Pane::renderImpl() {
  if (m_bgFooter != "")
    m_context->drawInterfaceQuad(m_bgFooter, Vec2F(position()));

  if (m_bgBody != "")
    m_context->drawInterfaceQuad(m_bgBody, Vec2F(position()) + Vec2F(0, m_footerSize[1]));

  if (m_bgHeader != "") {
    auto headerPos = Vec2F(position()) + Vec2F(0, m_footerSize[1] + m_bodySize[1]);
    m_context->drawInterfaceQuad(m_bgHeader, headerPos);

    if (m_icon) {
      m_icon->setPosition(Vec2I(0, m_footerSize[1] + m_bodySize[1]) + m_iconOffset);
      m_icon->render(m_drawingArea);
      m_context->resetInterfaceScissorRect();
    }

    m_context->setTextStyle(m_textStyle);
    m_context->setFontColor(m_titleColor.toRgba());
    m_context->setFontMode(FontMode::Shadow);
    m_context->renderInterfaceText(m_title, {headerPos + Vec2F(m_titleOffset)});
    m_context->setFontColor(m_subTitleColor.toRgba());
    m_context->renderInterfaceText(m_subTitle, {headerPos + Vec2F(m_subTitleOffset)});
    m_context->clearTextStyle();
  }
}

}
